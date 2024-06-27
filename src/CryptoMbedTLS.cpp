/*
 *  Copyright (c) 2019 - 2024 Sinric. All rights reserved.
 *  Licensed under Creative Commons Attribution-Share Alike (CC BY-SA)
 *
 *  This file is part of the Sinric Pro ESP32 Business SDK (https://github.com/sinricpro/esp32-business-sdk)
 */
#include "CryptoMbedTLS.h"

/**
 * @brief Decode a base64 string
 * @param data
 *      Data to encode
 * @return
 *      Encoded data
 */
std::vector<uint8_t> CryptoMbedTLS::base64Decode(const std::string &data)
{
    // Calculate max decode length
    size_t requiredSize;

    mbedtls_base64_encode(nullptr, 0, &requiredSize, (unsigned char *)data.c_str(), data.size());

    std::vector<uint8_t> output(requiredSize);
    size_t outputLen = 0;
    mbedtls_base64_decode(output.data(), requiredSize, &outputLen, (unsigned char *)data.c_str(), data.size());

    return std::vector<uint8_t>(output.begin(), output.begin() + outputLen);
}

/**
 * @brief Covert a string to base64 encoded string
 * @param data
 *      Data to encode
 * @return
 *      Encoded data
 */
std::string CryptoMbedTLS::base64Encode(const std::vector<uint8_t> &data)
{
    // Calculate max output length
    size_t requiredSize;
    mbedtls_base64_encode(nullptr, 0, &requiredSize, data.data(), data.size());

    std::vector<uint8_t> output(requiredSize);
    size_t outputLen = 0;

    mbedtls_base64_encode(output.data(), requiredSize, &outputLen, data.data(), data.size());

    return std::string(output.begin(), output.begin() + outputLen);
}
 
/**
 * @brief Base function for AES CTR encryption/decryption
 * @param key Secret key
 * @param iv Initial vector
 * @param data Data to encrypt/decrypt
 * @param isEncrypt True for encryption, false for decryption
 * @return Boolean indicating success or failure
 */
bool CryptoMbedTLS::aesCTRXcryptBase(const std::vector<uint8_t> &key, std::vector<uint8_t> &iv, std::vector<uint8_t> &data, bool isEncrypt) 
{
    if (!isAesInitialized()) return false;

    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);

    if (!setupAesContext(ctx, key, isEncrypt)) {
        mbedtls_aes_free(&ctx);
        return false;
    }

    bool success = performCryption(ctx, iv, data, isEncrypt);

    mbedtls_aes_free(&ctx);
    return success;
}

/**
 * @brief Encrypt a string using AES CTR 
 */
bool CryptoMbedTLS::aesCTRXcrypt(const std::vector<uint8_t> &key, std::vector<uint8_t> &iv, std::vector<uint8_t> &data) 
{
    return aesCTRXcryptBase(key, iv, data, true);
}

/**
 * @brief Decrypt an AES CTR encrypted string
 */
bool CryptoMbedTLS::aesCTRXdecrypt(const std::vector<uint8_t> &key, std::vector<uint8_t> &iv, std::vector<uint8_t> &data)
{
    return aesCTRXcryptBase(key, iv, data, false);
}

bool CryptoMbedTLS::isAesInitialized()
{
    if (!m_aes_initialized) {
        DEBUG_PROV(PSTR("[CryptoMbedTLS.isAesInitialized()]: AES keys not generated!.\r\n"));
        return false;
    }
    return true;
}

bool CryptoMbedTLS::setupAesContext(mbedtls_aes_context &ctx, const std::vector<uint8_t> &key, bool isEncrypt)
{
    int rc = isEncrypt ? mbedtls_aes_setkey_enc(&ctx, key.data(), key.size() * 8)
                       : mbedtls_aes_setkey_dec(&ctx, key.data(), key.size() * 8);
    
    if (rc != 0) {
        DEBUG_PROV(PSTR("[CryptoMbedTLS.setupAesContext()]: mbedtls_aes_setkey_%s failed.\r\n"), isEncrypt ? "enc" : "dec");
        return false;
    }
    return true;
}

bool CryptoMbedTLS::performCryption(mbedtls_aes_context &ctx, std::vector<uint8_t> &iv, std::vector<uint8_t> &data, bool isEncrypt)
{
    size_t off = 0;
    unsigned char streamBlock[16] = {0};
    char copyOfIv[16];
    memcpy(copyOfIv, iv.data(), 16);

    DEBUG_PROV(PSTR("[CryptoMbedTLS.performCryption()]: Perform %s .."), isEncrypt ? "encrypting" : "decrypting");

    int rc = mbedtls_aes_crypt_ctr(&ctx, data.size(), &off,
                                   reinterpret_cast<unsigned char*>(copyOfIv),
                                   streamBlock, data.data(), data.data());

    if (rc != 0) {
        DEBUG_PROV(PSTR("[CryptoMbedTLS.performCryption()]: mbedtls_aes_crypt_ctr failed.\r\n"));
    } else {
        DEBUG_PROV(PSTR("Success!\r\n"));
    }

    return (rc == 0);
} 

/**
 * @brief initialize MbedTLS
 */
