/***************************************************
 * aes.cpp
 * Created on Mon, 13 Jul 2020 07:15:20 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

extern "C" {
#include "util/aes/aes.h"
#include "util/aes/cmac.h"
}
#include "util/concat.h"
#include "types.h"
#include "aes.h"

namespace lora {

Bytearray aes128_encrypt(const Key &key, const Bytearray &plain) {
  struct AES_ctx ctx;
  Bytearray result(plain);
  unsigned int length = ((plain.size() + 15) / 16) * 16;

  result.reserve(length);
  while(result.size() < length) {
    result.push_back(0);
  }

  AES_init_ctx(&ctx, key);

  for(unsigned int i = 0; i < length / 16; i ++) {
    AES_ECB_encrypt(&ctx, &result[i * 16]);
  }

  return result;
}

Bytearray aes128_decrypt(const Key &key, const Bytearray &cypher) {
  struct AES_ctx ctx;
  Bytearray result(cypher);
  unsigned int length = ((cypher.size() + 15) / 16) * 16;

  result.reserve(length);
  while(result.size() < length) {
    result.push_back(0);
  }

  AES_init_ctx(&ctx, key);

  for(unsigned int i = 0; i < length / 16; i ++) {
    AES_ECB_decrypt(&ctx, &result[i * 16]);
  }

  return result;
}

Bytearray aes128_cmac(const Key &key, const Bytearray &msg) {
  struct AES_ctx ctx;
  Bytearray cmac(16);

  AES_init_ctx(&ctx, key);
  AES_ECB_CMAC(&ctx, msg.data(), msg.size(), cmac.data());
  return cmac;
}

Bytearray loramac_decrypt(const Key &key, const Bytearray &payload, 
                          uint32_t fcnt, uint32_t dev_addr, int direction)
{
  Bytearray aBlock = concat((uint8_t)1, (uint32_t)0, (uint8_t)direction, dev_addr, fcnt, (uint16_t)0);
  Bytearray encrypted(payload), sBlock;
  uint16_t ctr = 1, bn = 0;
  long size = payload.size();

  // complete blocks
  while(size >= 16) {
    aBlock[15] = ctr & 0xFF;
    ctr ++;
    sBlock = aes128_encrypt(key, aBlock);
    for(unsigned int i = 0; i < aBlock.size(); ++ i) {
      encrypted[bn + i] ^= sBlock[i];
    }

    size -= 16;
    bn += 16;
  }

  // partial blocks
  if(size > 0) {
    aBlock[15] = ctr & 0xFF;
    sBlock = aes128_encrypt(key, aBlock);
    for(unsigned int i = 0; i < size; ++ i) {
      encrypted[bn + i] ^= sBlock[i];
    }
  }

  return encrypted;
}

}

#ifdef TEST_AES
#include <iostream>
#include <stdio.h>

int main() {
  lora::Key key = {0x2b, 0x7e, 0x15, 0x16 , 0x28, 0xae, 0xd2, 0xa6 , 0xab, 0xf7, 0x15, 0x88 , 0x09, 0xcf, 0x4f, 0x3c};
  lora::Bytearray cmac;
  uint8_t msg[] = "Hello world! I'm so glad to see you!";

  cmac = lora::aes128_cmac(key, lora::Bytearray(msg, msg + sizeof(msg) - 1));
  for(int i = 0; i < cmac.size(); ++ i) {
    printf("%0.2x", cmac[i]);
  }
  printf("\n");
}
#endif
