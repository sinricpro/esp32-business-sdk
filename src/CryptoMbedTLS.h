/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 *  
 *  @brief This class (CryptoMbedTLS) provides essential cryptographic functionalities like base64 encoding/decoding, 
 *  AES CTR encryption/decryption, and RSA key exchange for secure communication during BLE provisioning.
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

    bool parsePublicKey(const std::string& public_key_pem);
    bool generateSessionKey(unsigned char* session_key);
    bool encryptSessionKey(const unsigned char* session_key, std::vector<uint8_t>& encrypted_key);
    void prepareAesKeyAndIv(const unsigned char* session_key);
    void encodeSessionKey(const std::vector<uint8_t>& encrypted_key, std::string& data);

    bool performCryption(mbedtls_aes_context &ctx, std::vector<uint8_t> &iv, std::vector<uint8_t> &data, bool isEncrypt);
    bool setupAesContext(mbedtls_aes_context &ctx, const std::vector<uint8_t> &key, bool isEncrypt);
    bool isAesInitialized();
    bool aesCTRXcryptBase(const std::vector<uint8_t> &key, std::vector<uint8_t> &iv, std::vector<uint8_t> &data, bool isEncrypt);
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
