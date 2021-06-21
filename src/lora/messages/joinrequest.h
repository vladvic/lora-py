/***************************************************
 * joinrequest.h
 * Created on Wed, 15 Jul 2020 08:34:08 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include "../message.h"

namespace lora {

class JoinRequest
  : public Message
{
public:
  const EUI &appEui() const;
  const EUI &devEui() const;
  uint32_t devNOnce() const;

  void setAppEui(EUI value);
  void setDevEui(EUI value);
  void setDevNOnce(uint32_t nonce);

private:
  virtual
  void compose(const Device *dev);
  virtual
  void parse(const Device *dev);

  EUI m_appEui;
  EUI m_devEui;
  uint32_t m_devNOnce;

  typedef Message::Registrar<JOIN_REQUEST, JoinRequest> Registrar;
  static Registrar s_registrar;
};

}
