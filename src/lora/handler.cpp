/***************************************************
 * handler.cpp
 * Created on Wed, 15 Jul 2020 08:44:12 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <iostream>
#include <stdexcept>
#include "types.h"
#include "util/concat.h"
#include "message.h"
#include "handler.h"

namespace lora {

Handler::Handler(AppHandler *handler)
  : m_handler(handler)
{
}

void Handler::dataReceived(const GatewayId &id, Bytearray &data, const MediaParameters &params)
{
  Packet packet(data);

  switch(packet.type()) {
  case JOIN_REQUEST:
    processJoinRequest(id, packet.message<JoinRequest>(), params);
    break;
  case JOIN_ACCEPT:
    m_handler->log(WARN, "Join accept messages are not supported!");
    break;
  case UNCONFIRMED_DATA_UP:
    processData(id, packet.message<UnconfirmedDataUp>(), params);
    break;
  case UNCONFIRMED_DATA_DOWN:
    break;
  case CONFIRMED_DATA_UP:
    processData(id, packet.message<ConfirmedDataUp>(), params);
    break;
  case CONFIRMED_DATA_DOWN:
    break;
  case REJOIN_REQUEST:
    break;
  default:
    m_handler->log(ERR, "Unknown message type received!");
  }
}

void Handler::processJoinRequest(const GatewayId &id, Message &msg, const MediaParameters &p)
{
  m_handler->log(ERR, "Processing join request message");
  Packet joinAcceptPacket(JOIN_ACCEPT);
  const JoinRequest &request = dynamic_cast<const JoinRequest&>(msg);
  auto devInfos = m_handler->findDeviceInfo(request.devEui(), request.appEui());
  uint32_t nOnce = request.devNOnce();
  struct timespec curTime;
  clock_gettime(CLOCK_MONOTONIC_RAW, &curTime);

  for(auto devInfo : devInfos)
  {
    DeviceSession *session = devInfo->session;
    Device device(devInfo, session);

    msg.setDevice(&device);

    if(!msg.verify()) {
      m_handler->log(INFO, "LoRa packet verification failed!");
      continue;
    }

    if(session->devNOnce == nOnce) {
      m_handler->log(INFO, "Device NOnce is not incremented!");
      continue;
    }

    session->devNOnce = nOnce;

    JoinAccept &accept = joinAcceptPacket.message<JoinAccept>();
    accept.setDevice(&device);
    accept.setJoinNOnce(++ session->nOnce);
    accept.setNetworkId(session->networkId);
    accept.setDevAddress(session->deviceAddr);
    accept.setDlSettings(0);
    accept.setRxDelay(session->rxDelay1);

    int i = 0;
    for(uint32_t val : devInfo->cflist)
    {
      if(i < 4) {
        accept.cfList()[i ++] = val;
      }
    }

    session->lastAccessTime = curTime.tv_sec * 1000000 + curTime.tv_nsec / 1000;
    session->generateSessionKeys();
    session->fCntUp = -1;
    session->fCntDown = 0;
    m_handler->saveDeviceSession(session);

    auto payload = accept.getPayload();
    MediaSettings s;

    s.txTimestampUs = p.rxTimestampUs + session->joinAckDelay1 * 1000000; // +5 seconds
    s.frequency = p.frequency;
    s.rfChain = 0;
    s.modulation = p.modulation;
    s.dataRate = p.dataRate;
    s.codingRate = p.codingRate;
    s.polarity = INVERSE;

    sendData(id, payload, s);

    accept.parse(&device);
  }
}

void Handler::processData(const GatewayId &id, Message &msg, const MediaParameters &p)
{
  m_handler->log(ERR, "Processing data message");
  DataUp &request = dynamic_cast<DataUp&>(msg);
  uint32_t deviceAddr = request.devAddress();
  auto devSessions = m_handler->findDeviceSession(deviceAddr);
  std::unique_ptr<SendDataItem> toSend(new SendDataItem);
  struct timespec curTime;

  clock_gettime(CLOCK_MONOTONIC_RAW, &curTime);

  toSend->port = 0;
  toSend->confirmed = false;
  toSend->gatewayId.reset(id.clone());
  toSend->settings.frequency = p.frequency;
  toSend->settings.rfChain = 0;
  toSend->settings.modulation = p.modulation;
  toSend->settings.dataRate = p.dataRate;
  toSend->settings.codingRate = p.codingRate;
  toSend->settings.polarity = INVERSE;
  toSend->ctrl = 0;

  for(auto session : devSessions)
  {
    DeviceInfo *devInfo = session->device;
    Device device(devInfo, session);

    request.setDevice(&device);

    if(!request.verify()) {
      m_handler->log(INFO, "LoRa packet verification failed!");
      continue;
    }

    auto &sendQueue = m_handler->getDeviceSendQueue(session);

    session->lastReceivedUs = curTime.tv_sec * 1000000 + 
                              curTime.tv_nsec / 1000;
    session->lastReceivedMedia = p;

    toSend->session = session;
    auto timestampUs = p.rxTimestampUs + session->rxDelay1 * 1000000;
    toSend->settings.txTimestampUs = timestampUs > 0 ? timestampUs : 1;
    toSend->sendBefore = curTime.tv_sec * 1000000 + 
                         curTime.tv_nsec / 1000 +
                         session->rxDelay1 * 1000000;

    // Check if we got ACK
    if(request.ctrl() & ACK) {
      if(!sendQueue.empty()) {
        if(sendQueue.front().confirmed && sendQueue.front().transmitted) {
          sendQueue.pop_front();
        }
        else {
          m_handler->log(WARN, "Got ACK, but never sent confirmed packet!");
        }
      }
      else {
        m_handler->log(WARN, "Got ACK with empty queue!");
      }
    }

    bool retransmitted = (request.cnt() == session->fCntUp);
    bool gotCommands = false;
    
    const Bytearray &data = request.data();

    if(!retransmitted) {
      if(data.size()) {
        if(request.port() == 0) {
          m_handler->log(INFO, "Processing MAC command at port 0");
          processMacCommand(id, session, p, data);
          gotCommands = true;
        }
        else {
          m_handler->processAppData(session, request.port(), data);
        }
      }
      session->fCntUp = request.cnt();
      m_handler->saveDeviceSession(session);
    }
    else {
      m_handler->log(INFO, "LoRa packet was retransmitted!");
    }

    if(!request.opts().empty()) {
      processMacCommand(id, session, p, request.opts());
      gotCommands = true;
    }

    if(!sendQueue.empty()) {
      toSend->port = sendQueue.front().port;
      toSend->data = sendQueue.front().data;
      toSend->confirmed = sendQueue.front().confirmed;
    }

    bool confirmed = (request.type() == CONFIRMED_DATA_UP);

    if(confirmed) {
      toSend->ctrl |= ACK;
    }

    if(confirmed || gotCommands || !sendQueue.empty()) {
      if(!sendQueue.empty() && !toSend->confirmed) {
        sendQueue.pop_front();
      }
      else if(!sendQueue.empty()) {
        sendQueue.front().transmitted = true;
      }

      session->lastAccessTime = curTime.tv_sec * 1000000 + curTime.tv_nsec / 1000;
      m_sendQueue.push_back(toSend.release());
      break;
    }
  }
}

void 
Handler::processClassCSendQueue()
{
  std::list<DeviceSession *> sessions = m_handler->getDeviceSessions();
  struct timespec curTime;

  clock_gettime(CLOCK_MONOTONIC_RAW, &curTime);

  auto timestampUs = curTime.tv_sec * 1000000 + 
                     curTime.tv_nsec / 1000;

  for(auto session : sessions) {
    if(!session->device) {
      continue;
    }

    if(session->device->devClass != CLASS_C) {
      continue;
    }

    auto &sendQueue = m_handler->getDeviceSendQueue(session);

    if(sendQueue.empty()) {
      continue;
    }

    if(session->lastAccessTime > (timestampUs - 1000000)) {
      continue;
    }

    std::unique_ptr<SendDataItem> toSend(new SendDataItem);
    toSend->port = sendQueue.front().port;
    toSend->data = sendQueue.front().data;
    toSend->confirmed = sendQueue.front().confirmed;
    toSend->session = session;
    toSend->settings.frequency = session->rx2Channel != 0 ? session->rx2Channel : 869100000; // Default to RU868
    toSend->settings.rfChain = 0;
    toSend->settings.modulation = LORA;
    toSend->settings.dataRate = session->rx2Datarate;
    toSend->settings.codingRate = CR4_5;
    toSend->settings.polarity = INVERSE;
    toSend->settings.txTimestampUs = 0; // Immediately
    toSend->sendBefore = timestampUs;

    if(!toSend->confirmed) {
      sendQueue.pop_front();
    }
    else {
      ++ sendQueue.front().transmitted;
    }

    session->lastAccessTime = timestampUs;
    m_sendQueue.push_back(toSend.release());
  }
}

void 
Handler::processSendQueue()
{
  auto &commands = m_commands;

  processClassCSendQueue();

  if(m_sendQueue.empty()) {
    return;
  }

  struct timespec curTime;

  clock_gettime(CLOCK_MONOTONIC_RAW, &curTime);
  uint32_t curTs = curTime.tv_sec * 1000000 + curTime.tv_nsec / 1000;

  while(!m_sendQueue.empty() && curTs) {
    if(m_sendQueue.front()->sendBefore < (curTs - 200000)) {
      break;
    }

    std::unique_ptr<SendDataItem> sendData(m_sendQueue.front());

    m_sendQueue.pop_front();
    Bytearray data;

    auto it = commands.begin();
    while(it != commands.end()) {
      if(it->session == sendData->session) {
        concat_recursive(data, (uint8_t)it->command, it->params);
        it = commands.erase(it);
        continue;
      }

      ++ it;
    }

    if(!data.empty()) {
      if(sendData->port == 0) {
        concat_bytearray(sendData->data, data);
      }
      else {
        sendData->opts = data;
      }
    }

    sendDeviceData(*sendData);
  }
}

void
Handler::processMacCommand(const GatewayId &id, DeviceSession *session, const MediaParameters &params, const Bytearray &command)
{
  auto &commands = m_commands;//session->commands;
  for(unsigned int i = 0; i < command.size(); ++ i) {
    switch(command[i]) {
    case LinkCheckReq:
      {
      int gwcnt = 1;
      for(auto it = commands.begin(); it != commands.end(); ++ it) {
        if(it->session == session && it->command == LinkCheckAns) {
          gwcnt += it->params[1];
          it->params[1] += 1;
          break;
        }
      }

      if(gwcnt == 1) {
        commands.emplace_back(LinkCheckAns, session, id, (uint8_t)params.lsnr, (uint8_t)1);
      }
      }
      break;
    case LinkADRAns:
      // Status payload
      ++ i;
      break;
    case DutyCycleAns:
      // No payload
      break;
    case RXParamSetupAns:
      // Status payload
      ++ i;
      break;
    case DevStatusAns:
      // Battery level
      {
      int batLevel = command[i ++];
      // Margin
      int margin = command[i ++];
      m_handler->deviceStatus(session, batLevel, margin);
      }
      break;
    case NewChannelAns:
      // Status payload
      ++ i;
      break;
    case RXTimingSetupReq:
      {
      if(command.size() <= i + 1) {
        break; // response
      }
      uint8_t settings = command[i ++] & 0x0f;
      if(settings == 0) settings = 1;
      session->rxDelay1 = settings;
      m_handler->saveDeviceSession(session);
      commands.emplace_back(RXTimingSetupAns, session, id);
      }
      break;
    default:
      printf("Unknown MAC command %d", (unsigned int)command[i]);
      break;
    }
  }
}

void 
Handler::sendDeviceData(const SendDataItem &data)
{
  const DeviceInfo *devInfo = data.session->device;
  Device device(devInfo, data.session);
  uint32_t deviceAddr = data.session->deviceAddr;
  Packet ackPacket(data.confirmed?CONFIRMED_DATA_DOWN:UNCONFIRMED_DATA_DOWN);
  DataUp &ack = ackPacket.message<DataUp>();

  ack.setData(data.data);
  ack.setPort(data.port);
  ack.setOpts(data.opts);
  ack.setDevice(&device);
  ack.setCtrl(data.ctrl);
  ack.setCnt(data.session->fCntDown);
  ack.setDevAddress(deviceAddr);
  
  data.session->fCntDown ++;
  m_handler->saveDeviceSession(data.session);

  auto payload = ack.getPayload();

  if (data.gatewayId != nullptr) {
    sendData(*data.gatewayId, payload, data.settings);
  }
  else {
    for(auto &gw : m_gateways)
    {
      sendData(*gw.second, payload, data.settings);
    }
  }
}

void
Handler::addGateway(GatewayId *gw)
{
  std::string strId = gw->toString();

  if (m_gateways.find(strId) == m_gateways.end()) {
    std::cout << "Adding gateway " << strId << std::endl;
    m_gateways.emplace(strId, std::unique_ptr<GatewayId>(gw->clone()));
  }
}

SendData::SendData(int port, const Bytearray &data, bool confirm, DeviceSession *s)
  : port(port)
  , data(data)
  , confirmed(confirm)
  , session(s)
  , transmitted(0)
{ }

}
