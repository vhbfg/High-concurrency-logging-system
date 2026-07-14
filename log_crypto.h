#pragma once

#include "log_common.h"

#include <string>

std::string xor_encrypt_to_hex(const std::string& plain,
                               unsigned char key = LOG_XOR_KEY);
std::string xor_decrypt_from_hex(const std::string& hex,
                                 unsigned char key = LOG_XOR_KEY);
bool        is_valid_hex_string(const std::string& s);
int         hex_char_to_value(char c);
