/***************************************************
 * aes.h
 * Created on Mon, 13 Jul 2020 07:00:12 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include "types.h"

namespace lora {

Bytearray aes128_encrypt(const Key &key, const Bytearray &what);
Bytearray aes128_decrypt(const Key &key, const Bytearray &what);
Bytearray aes128_cmac(const Key &key, const Bytearray &what);
Bytearray loramac_decrypt(const Key &key, const Bytearray &payload, 
                          uint32_t fcnt, uint32_t dev_addr, int direction);

}

