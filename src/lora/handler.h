/***************************************************
 * handler.h
 * Created on Fri, 10 Jul 2020 04:27:45 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <list>
#include <memory>
#include "types.h"
#include "message.h"
#include "net/udpserver.h"

namespace lora {

class Message;

enum LogLevel {
  CRIT = 0,
  ERR,
  WARN,
  INFO,
  DEBUG,
};

struct SendData {
  int port;
  Bytearray data;
  bool confirmed;
  DeviceSession *session;
  int transmitted;
  SendData(int port, const Bytearray &data, bool confirm, DeviceSession *s);
};

struct SendDataItem {
  int port;
  uint32_t sendBefore;
  std::string gatewayId;
  DeviceSession *session;
  MediaSettings settings;
  Bytearray data;
  Bytearray opts;
  uint8_t ctrl;
  bool confirmed;
};

typedef std::list<SendData> SendDataList;

class AppHandler {
public:
  virtual std::list<DeviceInfo *>
  findDeviceInfo(const EUI &devEUI, const EUI &appEUI) = 0;
  virtual std::list<DeviceSession *>
  findDeviceSession(uint32_t deviceAddr, uint32_t networkAddr = 0) = 0;
  virtual std::list<DeviceSession *>
  getDeviceSessions() = 0;
  virtual void
  saveDeviceSession(const DeviceSession *) = 0;
  virtual SendDataList &
  getDeviceSendQueue(const DeviceSession *) = 0;
  virtual void
  processAppData(DeviceSession *s, int port, const Bytearray &data) = 0;
  virtual void
  deviceStatus(DeviceSession *s, int batLevel, int margin) = 0;
  virtual void
  log(int level, const std::string &line) = 0;
  virtual void
  logf(int level, const std::string &line, ...) = 0;
};

class Handler {
public:
  Handler(AppHandler *handler);
  virtual ~Handler() = default;

  void
  dataReceived(const GatewayId &id, Bytearray &data, const MediaParameters &params);
  void
  processSendQueue();
  virtual void
  sendData(const GatewayId &id, Bytearray &data, const MediaSettings &settings) = 0;
  void
  addGateway(GatewayId *gw);
  void
  removeGateway(GatewayId *gw);

private:
  void processClassCSendQueue();
  void processJoinRequest(const GatewayId &id, Message &msg, const MediaParameters &params);
  void processData(const GatewayId &id, Message &msg, const MediaParameters &params);
  void sendDeviceData(const SendDataItem &data);
  void processMacCommand(const GatewayId &id, DeviceSession *session, const MediaParameters &params, const Bytearray &command);

  AppHandler *m_handler;
  std::list<SendDataItem*> m_sendQueue;
  std::list<MacCommand> m_commands;
  std::map<std::string, std::unique_ptr<GatewayId>> m_gateways;
  size_t m_lastPacketTime;
};

}
