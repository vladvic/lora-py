/***************************************************
 * proto.h
 * Created on Thu, 09 Jul 2020 09:46:40 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <map>
#include "net/udpserver.h"
#include "lora/types.h"
#include "lora/handler.h"

namespace semtech {

enum Command {
  PUSH_DATA = 0,
  PUSH_ACK = 1,
  PULL_DATA = 2,
  PULL_RESP = 3,
  PULL_ACK = 4,
  TX_ACK = 5,
};

class GatewayId
  : public lora::GatewayId
{
public:
  GatewayId() = default;
  GatewayId(const unsigned char *id);
  virtual std::string toString() const;
  lora::GatewayId *clone() const;

private:
  std::string m_id;
};

struct GatewayInfo {
  struct timespec lastUpdateTime;
  udp::NetworkAddress address;
};

struct StatsMessage {
  std::string time; // | string | UTC 'system' time of the gateway, ISO 8601 'expanded' format
  double latitude; // | number | GPS latitude of the gateway in degree (float, N is +)
  double longitude; // | number | GPS latitude of the gateway in degree (float, E is +)
  int altitude; // | number | GPS altitude of the gateway in meter RX (integer)
  int rxnb; // | number | Number of radio packets received (unsigned integer)
  int rxok; // | number | Number of radio packets received with a valid PHY CRC
  int rxfw; // | number | Number of radio packets forwarded (unsigned integer)
  double ackr; // | number | Percentage of upstream datagrams that were acknowledged
  int dwnb; // | number | Number of downlink datagrams received (unsigned integer)
  int txnb; // | number | Number of packets emitted (unsigned integer)
};

class AppHandler
  : public lora::AppHandler
{
public:
  virtual void
  processStats(const StatsMessage &) = 0;
};

struct LoraData {
  GatewayId gateway;
  lora::Bytearray data;
  lora::MediaSettings settings;
  uint32_t sentTimestamp;
  uint32_t retries;
  uint32_t currentShift;
};

class ProtoHandler
  : public lora::Handler
  , public udp::UdpHandler
{
public:
  ProtoHandler(lora::AppHandler *handler);
  virtual ~ProtoHandler() = default;
  virtual void
  selectTimeout(udp::UdpServer *srv);
  virtual bool 
  messageReceived(udp::UdpServer *srv, const udp::Bytearray &msg, const udp::NetworkAddress &src);
  virtual void
  sendData(const lora::GatewayId &id, lora::Bytearray &data, const lora::MediaSettings &settings);

private:
  void sendGatewayData(const LoraData &downlink, uint16_t token);
  lora::DataRate dataRateFromString(const std::string &dr);
  lora::CodingRate codingRateFromString(const std::string &cr);
  std::string dataRateToString(lora::DataRate);
  std::string codingRateToString(lora::CodingRate);

  udp::UdpServer *m_outboundServer;
  lora::AppHandler *m_handler;
  std::map<std::string, udp::NetworkAddress> m_gatewayAddress;
  std::map<uint32_t, LoraData> m_downlinkPackets;
};

}
