/***************************************************
 * joinrequest.cpp
 * Created on Wed, 15 Jul 2020 08:34:38 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <iostream>
#include <string.h>
#include "../types.h"
#include "../util/concat.h"
#include "../aes.h"
#include "data.h"

namespace lora {

DataUp::DataUp()
  : m_devAddr(0)
  , m_ctrl(0)
  , m_port(0)
  , m_cnt(0)
{
}

uint32_t DataUp::devAddress() const
{
  return m_devAddr;
}

uint8_t  DataUp::ctrl() const
{
  return  m_ctrl;
}

uint16_t DataUp::cnt() const
{
  return m_cnt;
}

const Bytearray &DataUp::opts() const
{
  return m_opts;
}

uint8_t DataUp::port() const
{
  return m_port;
}

const Bytearray &DataUp::data() const
{
  return m_data;
}

void DataUp::setDevAddress(uint32_t value)
{
  m_devAddr = value;
  m_payload.clear();
}

void DataUp::setCtrl(uint8_t ctrl)
{
  m_ctrl = ctrl;
  m_payload.clear();
}

void DataUp::setCnt(uint16_t cnt)
{
  m_cnt = cnt;
  m_payload.clear();
}

void DataUp::setOpts(const Bytearray &opts)
{
  m_opts = opts;
  m_optsEncrypted.clear();
  m_payload.clear();
}

void DataUp::setPort(uint8_t port)
{
  m_port = port;
  m_payload.clear();
}

void DataUp::setData(const Bytearray &data)
{
  m_data = data;
  m_dataEncrypted.clear();
  m_payload.clear();
}

uint32_t DataUp::calcMic()
{
  const DeviceSession *session = getDevice()->session;
  Bytearray msg;

  // Block B0
  composeMsg(msg, getDevice());
  uint32_t cnt  = m_cnt & 0xffff;

  Bytearray b0 = concat((uint8_t)0x49, (uint16_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)getDirection(), 
                        (uint32_t)m_devAddr, (uint32_t)cnt, (uint8_t)0, (uint8_t)(msg.size()));

  b0.insert(b0.end(), msg.begin(), msg.end());
  Bytearray cmac = aes128_cmac(session->sNwkSIntKey, b0);

  return extract_int(&cmac[0], 4);
}

void DataUp::composeMsg(Bytearray &msg, const Device *dev)
{
  const DeviceSession *session = dev->session;
  msg.push_back((uint8_t)type() << 5);

  if(m_optsEncrypted.empty()) {
    m_optsEncrypted = loramac_decrypt(session->nwkSEncKey, m_opts, m_cnt, m_devAddr, getDirection());
  }
  if(m_dataEncrypted.empty()) {
    if(m_port == 0) {
      m_dataEncrypted = loramac_decrypt(session->nwkSEncKey, m_data, m_cnt, m_devAddr, getDirection());
    }
    else {
      m_dataEncrypted = loramac_decrypt(session->appSKey, m_data, m_cnt, m_devAddr, getDirection());
    }
  }

  unsigned int optLen = m_optsEncrypted.size() & 0x0f;
  concat_recursive(msg, m_devAddr, (uint8_t)((m_ctrl & 0xf0) | optLen), (uint16_t)m_cnt);
  concat_recursive(msg, m_optsEncrypted);

  if(m_dataEncrypted.size() > 0) {
    concat_recursive(msg, (uint8_t)m_port, m_dataEncrypted);
  }
}

void DataUp::compose(const Device *dev)
{
  m_payload.clear();
  composeMsg(m_payload, dev);
  uint32_t mic = this->calcMic();
  concat_recursive(m_payload, mic);
}

void DataUp::parse(const Device *dev)
{
  const DeviceSession *session = dev->session;
  m_devAddr = extract_int(&m_payload[1], 4);
  m_ctrl = m_payload[5];
  m_cnt = extract_int(&m_payload[6], 2);

  unsigned int optLen = m_ctrl & 0x0f;
  m_optsEncrypted = Bytearray(m_payload.begin() + 8, m_payload.begin() + 8 + optLen);

  if(session != NULL) {
    m_opts = loramac_decrypt(session->nwkSEncKey, m_optsEncrypted, m_cnt, m_devAddr, getDirection());
  }

  if(m_payload.size() <= optLen + 8 + 4/*mic*/) {
    return;
  }

  unsigned int dataStart = optLen + 8;
  m_port = m_payload[dataStart ++];
  m_dataEncrypted = Bytearray(m_payload.begin() + dataStart, m_payload.end());
  m_dataEncrypted.resize(m_dataEncrypted.size() - 4);

  if(session != NULL) {
    if(0 == m_port) {
      m_data = loramac_decrypt(session->nwkSEncKey, m_dataEncrypted, m_cnt, m_devAddr, getDirection());
    }
    else {
      m_data = loramac_decrypt(session->appSKey, m_dataEncrypted, m_cnt, m_devAddr, getDirection());
    }
  }
}

}

