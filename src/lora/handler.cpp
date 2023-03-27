/***************************************************
 * handler.cpp
 * Created on Wed, 15 Jul 2020 08:44:12 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <memory>
#include <iostream>
#include <stdexcept>
#include "types.h"
#include "util/concat.h"
#include "message.h"
#include "handler.h"

namespace lora {

Handler::Handler(AppHandler *handler)
  : m_handler(handler)
  , m_lastPacketTime(0)
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
    m_handler->log(WARN, "LORA: Join accept messages are not supported!");
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
    m_handler->log(ERR, "LORA: Unknown message type received!");
  }
}

void Handler::processJoinRequest(const GatewayId &id, Message &msg, const MediaParameters &p)
{
  Packet joinAcceptPacket(JOIN_ACCEPT);
  const JoinRequest &request = dynamic_cast<const JoinRequest&>(msg);
  std::string devEui = hexToString(request.devEui(), sizeof(EUI));
  std::string appEui = hexToString(request.appEui(), sizeof(EUI));
  m_handler->logf(INFO, "LORA: Processing join request message for device %s:%s", devEui.c_str(), appEui.c_str());
  auto devInfos = m_handler->findDeviceInfo(request.devEui(), request.appEui());
  uint32_t nOnce = request.devNOnce();

  struct timespec curTime;
  clock_gettime(CLOCK_MONOTONIC_RAW, &curTime);

  for(auto devInfo : devInfos)
  {
    DeviceSession *session = devInfo->session;
    Device device(devInfo, session);

    try {
      msg.setDevice(&device);
    }
    catch(std::exception &e) {
      m_handler->logf(ERR, "%s", e.what());
      continue;
    }

    if(!msg.verify()) {
      m_handler->log(ERR, "LORA: LoRa packet verification failed!");
      continue;
    }

    if(session->devNOnce == nOnce && session->devNOnce != 0) {
      m_handler->logf(ERR, "LORA: Device NOnce is not incremented (old: %u, new: %u)!", session->devNOnce, nOnce);
      continue;
    }

    session->devNOnce = nOnce;

    JoinAccept &accept = joinAcceptPacket.message<JoinAccept>();
    joinAcceptPacket.setDevice(&device);
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

    session->generateSessionKeys();
    session->fCntUp = -1;
    session->fCntDown = 0;
    session->lastAccessTime = curTime.tv_sec * 1000000 + curTime.tv_nsec / 1000;
    m_handler->saveDeviceSession(session);

    auto payload = accept.getPayload();
    MediaSettings s;

    unsigned int timestamp = p.rxTimestampUs + session->joinAckDelay1 * 1000000; // RX1 window

    if (m_lastPacketTime < timestamp) {
      if ((timestamp - m_lastPacketTime) < 500000) {
        timestamp = p.rxTimestampUs + (session->joinAckDelay1 + 1) * 1000000; // RX2 window
      }
    }
    else {
      if ((m_lastPacketTime - timestamp) < 500000) {
        timestamp = p.rxTimestampUs + (session->joinAckDelay1 + 1) * 1000000; // RX2 window
      }
    }

    m_lastPacketTime = timestamp;
    s.txTimestampUs = timestamp;
    s.frequency = p.frequency;
    s.rfChain = 0;
    s.modulation = p.modulation;
    s.dataRate = p.dataRate;
    s.codingRate = p.codingRate;
    s.polarity = INVERSE;

    m_handler->logf(INFO, "LORA: Sending join accept to device %s:%s!", devEui.c_str(), appEui.c_str());
    sendData(id, payload, s);
  }
}

void Handler::processData(const GatewayId &id, Message &msg, const MediaParameters &p)
{
  m_handler->log(INFO, "LORA: Processing data message");
  DataUp &request = dynamic_cast<DataUp&>(msg);
  uint32_t deviceAddr = request.devAddress();
  auto devSessions = m_handler->findDeviceSession(deviceAddr);
  std::unique_ptr<SendDataItem> toSend(new SendDataItem);
  struct timespec curTime;

  m_handler->logf(INFO, "LORA: Processing data message for device addr %u", deviceAddr);
  clock_gettime(CLOCK_MONOTONIC_RAW, &curTime);

  toSend->port = 0;
  toSend->confirmed = false;
  toSend->gatewayId = id.toString();
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

    m_handler->logf(INFO, "LORA: Processing data for device %u:%u", session->deviceAddr, session->networkId);

    try {
      request.setDevice(&device);
    }
    catch(std::exception &e) {
      m_handler->logf(ERR, "E: %s", e.what());
      continue;
    }

    if(!request.verify()) {
      m_handler->log(ERR, "LORA: LoRa packet verification failed!");
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
          m_handler->log(WARN, "LORA: Got ACK, but never sent confirmed packet!");
        }
      }
      else {
        m_handler->log(WARN, "LORA: Got ACK with empty queue!");
      }
    }

    bool retransmitted = (request.cnt() == session->fCntUp);
    bool gotCommands = false;
    
    const Bytearray &data = request.data();

    if(!retransmitted) {
      if(data.size()) {
        if(request.port() == 0) {
          m_handler->log(INFO, "LORA: Processing MAC command at port 0");
          processMacCommand(id, session, p, data);
          gotCommands = true;
        }
        else {
          m_handler->processAppData(session, request.port(), data);
        }
      }
      else {
        m_handler->log(INFO, "LORA: Empty data!");
      }
      session->fCntUp = request.cnt();
      m_handler->saveDeviceSession(session);
    }
    else {
      m_handler->log(WARN, "LORA: LoRa packet was retransmitted!");
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
      m_handler->log(INFO, "LORA: This is a confirmed packet, sending confirmation!");
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
      m_handler->log(INFO, "LORA: Processing LinkCheckReq");
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
      m_handler->log(INFO, "LORA: Processing DutyCycleAns");
      break;
    case RXParamSetupAns:
      // Status payload
      m_handler->log(INFO, "LORA: Processing RXParamSetupAns");
      ++ i;
      break;
    case DevStatusAns:
      // Battery level
      m_handler->log(INFO, "LORA: Processing DevStatusAns");
      {
      int batLevel = command[i ++];
      // Margin
      int margin = command[i ++];
      m_handler->deviceStatus(session, batLevel, margin);
      }
      break;
    case NewChannelAns:
      // Status payload
      m_handler->log(INFO, "LORA: Processing NewChannelAns");
      ++ i;
      break;
    case RXTimingSetupReq:
      m_handler->log(INFO, "LORA: Processing RXTimingSetupReq");
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
      m_handler->logf(INFO, "Unknown MAC command %d", (unsigned int)command[i]);
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

  std::string devEui = hexToString(devInfo->devEUI, sizeof(EUI));
  std::string appEui = hexToString(devInfo->appEUI, sizeof(EUI));

  if (m_gateways.find(data.gatewayId) != m_gateways.end()) {
    auto &gw = m_gateways.find(data.gatewayId)->second;
    m_handler->logf(INFO, "LORA: Sending device data through gw %s to device %s:%s", data.gatewayId.c_str(), devEui.c_str(), appEui.c_str());
    sendData(*gw, payload, data.settings);
  }
  else {
    for(auto &gw : m_gateways)
    {
      std::string strId = gw.second->toString();
      m_handler->logf(INFO, "LORA: Sending device data through gw %s to device %s:%s", strId.c_str(), devEui.c_str(), appEui.c_str());
      sendData(*gw.second, payload, data.settings);
    }
  }
}

void
Handler::addGateway(GatewayId *gw)
{
  std::string strId = gw->toString();

  if (m_gateways.find(strId) == m_gateways.end()) {
    m_handler->logf(INFO, "LORA: Adding gateway %s", strId.c_str());
    m_gateways.emplace(strId, std::unique_ptr<GatewayId>(gw->clone()));
  }
}

void
Handler::removeGateway(GatewayId *gw)
{
  std::string strId = gw->toString();

  if (m_gateways.find(strId) != m_gateways.end()) {
    m_handler->logf(INFO, "LORA: Removing gateway %s", strId.c_str());
    m_gateways.erase(strId);
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
