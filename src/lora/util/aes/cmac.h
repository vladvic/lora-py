/***************************************************
 * cmac.h
 * Created on Mon, 13 Jul 2020 10:18:05 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

void AES_ECB_CMAC(const struct AES_ctx* ctx, const uint8_t* buf, int len, uint8_t *cmac);

