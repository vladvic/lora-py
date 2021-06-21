/***************************************************
 * base64.h
 * Created on Mon, 08 Oct 2018 15:11:06 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <string>
#include <vector>
#include "lora/types.h"

namespace semtech {

void base64Encode(std::string &dst, const lora::Bytearray &src);
void base64Decode(lora::Bytearray &dst, const char *src);

}
