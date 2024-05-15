/* 
  Copyright (c) 2019-2024 Sinric
*/

#pragma once 

#include <vector>
#include <string>
#include <memory>
#include <cstring>

#include <mbedtls/base64.h>
#include <mbedtls/bignum.h>
#include <mbedtls/md.h>
#include <mbedtls/aes.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/pk.h>

#include "ProvDebug.h"

#define MAX_RSA_BUF_SIZE 1024
 
class CryptoMbedTLS {
private:
    bool m_aes_initialized;
    
    mbedtls_ctr_drbg_context m_ctr_drbg_contex;
    mbedtls_entropy_context m_entropy_context;
    mbedtls_pk_context m_pk_context;

public:
    CryptoMbedTLS();
    ~CryptoMbedTLS();

    std::vector<uint8_t> key = {};
    std::vector<uint8_t> iv = {};
        
    // Base64
    std::vector<uint8_t> base64Decode(const std::string& data);
    std::string base64Encode(const std::vector<uint8_t>& data);
 
    // AES CTR
    bool aesCTRXcrypt(const std::vector<uint8_t>& key, std::vector<uint8_t>& iv, std::vector<uint8_t>& data);
    bool aesCTRXdecrypt(const std::vector<uint8_t> &key, std::vector<uint8_t> &iv, std::vector<uint8_t> &data);

    // RSA
    bool initMbedTLS();
    bool getSharedSecret(const std::string& public_key_pem, std::string& data);
    void deinitMbedTLS();
};
