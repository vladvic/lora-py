/***************************************************
 * proto.cpp
 * Created on Thu, 16 Jul 2020 05:29:36 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <stdlib.h>
#include <algorithm>
#include <string>
#include <iostream>
#include "lora/util/concat.h"
#include "net/udpserver.h"
#include "pjson.h"
#include "base64.h"
#include "proto.h"

namespace semtech {

GatewayId::GatewayId(const unsigned char *id)
{
  m_id = lora::hexToString(id, 8);
}

lora::GatewayId *GatewayId::clone() const
{
  return new GatewayId(*this);
}

std::string GatewayId::toString() const
{
  return m_id;
}

std::string ProtoHandler::dataRateToString(lora::DataRate dr)
{
  if(dr == lora::DR0) {
    return "SF12BW125";
  }
  if(dr == lora::DR1) {
    return "SF11BW125";
  }
  if(dr == lora::DR2) {
    return "SF10BW125";
  }
  if(dr == lora::DR3) {
    return "SF9BW125";
  }
  if(dr == lora::DR4) {
    return "SF8BW125";
  }
  if(dr == lora::DR5) {
    return "SF7BW125";
  }
  if(dr == lora::DR6) {
    return "SF7BW250";
  }
  if(dr == lora::DR8) {
    return "SF12BW500";
  }
  if(dr == lora::DR9) {
    return "SF11BW500";
  }
  if(dr == lora::DR10) {
    return "SF10BW500";
  }
  if(dr == lora::DR11) {
    return "SF9BW500";
  }
  if(dr == lora::DR12) {
    return "SF8BW500";
  }
  if(dr == lora::DR13) {
    return "SF7BW500";
  }

  return "SF12BW125";
}

std::string ProtoHandler::codingRateToString(lora::CodingRate cr)
{
  if(cr == lora::CR4_5) {
    return "4/5";
  }
  if(cr == lora::CR4_6) {
    return "4/6";
  }
  if(cr == lora::CR4_7) {
    return "4/7";
  }
  if(cr == lora::CR4_8) {
    return "4/8";
  }

  return "4/5";
}

lora::CodingRate ProtoHandler::codingRateFromString(const std::string &cr)
{
  if(cr == "4/5") {
    return lora::CR4_5;
  }
  else if(cr == "4/6") {
    return lora::CR4_6;
  }
  else if(cr == "4/7") {
    return lora::CR4_7;
  }
  else if(cr == "4/8") {
    return lora::CR4_8;
  }

  return lora::CR4_5;
}

lora::DataRate ProtoHandler::dataRateFromString(const std::string &dr)
{
  if(dr == "SF12BW125") {
    return lora::DR0;
  }
  else if(dr == "SF11BW125") {
    return lora::DR1;
  }
  else if(dr == "SF10BW125") {
    return lora::DR2;
  }
  else if(dr == "SF9BW125") {
    return lora::DR3;
  }
  else if(dr == "SF8BW125") {
    return lora::DR4;
  }
  else if(dr == "SF7BW125") {
    return lora::DR5;
  }
  else if(dr == "SF7BW250") {
    return lora::DR6;
  }
  else if(dr == "SF12BW500") {
    return lora::DR8;
  }
  else if(dr == "SF11BW500") {
    return lora::DR9;
  }
  else if(dr == "SF10BW500") {
    return lora::DR10;
  }
  else if(dr == "SF9BW500") {
    return lora::DR11;
  }
  else if(dr == "SF8BW500") {
    return lora::DR12;
  }
  else if(dr == "SF7BW500") {
    return lora::DR13;
  }

  return lora::DR0;
}

bool
ProtoHandler::messageReceived(udp::UdpServer *srv, const udp::Bytearray &msg, const udp::NetworkAddress &src)
{
  pjson::document doc;
  char message[msg.size()];
  lora::Bytearray loraData;
  int version = msg[0];
  int16_t token = lora::extract_int(&msg[1], 2);
  Command cmd = (Command)msg[3];

  if(version != 0x02) {
    m_handler->log(lora::ERR, "ST: Unknown protocol version!");
    return false;
  }

  if(msg.size() > 12) {
    memcpy(message, &msg[12], msg.size()-12);
    message[msg.size()-12] = 0;
    m_handler->logf(lora::DEBUG, "ST: Got json packet from %s: %s\n", src.address().c_str(), message);
    if(!doc.deserialize_in_place(message)) {
      m_handler->logf(lora::ERR, "ST: Invalid json packet received: %s", message);
      return false;
    }
  }

  switch(cmd) {
  case PUSH_DATA:
    {

    udp::Bytearray response;
    response.push_back(version);
    lora::concat_int(response, token, 2);
    response.push_back(PUSH_ACK);
    srv->sendTo(response, src); // Push ack response

    if(doc.is_object() && doc.has_key("rxpk")) {
      auto &rxpktArray = doc["rxpk"];

      if(!rxpktArray.is_array()) {
        break;
      }

      for(unsigned int i = 0; i < rxpktArray.size(); ++ i) {
        auto &rxpkt = rxpktArray[i];

        if(!rxpkt.is_object()) {
          continue;
        }

        if(rxpkt.has_key("data")) {
          lora::MediaParameters params;
          if(rxpkt.has_key("tmst")) {
            params.rxTimestampUs = rxpkt["tmst"].as_int64();
          }
          if(rxpkt.has_key("freq")) {
            std::string f = rxpkt["freq"].as_string_ptr();
            f.erase(f.find('.'), 1);
            params.frequency = strtol(f.c_str(), NULL, 10);
          }
          if(rxpkt.has_key("chan")) {
            params.interface = rxpkt["chan"].as_int32();
          }
          if(rxpkt.has_key("rfch")) {
            params.rfChain = rxpkt["rfch"].as_int32();
          }
          if(rxpkt.has_key("crc")) {
            params.crc = rxpkt["crc"].as_int32(); // CRC status: 1 = OK, -1 = fail, 0 = no CRC
          }
          if(rxpkt.has_key("modu")) {
            std::string modu = rxpkt["modu"].as_string();
            if(modu == "LORA") {
              params.modulation = lora::LORA; // Modulation identifier "LORA" or "FSK"
            }
            else if(modu == "FSK") {
              params.modulation = lora::FSK;
            }
          }

          if(rxpkt.has_key("datr")) {
            if(params.modulation == lora::LORA) {
                params.dataRate = dataRateFromString(rxpkt["datr"].as_string());
            }
            else {
              params.fskDataRate = rxpkt["datr"].as_int32();
            }
          }

          if(rxpkt.has_key("codr")) {
            params.codingRate = codingRateFromString(rxpkt["codr"].as_string());
          }

          if(rxpkt.has_key("rssi")) {
            params.rssi = rxpkt["rssi"].as_int32();
          }
          if(rxpkt.has_key("lsnr")) {
            params.lsnr = rxpkt["lsnr"].as_int32();
          }

          GatewayId id(&msg[4]);
          const char *data = rxpkt["data"].as_string_ptr();
          base64Decode(loraData, data);
          try {
            dataReceived(id, loraData, params);
          }
          catch(std::exception &e) {
            m_handler->logf(lora::ERR, "ST: %s", e.what());
          }
        }
      }
    }

    if(doc.is_object() && doc.has_key("stat")) {
      auto &stat = doc["stat"];
      AppHandler *handler = dynamic_cast<AppHandler*>(m_handler);
      if(handler) {
        StatsMessage stats;
        stats.time = stat["time"].as_string();
        if(stat.has_key("lati")) {
          stats.latitude = stat["lati"].as_float();
        }
        if(stat.has_key("long")) {
          stats.longitude = stat["long"].as_float();
        }
        if(stat.has_key("alti")) {
          stats.altitude = stat["alti"].as_int32();
        }
        stats.rxnb = stat["rxnb"].as_int32();
        stats.rxok = stat["rxok"].as_int32();
        stats.rxfw = stat["rxfw"].as_int32();
        stats.ackr = stat["ackr"].as_int32();
        stats.dwnb = stat["dwnb"].as_int32();
        stats.txnb = stat["txnb"].as_int32();
        handler->processStats(stats);
      }
    }

    }
    break;
  case PULL_DATA:
    {

    if(!m_outboundServer) {
      m_outboundServer = srv;
    }

    GatewayId id(&msg[4]);

    if(m_gatewayAddress.find(id.toString()) == m_gatewayAddress.end()) {
      addGateway(&id);
    }

    m_gatewayAddress[id.toString()] = src;

    udp::Bytearray response;
    response.push_back(version);
    lora::concat_int(response, token, 2);
    response.push_back(PULL_ACK);
    srv->sendTo(response, src); // Push ack response

    }
  case TX_ACK:
    if(doc.is_object()) {
      std::string ack = doc["txpk_ack"]["error"].as_string_ptr();

      if(ack == "COLLISION_PACKET" ||
         ack == "COLLISION_BEACON") {
        LoraData &d = m_downlinkPackets[token];
        d.currentShift += 50000;

        if(d.currentShift < 800000) {
          sendGatewayData(d, token);
          break;
        }
      }
    }
    m_downlinkPackets.erase(token);
    break;

  default:
    break;
  }

  return true;
}

ProtoHandler::ProtoHandler(lora::AppHandler *handler)
  : lora::Handler(handler)
  , m_outboundServer(NULL)
  , m_handler(handler)
{
}

void
ProtoHandler::sendData(const lora::GatewayId &id, lora::Bytearray &data, const lora::MediaSettings &s)
{
  LoraData downlink;
  downlink.gateway = dynamic_cast<const GatewayId&>(id);
  downlink.data = data;
  downlink.settings = s;
  downlink.currentShift = 0;
  uint16_t token = rand() % UINT16_MAX;

  if(s.txTimestampUs != 0) {
    m_downlinkPackets.emplace(std::make_pair(token, downlink));
  }

  sendGatewayData(downlink, token);
}

void
ProtoHandler::sendGatewayData(const LoraData &downlink, uint16_t token)
{
  char freq[128];
  std::string jsonPacket;
  const lora::GatewayId &id = dynamic_cast<const lora::GatewayId&>(downlink.gateway);
  const lora::Bytearray &data = downlink.data;
  const lora::MediaSettings &s = downlink.settings;
  uint32_t shift = downlink.currentShift;

  if(m_gatewayAddress.find(id.toString()) == m_gatewayAddress.end()) {
    return;
  }

  udp::NetworkAddress &addr = m_gatewayAddress[id.toString()];

  jsonPacket = "{\"txpk\":{";

  snprintf(freq, sizeof(freq), "%d.%.06d", s.frequency/1000000, s.frequency%1000000);
  jsonPacket += "\"freq\":";
  jsonPacket += freq;
  jsonPacket += ",";

  jsonPacket += "\"ipol\":";
  jsonPacket += s.polarity == lora::NORMAL ? "false" : "true";
  jsonPacket += ",";

  jsonPacket += "\"rfch\":";
  jsonPacket += std::to_string(s.rfChain);
  jsonPacket += ",";

  if(s.power > 0) {
    jsonPacket += "\"powe\":";
    jsonPacket += std::to_string(s.power);
    jsonPacket += ",";
  }

  jsonPacket += "\"modu\":\"";
  jsonPacket += s.modulation == lora::LORA ? "LORA" : "FSK";
  jsonPacket += "\",";

  if(s.modulation == lora::LORA) {
    jsonPacket += "\"datr\":\"";
    jsonPacket += dataRateToString(s.dataRate);
    jsonPacket += "\",";
  }
  else {
    jsonPacket += "\"datr\":";
    jsonPacket += std::to_string(s.fskDataRate);
    jsonPacket += ",";
  }

  if(s.modulation == lora::LORA) {
    jsonPacket += "\"codr\":\"";
    jsonPacket += codingRateToString(s.codingRate);
    jsonPacket += "\",";
  }
  else {
    jsonPacket += "\"fdev\":";
    jsonPacket += std::to_string(s.fDeviation);
    jsonPacket += ",";
  }

  if(s.preambleSize > 0) {
    jsonPacket += "\"prea\":";
    jsonPacket += std::to_string(s.preambleSize);
    jsonPacket += ",";
  }

  if(s.disableCrc) {
    jsonPacket += "\"ncrc\":true,";
  }

  if(s.txTimestampUs > 0) {
    jsonPacket += "\"tmst\":";
    jsonPacket += std::to_string(s.txTimestampUs + shift);
    jsonPacket += ",";
  }
  else {
    jsonPacket += "\"imme\":true,";
  }

  jsonPacket += "\"size\":";
  jsonPacket += std::to_string(data.size());
  jsonPacket += ",";

  std::string b64data;
  base64Encode(b64data, data);

  jsonPacket += "\"data\":\"";
  jsonPacket += b64data;
  jsonPacket += "\"}}";

  std::cout << "Sending json packet to " << addr.address() << ": " << jsonPacket << std::endl;
  udp::Bytearray bytes;
  bytes.push_back(0x02);
  lora::concat_int(bytes, token, 2);
  bytes.push_back(PULL_RESP);
  bytes.insert(bytes.end(), jsonPacket.begin(), jsonPacket.end());

  m_outboundServer->sendTo(bytes, addr);
}

}
