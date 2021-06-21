/***************************************************
 * base64.cpp
 * Created on Mon, 08 Oct 2018 15:16:09 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <string.h>
#include "base64.h"

namespace semtech {

static const char base64Code[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz"
                                 "0123456789+/";

void base64Encode(std::string &dst, const lora::Bytearray &src) {
  dst.resize(0);
  int val=0, valb=-6;

  for (unsigned char c : src) {
    val = (val<<8) + c;
    valb += 8;
    while (valb>=0) {
      dst.push_back(base64Code[(val>>valb)&0x3F]);
      valb-=6;
    }
  }

  if (valb>-6) dst.push_back(base64Code[((val<<8)>>(valb+8))&0x3F]);

  while (dst.size()%4) {
    dst.push_back('=');
  }
}

static const unsigned char base64Reverse[] =
{
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,   //   0..15
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,   //  16..31
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,   //  32..47
  52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255, 254, 255, 255,   //  48..63
  255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,   //  64..79
  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,   //  80..95
  255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,   //  96..111
  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255,   // 112..127
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,   // 128..143
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
};

#ifdef __AVX2__

#include <pmmintrin.h>
#include <immintrin.h>

static inline __m256i dec_reshuffle(__m256i in) {

  // inlined procedure pack_madd from https://github.com/WojciechMula/base64simd/blob/master/decode/pack.avx2.cpp
  // The only difference is that elements are reversed,
  // only the multiplication constants were changed.

  const __m256i merge_ab_and_bc = _mm256_maddubs_epi16(in, _mm256_set1_epi32(0x01400140)); //_mm256_maddubs_epi16 is likely expensive
  __m256i out = _mm256_madd_epi16(merge_ab_and_bc, _mm256_set1_epi32(0x00011000));
  // end of inlined

  // Pack bytes together within 32-bit words, discarding words 3 and 7:
  out = _mm256_shuffle_epi8(out, _mm256_setr_epi8(
        2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1,
        2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1
  ));
  // the call to _mm256_permutevar8x32_epi32 could be replaced by a call to _mm256_storeu2_m128i but it is doubtful that it would help
  return _mm256_permutevar8x32_epi32(
      out, _mm256_setr_epi32(0, 1, 2, 4, 5, 6, -1, -1));
}

size_t fast_avx2_base64_decode(char *out, const char *src, size_t &srclen) {
  char* out_orig = out;
  while (srclen >= 45) {
    __m256i str = _mm256_loadu_si256((__m256i *)src);

    // code by @aqrit from
    // https://github.com/WojciechMula/base64simd/issues/3#issuecomment-271137490
    // transated into AVX2
    const __m256i lut_lo = _mm256_setr_epi8(
        0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x13, 0x1A, 0x1B, 0x1B, 0x1B, 0x1A,
        0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x13, 0x1A, 0x1B, 0x1B, 0x1B, 0x1A
        );
    const __m256i lut_hi = _mm256_setr_epi8(
        0x10, 0x10, 0x01, 0x02, 0x04, 0x08, 0x04, 0x08,
        0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
        0x10, 0x10, 0x01, 0x02, 0x04, 0x08, 0x04, 0x08,
        0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
        );
    const __m256i lut_roll = _mm256_setr_epi8(
        0,   16,  19,   4, -65, -65, -71, -71,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   16,  19,   4, -65, -65, -71, -71,
        0,   0,   0,   0,   0,   0,   0,   0
        );

    const __m256i mask_2F = _mm256_set1_epi8(0x2f);

    // lookup
    __m256i hi_nibbles  = _mm256_srli_epi32(str, 4);
    __m256i lo_nibbles  = _mm256_and_si256(str, mask_2F);

    const __m256i lo    = _mm256_shuffle_epi8(lut_lo, lo_nibbles);
    const __m256i eq_2F = _mm256_cmpeq_epi8(str, mask_2F);

    hi_nibbles = _mm256_and_si256(hi_nibbles, mask_2F);
    const __m256i hi    = _mm256_shuffle_epi8(lut_hi, hi_nibbles);
    const __m256i roll  = _mm256_shuffle_epi8(lut_roll, _mm256_add_epi8(eq_2F, hi_nibbles));

    if (!_mm256_testz_si256(lo, hi)) {
      break;
    }

    str = _mm256_add_epi8(str, roll);
    // end of copied function

    srclen -= 32;
    src += 32;

    // end of inlined function

    // Reshuffle the input to packed 12-byte output format:
    str = dec_reshuffle(str);
    _mm256_storeu_si256((__m256i *)out, str);
    out += 24;
  }
  return out - out_orig;
}

#else
#warning "No AVX detected: using slow base64 decoder"

size_t slow_plain_base64_decode(char *dst, const char *src, size_t &src_size) {
  size_t s, d = 0;
  char curval = 0, decoded;

  if(src_size % 4) {
    // Error!
    return 0;
  }

  for(s = 0; s < src_size; s ++) {
    if(src[s] == '=') {
      dst[d] = curval;
      break;
    }   

    decoded = base64Reverse[(uint8_t)src[s]];

    switch(s % 4) {
    case 0:
      curval = decoded << 2;
      break;
    case 1:
      curval |= decoded >> 4;
      dst[d ++] = curval;
      curval = decoded << 4;
      break;
    case 2:
      curval |= decoded >> 2;
      dst[d ++] = curval;
      curval = decoded << 6;
      break;
    case 3:
      curval |= decoded;
      dst[d ++] = curval;
    }
  }

  src_size = 0;
  return d;
}

#endif

void base64Decode(lora::Bytearray &dst, const char *src) {
  size_t size = strlen(src);
  dst.resize(size * 3 / 4);
  size_t left = size;
  unsigned char c = 0;

  size_t out = 
#ifdef __AVX2__
              fast_avx2_base64_decode((char*)dst.data(), src, left);
#else
              slow_plain_base64_decode((char*)dst.data(), src, left);
#endif
  dst.resize(out);

  for(size_t i = size-left; i < size; i ++) {
    c  = base64Reverse[(int)src[i]] << 2;
    c |= base64Reverse[(int)src[++i]] >> 4;
    if(src[i] == '=') break;
    dst.push_back((char)c);
    c  = base64Reverse[(int)src[i]] << 4;
    c |= base64Reverse[(int)src[++i]] >> 2;
    if(src[i] == '=') break;
    dst.push_back((char)c);
    c  = base64Reverse[(int)src[i]] << 6;
    c |= base64Reverse[(int)src[++i]];
    if(src[i] == '=') break;
    dst.push_back((char)c);
  }
}

}
