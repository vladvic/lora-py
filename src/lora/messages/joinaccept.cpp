/***************************************************
 * joinaccept.cpp
 * Created on Fri, 17 Jul 2020 06:17:26 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <iostream>
#include <assert.h>
#include "../aes.h"
#include "../util/concat.h"
#include "joinaccept.h"

namespace lora {

CFListPropertySetter::CFListPropertySetter(JoinAccept *m, int idx)
  : m_joinAccept(m)
  , m_index(idx)
{
  assert(m_joinAccept);
}

CFListPropertySetter::operator uint32_t()
{
  return m_joinAccept->m_cfList[m_index].frequency * 100;
}

uint32_t CFListPropertySetter::operator =(uint32_t v)
{
  m_joinAccept->setCFListEntry(m_index, v);
  return v;
}

JoinAccept::JoinAccept()
  : m_joinNOnce(0)
  , m_networkId(0)
  , m_devAddress(0)
  , m_dlSettings(0)
  , m_rxDelay(0)
{
  for(unsigned int i = 0; i < sizeof(m_cfList) / sizeof(CFListEntry); ++ i) {
    m_cfList[i].frequency = 0;
    m_cfListProperty.emplace_back(this, i);
  }
}

JoinAccept::~JoinAccept()
{
  m_cfListProperty.clear();
}

uint32_t JoinAccept::joinNOnce() const
{
  return m_joinNOnce;
}

uint32_t JoinAccept::networkId() const
{
  return m_networkId;
}

uint32_t JoinAccept::devAddress() const
{
  return m_devAddress;
}

uint32_t JoinAccept::dlSettings() const
{
  return m_dlSettings;
}

uint32_t JoinAccept::rxDelay() const
{
  return m_rxDelay;
}

const CFList &JoinAccept::cfList() const
{
  return m_cfListProperty;
}


void JoinAccept::setJoinNOnce(uint32_t v)
{
  m_joinNOnce = v;
  m_payload.clear();
}

void JoinAccept::setNetworkId(uint32_t v)
{
  m_networkId = v;
  m_payload.clear();
}

void JoinAccept::setDevAddress(uint32_t v)
{
  m_devAddress = v;
  m_payload.clear();
}

void JoinAccept::setDlSettings(uint32_t v)
{
  m_dlSettings = v;
  m_payload.clear();
}

void JoinAccept::setRxDelay(uint32_t v)
{
  m_rxDelay = v;
  m_payload.clear();
}

void JoinAccept::setCFList(const std::vector<uint32_t> &list)
{
  for(unsigned int i = 0; i < m_cfListProperty.size(); ++ i) {
    if(i >= list.size()) {
      break;
    }
    m_cfListProperty[i] = list[i];
  }
}

CFList &JoinAccept::cfList()
{
  return m_cfListProperty;
}

void JoinAccept::compose(const Device *dev)
{
  lora::Bytearray payload;
  m_payload.clear();
  m_payload.push_back(JOIN_ACCEPT << 5);
  concat_int(payload, m_joinNOnce, 3);
  concat_int(payload, m_networkId, 3);
  concat_int(payload, m_devAddress, 4);
  payload.push_back(m_dlSettings);
  payload.push_back(m_rxDelay);

  int cfListSize = 0;
  for(unsigned int i = 0; i < sizeof(m_cfList) / sizeof(CFListEntry); ++ i) {
    if(m_cfList[i].frequency == 0) {
      break;
    }
    ++ cfListSize;
    concat_int(payload, m_cfList[i].frequency, 3);
  }

  int paddingSize = (16 - (cfListSize * 3 % 16)) % 16;

  for(int i = 0; i < paddingSize; ++ i) {
    payload.push_back(0);
  }

  const DeviceInfo *info = dev->info;

  if(!info) {
    throw std::runtime_error("JoinAccept: Couldn't compose packet: device info not found!");
  }

  concat_bytearray(m_payload, payload);
  Bytearray mic = aes128_cmac(info->appKey, m_payload);
  mic.resize(4);
  concat_bytearray(payload, mic);

  m_payload.resize(1);
  concat_bytearray(m_payload, aes128_decrypt(info->appKey, payload));
}

void JoinAccept::parse(const Device *dev)
{
  lora::Bytearray payload(m_payload.begin() + 1, m_payload.end());
  const DeviceInfo *info = dev->info;

  if(!info) {
    throw std::runtime_error("JoinAccept: Couldn't parse packet: device info not found!");
  }

  payload = aes128_encrypt(info->appKey, payload);

  m_joinNOnce = extract_int(&payload[1], 3);
  m_networkId = extract_int(&payload[4], 3);
  m_devAddress = extract_int(&payload[7], 4);
  m_dlSettings = payload[11];
  m_rxDelay = payload[12];

  for(unsigned int i = 13; i < m_payload.size() - 6; i += 3) {
    m_cfList[(i - 13) / 3].frequency = extract_int(&payload[i], 3);
  }
}

void JoinAccept::setCFListEntry(int idx, uint32_t value)
{
  m_cfList[idx].frequency = value;
  m_payload.clear();
}


}
