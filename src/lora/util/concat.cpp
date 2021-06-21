/***************************************************
 * concat.cpp
 * Created on Tue, 14 Jul 2020 04:05:34 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include "concat.h"
#include <stdio.h>

namespace lora {

uint32_t extract_int(const uint8_t *from, int numBytes) {
  uint32_t res = 0;
  if(numBytes > 0) res |= from[0] << 8*0;
  if(numBytes > 1) res |= from[1] << 8*1;
  if(numBytes > 2) res |= from[2] << 8*2;
  if(numBytes > 3) res |= from[3] << 8*3;
  return res;
}

void concat_int(Bytearray &bytes, uint32_t num, int numBytes) {
  if(numBytes > 0) bytes.push_back((uint8_t)((num >> 0*8)&0xff));
  if(numBytes > 1) bytes.push_back((uint8_t)((num >> 1*8)&0xff));
  if(numBytes > 2) bytes.push_back((uint8_t)((num >> 2*8)&0xff));
  if(numBytes > 3) bytes.push_back((uint8_t)((num >> 3*8)&0xff));
}

template<>
void concat_recursive(Bytearray &b, uint8_t n) {
  b.push_back(n);
}

template<>
void concat_recursive(Bytearray &b, uint16_t n) {
  concat_int(b, n, 2);
}

template<>
void concat_recursive(Bytearray &b, uint24_t n) {
  concat_int(b, (uint32_t)n, 3);
}

template<>
void concat_recursive(Bytearray &b, uint32_t n) {
  concat_int(b, n);
}

template<>
void concat_recursive(Bytearray &b, const Bytearray &b2) {
  concat_bytearray(b, b2);
}

void concat_bytearray(Bytearray &bytes, const Bytearray &bytes2) {
  bytes.insert(bytes.end(), bytes2.begin(), bytes2.end());
}

}

#ifdef TEST_CONCAT
#include <iostream>

int main() {
  uint8_t s[] = "He";
  uint8_t e[] = "wor";
  int32_t inter = 0x206f6c6c;
  int32_t mic = 0x21646c;
  lora::Bytearray start(s, s+sizeof(s)-1), end(e, e+sizeof(e)-1), result;

  result = lora::concat(start, inter, end, mic);

  std::cout << result.data() << std::endl;
}
#endif
