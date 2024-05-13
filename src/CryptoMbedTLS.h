/* 
  Copyright (c) 2019-2024 Sinric
*/

#pragma once 

#include <vector>
#include <string>
#include <memory>

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

CryptoMbedTLS::CryptoMbedTLS()
{
}

CryptoMbedTLS::~CryptoMbedTLS()
{
}

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
 * @brief Encrypt a string to AES CTR 
 * @param key
 *      secret key
 * @param iv
 *      initial vector
 * @param data
 *      data to encrypt
 */
bool CryptoMbedTLS::aesCTRXcrypt(const std::vector<uint8_t> &key, std::vector<uint8_t> &iv, std::vector<uint8_t> &data)
{
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    int rc = 0;

    // needed for internal cache
    size_t off = 0;
    unsigned char streamBlock[16] = {0};

    // set IV
    rc = mbedtls_aes_setkey_enc(&ctx, key.data(), key.size() * 8);

    if (rc != 0) {
      mbedtls_aes_free(&ctx);
      DEBUG_PROV(PSTR("[CryptoMbedTLS.aesCTRXcrypt] mbedtls_aes_setkey_enc failed.\r\n"));
      return false;
    } 
 
    //copy iv
    char copyOfIv[16];
    memcpy(copyOfIv, iv.data(), 16);
 
    DEBUG_PROV(PSTR("[CryptoMbedTLS.aesCTRXcrypt] Perform encrypting .."));
    
    rc = mbedtls_aes_crypt_ctr(&ctx,
                          data.size(),
                          &off,
                          (unsigned char*)copyOfIv, //iv.data()
                          streamBlock,
                          data.data(),
                          data.data());

    if (rc != 0) {
      DEBUG_PROV(PSTR("mbedtls_aes_crypt_ctr failed.\r\n"));      
    } else {
      DEBUG_PROV(PSTR("Success!\r\n"));    
    }
    
    mbedtls_aes_free(&ctx);
    return true;
}

/**
 * @brief Decrypt a AES CTR string
 * @param key
 *      secret key
 * @param iv
 *      initial vector
 * @param data
 *      data to encrypt
 */
bool CryptoMbedTLS::aesCTRXdecrypt(const std::vector<uint8_t> &key, std::vector<uint8_t> &iv, std::vector<uint8_t> &data)
{
    if(!m_aes_initialized) {
      DEBUG_PROV(PSTR("[CryptoMbedTLS.aesCTRXdecrypt] AES key not generated!.\r\n"));
      return false;
    }
    
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    int rc = 0;

    // needed for internal cache
    size_t off = 0;
    unsigned char streamBlock[16] = {0};

    DEBUG_PROV(PSTR("[CryptoMbedTLS.aesCTRXdecrypt] Set AES key ..\r\n"));
    
    // set IV
    rc = mbedtls_aes_setkey_dec(&ctx, key.data(), key.size() * 8);

    if (rc != 0) {
      mbedtls_aes_free(&ctx);
      DEBUG_PROV(PSTR("[CryptoMbedTLS.aesCTRXdecrypt] mbedtls_aes_setkey_dec failed.\r\n"));
      return false;
    }  

    //copy iv
    char copyOfIv[16];
    memcpy(copyOfIv, iv.data(), 16);
     
    DEBUG_PROV(PSTR("[CryptoMbedTLS.aesCTRXdecrypt] Perform decrypting .."));
    
    rc = mbedtls_aes_crypt_ctr(&ctx,
                          data.size(),
                          &off,
                          (unsigned char*)copyOfIv, //iv.data()
                          streamBlock,
                          data.data(),
                          data.data());


    if (rc != 0) {
      DEBUG_PROV(PSTR("mbedtls_aes_crypt_ctr failed.\r\n"));      
    } else {
      DEBUG_PROV(PSTR("Success!\r\n"));    
    }

    mbedtls_aes_free(&ctx);
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
 * @brief Generate a shared secret using client's public key
 *
 * @param public_key_pem
 *      public RSA key in pem format
 * @param data
 *      Reference to shared key encrypted using given public key pem file in base64 format.
 */
bool CryptoMbedTLS::getSharedSecret(const std::string& public_key_pem, std::string& data) {
  DEBUG_PROV(PSTR("[CryptoMbedTLS.getSharedSecret()] Loading Public key..."));    
  
  size_t size_with_null = public_key_pem.size() + 1;
  const char* public_key_pem_c = public_key_pem.c_str();
  int rc = 0;
  rc = mbedtls_pk_parse_public_key(&m_pk_context, reinterpret_cast<const unsigned char*>(public_key_pem_c), size_with_null);

  if (rc != 0) {
    DEBUG_PROV(PSTR("mbedtls_pk_parse_public_key failed.\r\n"));
    return false;
  } else {
    DEBUG_PROV(PSTR("Success!\r\n"));    
  }

  DEBUG_PROV(PSTR("[CryptoMbedTLS.getSharedSecret()] Generating session key..."));    
    
  unsigned char session_key[32] = {0};    
  rc = mbedtls_ctr_drbg_random(&m_ctr_drbg_contex, session_key, 32);    
  
  if(rc != 0) {
    DEBUG_PROV(PSTR("mbedtls_ctr_drbg_random failed.\r\n"));
    return false;
  } else {
    DEBUG_PROV(PSTR("Success!\r\n"));  
  } 

  DEBUG_PROV(PSTR("[CryptoMbedTLS.getSharedSecret()] Encrypting session key using public key..."));

  std::vector<uint8_t> outbuf(512);
  size_t olen = 0;
  
  rc = mbedtls_pk_encrypt( &m_pk_context, session_key, sizeof(session_key),
                                  &outbuf[0], &olen, 512,
                                  mbedtls_ctr_drbg_random, &m_ctr_drbg_contex);
  if (rc != 0) {
    DEBUG_PROV(PSTR("mbedtls_pk_encrypt failed.\r\n"));
    return false;
  } else {
    DEBUG_PROV(PSTR("Success!\r\n"));  
  }

  key.resize(16);
  memcpy(&key[0], session_key, 16);

  iv.resize(16);
  memcpy(&iv[0], &session_key[16], 16);

//  Serial.printf("AES key:");
//  int i;
//  for(i = 0; i < 16; i++) {
//    Serial.printf("%02x", key.data[i]);
//  }
//  Serial.println();
//
//  Serial.printf("AES IV:");
//  for(i = 0; i < 16; i++) {
//    Serial.printf("%02x", iv.data[i]);
//  }
//  Serial.println();


  DEBUG_PROV(PSTR("[CryptoMbedTLS.getSharedSecret()] Converting to Base64...\r\n"));
  outbuf.resize(olen);
  data = base64Encode(outbuf); 

  DEBUG_PROV(PSTR("[CryptoMbedTLS.getSharedSecret()] Done...\r\n"));
  m_aes_initialized = true;
  return true;
}
