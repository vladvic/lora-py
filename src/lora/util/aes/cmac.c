/***************************************************
 * cmac.c
 * Created on Mon, 13 Jul 2020 10:34:07 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <string.h>
#include <stdio.h>
#include "aes.h"

const uint8_t const_Zero[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
const uint8_t const_Rb[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x87,};
const int block_size = 16;

#ifdef TEST_CMAC
static void
print_buffer(unsigned char *b, int len) {
  int i;
  for(i = 0; i < len; ++ i) {
    printf("%02x", b[i]);
  }
  printf("\n");
}
#endif

void CMAC_key_gen(const struct AES_ctx* ctx, char K1[16], char K2[16]) {
  // Generate K1 and K2 keys
  char L[16];
  int i;

  memcpy(L, const_Zero, sizeof(L));
  AES_ECB_encrypt(ctx, (unsigned char*)L);

  for(i = 0; i < block_size; ++ i) {
    K1[i] = (L[i] << 1) & 0xfe;
    if(i < (block_size - 1)) {
      K1[i] |= (L[i+1] >> 7) & 0x01;
    }
  }

  if((L[0] & 0x80)) {
    K1[block_size-1] ^= const_Rb[block_size-1];
  }

  for(i = 0; i < block_size; ++ i) {
    K2[i] = (K1[i] << 1) & 0xfe;
    if(i < (block_size - 1)) {
      K2[i] |= (K1[i+1] >> 7) & 0x01;
    }
  }

  if((K1[0] & 0x80)) {
    K2[block_size-1] ^= const_Rb[block_size-1];
  }
}

void AES_ECB_CMAC(const struct AES_ctx* ctx, const uint8_t* buf, int len, uint8_t *cmac) {
  char K1[block_size];
  char K2[block_size];
  char M_i[block_size];

  memcpy(cmac, const_Zero, block_size);

  CMAC_key_gen(ctx, K1, K2);

  int i, k, flag = (((len % block_size) == 0) && (len > 0)), last = 0;
  for(i = 0; i < (len + (len == 0)); i += block_size) {
    int block_len = (len - i);
    if(block_len > block_size) {
      block_len = block_size;
    }
    else {
      last = 1;
    }

    memcpy(M_i, &buf[i], block_len);

    if(last) {
      for(k = 0; k < block_size; k ++) {
        if(k == block_len) {
          M_i[k] = 0x80;
        } else if(k > block_len) {
          M_i[k] = 0;
        }

        if(flag) {
          M_i[k] ^= K1[k];
        }
        else {
          M_i[k] ^= K2[k];
        }
      }
    }

    for(k = 0; k < block_size; k ++) {
      cmac[k] ^= M_i[k];
    }

    AES_ECB_encrypt(ctx, cmac);
  }
}

#ifdef TEST_CMAC
#include <stdio.h>

int main() {
  uint8_t key[] = {0x2b, 0x7e, 0x15, 0x16 , 0x28, 0xae, 0xd2, 0xa6 , 0xab, 0xf7, 0x15, 0x88 , 0x09, 0xcf, 0x4f, 0x3c};
  //uint8_t key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
  uint8_t input[] = "Hello world! I'm so glad to see you!";
  uint8_t cmac[16];
  struct AES_ctx ctx;

  print_buffer(key, sizeof(key));
  AES_init_ctx(&ctx, key);
  AES_ECB_CMAC(&ctx, input, sizeof(input) - 1, cmac);

  print_buffer(cmac, sizeof(cmac));
}

#endif
