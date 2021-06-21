/***************************************************
 * handler.cpp
 * Created on Thu, 16 Jul 2020 10:09:55 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <string.h>
#include "apphandler.h"
#include "pyconn.h"

AppHandler::AppHandler(const std::string &configScript, const std::string &base)
  : m_parserBase(base)
  , m_networkId(0)
  , m_configScript(configScript)
{
  setDataSendCallback(
    [this](uint32_t net, uint32_t dev, int port, const char* data, int size, int confirm)
    {
      printf("Sending data to %d:%d\n", net, dev);
      lora::Bytearray ld(data, data + size);
      auto sessions = this->findDeviceSession(dev, net);

      if(!sessions.empty()) {
        this->send(sessions.front(), port, ld, (bool)confirm);
      }
      else {
        std::cout << "Device session not found!" << std::endl;
      }
    });

  const char *pypath = getenv("PYTHONPATH");
  if(m_parserBase != "") {
    setenv("PYTHONPATH", ((pypath?(std::string(pypath) + ":"):std::string("")) + m_parserBase).c_str(), 1);
  }

  initPythonModule(m_configScript);
}

AppHandler::~AppHandler() {
  setDataSendCallback([] (uint32_t net, uint32_t dev, int port, const char* data, int size, int confirm) { });
  finiPythonModule();
}

void
AppHandler::getDeviceFromScript(const lora::EUI &devEUI, const lora::EUI &appEUI) {
  std::string de = lora::hexToString(devEUI, sizeof(devEUI));
  std::string ae = lora::hexToString(appEUI, sizeof(appEUI));

  auto res = getDeviceInfo(m_configScript, de, ae);

  uint32_t nwkId, devAddr;

  for (auto &devInfo : res) {
    m_info[de+ae] = devInfo;
    m_info[de+ae].session = NULL;

    if (devInfo.session) {
      nwkId = devInfo.session->networkId;
      devAddr = devInfo.session->deviceAddr;

      auto sessions = findDeviceSession(devAddr, nwkId);
      if (!sessions.empty()) {
        m_info[de+ae].session = sessions.front();
      }

      delete devInfo.session;
    }

    if (!m_info[de+ae].session) {
      lora::DeviceSession sess;
      sess.device = &m_info[de+ae];
      sess.networkId = m_networkId;
      sess.rxDelay1 = 1;
      sess.joinAckDelay1 = 5;
      sess.rx2Channel  = 0;
      sess.rx2Datarate = lora::DR0;
      uint64_t addr;

      do {
        sess.deviceAddr = random() & 0x00ffffff;
        addr = (uint64_t)sess.networkId << 32;
        addr |= sess.deviceAddr;
      }
      while(m_session.find(addr) != m_session.end());

      std::cout << "Created session: " << sess.nOnce << ", " << sess.devNOnce << std::endl;

      m_session[addr] = sess;
      m_info[de+ae].session = &m_session[addr];
    }
  }
}

std::list<lora::DeviceInfo *>
AppHandler::findDeviceInfo(const lora::EUI &devEUI, const lora::EUI &appEUI) {
  std::list<lora::DeviceInfo *> lst;

  std::string de = lora::hexToString(devEUI, sizeof(devEUI));
  std::string ae = lora::hexToString(appEUI, sizeof(appEUI));

  if(m_info.find(de+ae) == m_info.end()) {
    getDeviceFromScript(devEUI, appEUI);
  }

  if(m_info.find(de+ae) != m_info.end()) {
    lst.push_back(&m_info[de+ae]);
  }

  return lst;
}

void
AppHandler::getSessionFromScript(uint32_t deviceAddr, uint32_t networkAddr) {
  auto res = ::getDeviceSession(m_configScript, deviceAddr, networkAddr);

  for(auto &sess : res) {
    uint64_t addr = sess.networkId;
    addr <<= 32;
    addr |= sess.deviceAddr;

    if(!sess.device) {
      printf("Warning: dangling session with no device!\n");
      continue;
    }

    m_session[addr] = sess;
    m_sessionByDevAddr.insert(std::make_pair(deviceAddr, &m_session[addr]));

    auto devs = findDeviceInfo(sess.device->devEUI, sess.device->appEUI);

    if(!devs.empty()) {
      m_session[addr].device = devs.front();
      if(devs.front()->session && devs.front()->session != &m_session[addr]) {
        eraseDeviceSession(devs.front()->session);
      }
      devs.front()->session = &m_session[addr];
    }

    delete sess.device;
  }
}

void
AppHandler::eraseDeviceSession(lora::DeviceSession *sess) {
  int64_t addr = sess->networkId;
  addr <<= 32;
  addr |= sess->deviceAddr;

  if(&m_session[addr] != sess) {
    return;
  }

  auto it = m_sessionByDevAddr.lower_bound(sess->deviceAddr);
  auto up = m_sessionByDevAddr.upper_bound(sess->deviceAddr);

  for(; it != up; ++ it) {
    if(it->second == sess) {
      m_sessionByDevAddr.erase(it);
      break;
    }
  }

  m_session.erase(addr);
}

std::list<lora::DeviceSession *>
AppHandler::getDeviceSessions() {
  std::list<lora::DeviceSession *> sessions;

  for(auto &it : m_session) {
    sessions.push_back(&it.second);
  }

  return sessions;
}

std::list<lora::DeviceSession *>
AppHandler::findDeviceSession(uint32_t deviceAddr, uint32_t networkAddr) {
  uint64_t addr = networkAddr;
  addr <<= 32;
  addr |= deviceAddr;

  std::list<lora::DeviceSession *> lst;
  if(networkAddr == 0) {
    if(m_sessionByDevAddr.lower_bound(deviceAddr) == m_sessionByDevAddr.upper_bound(deviceAddr)) {
      getSessionFromScript(deviceAddr, 0);
    }

    auto it = m_sessionByDevAddr.lower_bound(deviceAddr);
    auto up = m_sessionByDevAddr.upper_bound(deviceAddr);

    for(; it != up; ++ it) {
      lst.push_back(it->second);
    }

    return lst;
  }

  if(m_session.find(addr) == m_session.end()) {
    getSessionFromScript(deviceAddr, networkAddr);
  }

  if(m_session.find(addr) != m_session.end()) {
    lst.push_back(&m_session[addr]);
  }

  return lst;
}

void
AppHandler::saveDeviceSession(const lora::DeviceSession *s) {
  if(!s->device) {
    throw std::runtime_error("Orphan session detected!");
  }

  ::saveDeviceSession(m_configScript, s);
}

void
AppHandler::processAppData(lora::DeviceSession *s, int port, const lora::Bytearray &data) {
  printf("Got device data on port %d: [", port);

  for(auto byte : data) {
    printf("0x%02x ", byte);
  }

  printf("]\n");

  ::processAppData(m_configScript, s, port, (const char*)data.data(), data.size());
}

void
AppHandler::deviceStatus(lora::DeviceSession *s, int batLevel, int margin)
{
  std::cout << __FUNCTION__ << std::endl;
}

void
AppHandler::send(lora::DeviceSession *s, int port, lora::Bytearray data, bool confirm) {
  printf("Sending device data on port %d: [", port);

  for(auto byte : data) {
    printf("0x%02x ", byte);
  }

  printf("]\n");

  m_sessionSendData[(void*)s].emplace_back(port, data, confirm, s);
}

void
AppHandler::processStats(const semtech::StatsMessage &) {
  std::cout << __FUNCTION__ << std::endl;
}

lora::SendDataList &
AppHandler::getDeviceSendQueue(const lora::DeviceSession *s) {
  static lora::SendDataList data;

  return m_sessionSendData[(void*)s];
}

void
AppHandler::log(int level, const std::string &line) {
  std::cerr << line << std::endl;
}

