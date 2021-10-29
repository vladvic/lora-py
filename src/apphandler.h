/***************************************************
 * handler.h
 * Created on Thu, 16 Jul 2020 10:08:10 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <map>
#include <string>
#include <iostream>
#include "lora/types.h"
#include "semtech/proto.h"

class AppHandler 
  : public lora::AppHandler {
public:
  AppHandler(const std::string &configScript = "config", const std::string &base = "handlers");
  ~AppHandler();
  virtual std::list<lora::DeviceInfo *>
  findDeviceInfo(const lora::EUI &devEUI, const lora::EUI &appEUI);
  virtual std::list<lora::DeviceSession *>
  findDeviceSession(uint32_t networkAddr, uint32_t deviceAddr);
  virtual std::list<lora::DeviceSession *>
  getDeviceSessions();
  virtual void
  saveDeviceSession(const lora::DeviceSession *);
  virtual void
  processAppData(lora::DeviceSession *s, int port, const lora::Bytearray &data);
  virtual void
  processStats(const semtech::StatsMessage &);
  virtual lora::SendDataList &
  getDeviceSendQueue(const lora::DeviceSession *);
  virtual void
  deviceStatus(lora::DeviceSession *s, int batLevel, int margin);
  virtual void
  log(int level, const std::string &line);
  virtual void
  logf(int level, const std::string &format, ...);
  void send(lora::DeviceSession *session, int port, lora::Bytearray data, bool confirm);

private:
  void
  getDeviceFromScript(const lora::EUI &devEUI, const lora::EUI &appEUI);
  void
  getSessionFromScript(uint32_t networkAddr, uint32_t deviceAddr);
  void
  eraseDeviceSession(lora::DeviceSession *sess);

  std::map<std::string, lora::DeviceInfo> m_info;
  std::map<uint64_t, lora::DeviceSession> m_session;
  std::multimap<uint64_t, lora::DeviceSession*> m_sessionByDevAddr;
  std::map<void*, lora::SendDataList> m_sessionSendData;
  std::map<void*, std::string> m_deviceHandler;
  std::string m_parserBase;
  uint32_t m_networkId;
  std::string m_configScript;

  char *m_formatted;
  size_t m_formattedSize;
};