bool CryptoMbedTLS::initMbedTLS() {
  mbedtls_ctr_drbg_init(&m_ctr_drbg_contex);
  mbedtls_entropy_init(&m_entropy_context);
  mbedtls_pk_init(&m_pk_context);

  // Initialize entropy.
  int res = mbedtls_ctr_drbg_seed(
      &m_ctr_drbg_contex, mbedtls_entropy_func, &m_entropy_context, NULL, 0);
  
  if (res != 0) {
    DEBUG_PROV(PSTR("[CryptoMbedTLS.initMbedTLS()] mbedtls_ctr_drbg_seed failed.\r\n"));
    return false;
  }
 
  DEBUG_PROV(PSTR("[CryptoMbedTLS.initMbedTLS() mbedtls initialized.\r\n"));
  return true;
}

/**
 * @brief clean up MbedTLS memory allocations
 */
void CryptoMbedTLS::deinitMbedTLS() {
  DEBUG_PROV(PSTR("[CryptoMbedTLS.deinitMbedTLS()] mbedtls deinit.\r\n"));
   
  mbedtls_pk_free(&m_pk_context);
  mbedtls_entropy_free(&m_entropy_context);
  mbedtls_ctr_drbg_free(&m_ctr_drbg_contex);
}

/**
 * @brief Generate a shared secret using public key
 *
 * @param public_key_pem Public RSA key in PEM format
 * @param data Reference to shared key
 * @return Boolean indicating success or failure
 */
bool CryptoMbedTLS::getSharedSecret(const std::string& public_key_pem, std::string& data) {
    if (!parsePublicKey(public_key_pem)) return false;
    
    unsigned char session_key[32];
    if (!generateSessionKey(session_key)) return false;
    
    std::vector<uint8_t> encrypted_key;
    if (!encryptSessionKey(session_key, encrypted_key)) return false;
    
    prepareAesKeyAndIv(session_key);
    encodeSessionKey(encrypted_key, data);
    
    return true;
}

bool CryptoMbedTLS::parsePublicKey(const std::string& public_key_pem) {
    DEBUG_PROV(PSTR("[CryptoMbedTLS.parsePublicKey()]: Loading Public key..."));
    
    int rc = mbedtls_pk_parse_public_key(&m_pk_context,
                                         reinterpret_cast<const unsigned char*>(public_key_pem.c_str()),
                                         public_key_pem.size() + 1);
    if (rc != 0) {
        DEBUG_PROV(PSTR("[CryptoMbedTLS.parsePublicKey()]: mbedtls_pk_parse_public_key failed.\r\n"));
        return false;
    }
    DEBUG_PROV(PSTR("[CryptoMbedTLS.parsePublicKey()]: Public key loaded successfully.\r\n"));
    return true;
}

bool CryptoMbedTLS::generateSessionKey(unsigned char* session_key) {
  DEBUG_PROV(PSTR("[CryptoMbedTLS.generateSessionKey()]: Generating sessionKey key..."));

    int rc = mbedtls_ctr_drbg_random(&m_ctr_drbg_contex, session_key, 32);
    if (rc != 0) {
        DEBUG_PROV(PSTR("[CryptoMbedTLS.generateSessionKey()]: mbedtls_ctr_drbg_random failed.\r\n"));
        return false;
    }
    DEBUG_PROV(PSTR("[CryptoMbedTLS.generateSessionKey()]: Session key generated successfully.\r\n"));
    return true;
}

bool CryptoMbedTLS::encryptSessionKey(const unsigned char* session_key, std::vector<uint8_t>& encrypted_key) {
  DEBUG_PROV(PSTR("[CryptoMbedTLS.encryptSessionKey()]: Encrypting sessionKey key..."));

    encrypted_key.resize(512);
    size_t olen = 0;
    int rc = mbedtls_pk_encrypt(&m_pk_context, session_key, 32,
                                encrypted_key.data(), &olen, encrypted_key.size(),
                                mbedtls_ctr_drbg_random, &m_ctr_drbg_contex);
    if (rc != 0) {
        DEBUG_PROV(PSTR("[CryptoMbedTLS.encryptSessionKey()]: mbedtls_pk_encrypt failed.\r\n"));
        return false;
    }
    encrypted_key.resize(olen);
    DEBUG_PROV(PSTR("[CryptoMbedTLS.encryptSessionKey()]: Session key encrypted successfully.\r\n"));
    return true;
}

void CryptoMbedTLS::prepareAesKeyAndIv(const unsigned char* session_key) {
    key.resize(16);
    memcpy(&key[0], session_key, 16);
    iv.resize(16);
    memcpy(&iv[0], &session_key[16], 16);
    m_aes_initialized = true;
    DEBUG_PROV(PSTR("[CryptoMbedTLS.prepareAesKeyAndIv()]: AES initialized successfully.\r\n"));
}

void CryptoMbedTLS::encodeSessionKey(const std::vector<uint8_t>& encrypted_key, std::string& data) {
    data = base64Encode(encrypted_key);
    DEBUG_PROV(PSTR("[CryptoMbedTLS.encodeSessionKey()]: Session key encoded to Base64.\r\n"));
} 

CryptoMbedTLS::CryptoMbedTLS()
{
}

CryptoMbedTLS::~CryptoMbedTLS()
{
}
