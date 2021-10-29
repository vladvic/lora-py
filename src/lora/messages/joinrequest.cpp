/***************************************************
 * joinrequest.cpp
 * Created on Wed, 15 Jul 2020 08:34:38 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <string.h>
#include <algorithm>
#include "../util/concat.h"
#include "../aes.h"
#include "joinrequest.h"

namespace lora {

const EUI &JoinRequest::appEui() const
{
  return m_appEui;
}

const EUI &JoinRequest::devEui() const
{
  return m_devEui;
}

uint32_t JoinRequest::devNOnce() const
{
  return m_devNOnce;
}

void JoinRequest::setAppEui(EUI value)
{
  memcpy(m_appEui, value, sizeof(EUI));
  m_payload.clear();
}

void JoinRequest::setDevEui(EUI value)
{
  memcpy(m_devEui, value, sizeof(EUI));
  m_payload.clear();
}

void JoinRequest::setDevNOnce(uint32_t nonce)
{
  m_devNOnce = nonce;
  m_payload.clear();
}

void JoinRequest::compose(const Device *dev)
{
  const DeviceInfo *device = dev->info;

  if(!device) {
    throw std::runtime_error("JoinRequest: Couldn't parse packet: device info not found!");
  }

  Bytearray appEui(m_appEui, m_appEui + sizeof(m_appEui));
  Bytearray devEui(m_devEui, m_devEui + sizeof(m_devEui));
  std::reverse(appEui.begin(), appEui.end());
  std::reverse(devEui.begin(), devEui.end());
  m_payload.clear();
  m_payload.push_back(JOIN_REQUEST << 5);
  concat_recursive(m_payload, appEui, devEui);
  concat_int(m_payload, m_devNOnce, 2);
  Bytearray mic = aes128_cmac(device->appKey, m_payload);
  m_payload.insert(m_payload.end(), mic.begin(), mic.begin() + 4);
}

void JoinRequest::parse(const Device *dev)
{
  Bytearray appEui(&m_payload[1], &m_payload[1] + sizeof(m_appEui));
  Bytearray devEui(&m_payload[1 + sizeof(m_appEui)], &m_payload[1 + sizeof(m_appEui)] + sizeof(m_devEui));

  std::reverse(appEui.begin(), appEui.end());
  std::reverse(devEui.begin(), devEui.end());

  memcpy(m_appEui, &appEui[0], sizeof(m_appEui));
  memcpy(m_devEui, &devEui[0], sizeof(m_devEui));

  m_devNOnce = extract_int(&m_payload[1 + sizeof(m_appEui) * 2], 2);
}

}

