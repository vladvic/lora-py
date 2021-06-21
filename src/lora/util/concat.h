/***************************************************
 * concat.h
 * Created on Tue, 14 Jul 2020 03:58:33 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <tuple>
#include "../types.h"

namespace lora {

void concat_int(Bytearray &bytes, uint32_t num, int numBytes = 4);
void concat_bytearray(Bytearray &bytes, const Bytearray &bytes2);
uint32_t extract_int(const uint8_t *from, int numBytes = 4);

template<typename ... Ts>
void concat_recursive(Bytearray &b, uint8_t n, const Ts &... xs);
template<typename ... Ts>
void concat_recursive(Bytearray &b, uint16_t n, const Ts &... xs);
template<typename ... Ts>
void concat_recursive(Bytearray &b, uint24_t n, const Ts &... xs);
template<typename ... Ts>
void concat_recursive(Bytearray &b, uint32_t n, const Ts &... xs);
template<typename ... Ts>
void concat_recursive(Bytearray &b, const Bytearray &b2, const Ts &... xs);
template<>
void concat_recursive(Bytearray &b, uint8_t n);
template<>
void concat_recursive(Bytearray &b, uint16_t n);
template<>
void concat_recursive(Bytearray &b, uint24_t n);
template<>
void concat_recursive(Bytearray &b, uint32_t n);
template<>
void concat_recursive(Bytearray &b, const Bytearray &b2);

template<typename ... Ts>
void concat_recursive(Bytearray &b, uint8_t n, const Ts &... xs) {
  b.push_back(n);
  concat_recursive(b, xs...);
}

template<typename ... Ts>
void concat_recursive(Bytearray &b, uint16_t n, const Ts &... xs) {
  concat_int(b, n, 2);
  concat_recursive(b, xs...);
}

template<typename ... Ts>
void concat_recursive(Bytearray &b, uint24_t n, const Ts &... xs) {
  concat_int(b, (uint32_t)n, 3);
  concat_recursive(b, xs...);
}

template<typename ... Ts>
void concat_recursive(Bytearray &b, uint32_t n, const Ts &... xs) {
  concat_int(b, n);
  concat_recursive(b, xs...);
}

template<typename ... Ts>
void concat_recursive(Bytearray &b, const Bytearray &b2, const Ts &... xs) {
  concat_bytearray(b, b2);
  concat_recursive(b, xs...);
}

template<typename ... Ts>
Bytearray concat(const Ts &... xs) {
  Bytearray result;
  concat_recursive(result, xs...);

  return result;
}

}
