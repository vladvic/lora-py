/***************************************************
 * joinaccept.h
 * Created on Fri, 17 Jul 2020 03:37:24 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include "../message.h"
#include <vector>
#include <stdint.h>

namespace lora {

//typedef uint32_t CFListEntry;
struct CFListEntry {
  uint32_t frequency;
} __attribute__((__packed__));

class JoinAccept;
class CFListPropertySetter {
public:
  operator uint32_t();
  uint32_t operator =(uint32_t v);

  CFListPropertySetter(JoinAccept *m, int idx);
  ~CFListPropertySetter() = default;

private:
  JoinAccept *m_joinAccept;
  int m_index;

  friend class JoinAccept;
};

typedef std::vector<CFListPropertySetter> CFList;

class JoinAccept
  : public Message
{
public:
  JoinAccept();
  virtual ~JoinAccept();

  uint32_t joinNOnce() const;
  uint32_t networkId() const;
  uint32_t devAddress() const;
  uint32_t dlSettings() const;
  uint32_t rxDelay() const;
  const CFList &cfList() const;

  void setJoinNOnce(uint32_t);
  void setNetworkId(uint32_t);
  void setDevAddress(uint32_t);
  void setDlSettings(uint32_t);
  void setRxDelay(uint32_t);
  void setCFList(const std::vector<uint32_t> &list);
  CFList &cfList();

  virtual
  void compose(const Device *);
  virtual
  void parse(const Device *);
  void setCFListEntry(int idx, uint32_t value);

private:
  uint32_t m_joinNOnce;
  uint32_t m_networkId;
  uint32_t m_devAddress;
  uint32_t m_dlSettings;
  uint32_t m_rxDelay;
  CFListEntry m_cfList[5];
  CFList m_cfListProperty;

  typedef Message::Registrar<JOIN_ACCEPT, JoinAccept> Registrar;
  static Registrar s_registrar;

  friend class CFListPropertySetter;
};

}
