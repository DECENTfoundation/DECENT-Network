//
// Created by Josef Sevcik on 25/11/2016.
//
#include <decent/encrypt/encryptionutils.hpp>

#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/files.h>
#include <cryptopp/ccm.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>

namespace decent{ namespace crypto{
AutoSeededRandomPool rng;

std::string d_integer::to_string() const {


}

static encryption_results AES_encrypt_file(const std::string &fileIn, const std::string &fileOut, const aes_key &key) {
    try {
        CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption e;
        CryptoPP::FileSink fs(fileOut.c_str(), true);
        e.SetKey(key.key_byte, CryptoPP::AES::MAX_KEYLENGTH);
        CryptoPP::StreamTransformationFilter* filter=new CryptoPP::StreamTransformationFilter(e, &fs);

        const char* file_name = fileIn.c_str();
        CryptoPP::FileSource* fsource= new CryptoPP::FileSource(file_name, true, filter);
    } catch (const CryptoPP::Exception &e) {
        elog(e.GetWhat());
        switch (e.GetErrorType()) {
            case CryptoPP::Exception::IO_ERROR:
                return io_error;
            default:
                return other_error;
        }
    }
    return ok;
}

static encryption_results AES_decrypt_file(const std::string &fileIn, const std::string &fileOut, const aes_key &key) {
    try {
        CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption d;
        d.SetKey(key.key_byte, CryptoPP::AES::MAX_KEYLENGTH);
        const char* file_name = fileIn.c_str();
        CryptoPP::FileSink fs(fileOut.c_str(), true);
        CryptoPP::StreamTransformationFilter* filter = new CryptoPP::StreamTransformationFilter(d, &fs);
        CryptoPP::FileSource* fsource = new CryptoPP::FileSource(file_name, true, filter);
    } catch (const CryptoPP::Exception &e) {
        elog(e.GetWhat());
        switch (e.GetErrorType()) {
            case CryptoPP::Exception::IO_ERROR:
                return io_error;
            default:
                return other_error;
        }
    }
    return ok;
}

d_integer generate_private_el_gamal_key()
{
    CryptoPP::Integer im (rng, CryptoPP::Integer::One(), EL_GAMAL_MODULUS_512 -1);
    d_integer key = im;
    return key;
}

d_integer get_public_el_gamal_key(const d_integer &privateKey)
{
    ModularArithmetic ma(EL_GAMAL_MODULUS_512);
    d_integer publicKey = ma.Exponentiate(EL_GAMAL_GROUP_GENERATOR, privateKey);

    return publicKey;
}

static encryption_results el_gamal_encrypt(const valtype &message, d_integer &publicKey, valtype &ciphertext)
{
    ElGamalKeys::PublicKey key;
    key.AccessGroupParameters().Initialize(EL_GAMAL_MODULUS_512, EL_GAMAL_GROUP_GENERATOR);
    key.SetPublicElement(publicKey);

    CryptoPP::Integer randomizer(rng, CryptoPP::Integer::One(), EL_GAMAL_MODULUS_512 - 1);

    try{
        byte buffer[MESSAGE_SIZE];
        FC_ASSERT(message.size() <= MESSAGE_SIZE);
        copy(message.begin(), message.end(), buffer);
        SecByteBlock messageBytes(buffer, MESSAGE_SIZE);
        SecByteBlock outcomingCiphertextBytes(EL_GAMAL_CIPHERTEXT_SIZE);

        ElGamal::GroupParameters groupParams;
        groupParams.Initialize(EL_GAMAL_MODULUS_512, EL_GAMAL_GROUP_GENERATOR);
        ModularArithmetic mr(EL_GAMAL_MODULUS_512);
        CryptoPP::Integer publicElement = key.GetPublicElement();
        FC_ASSERT(publicKey == publicElement);

        CryptoPP::Integer m;
        m.Decode(messageBytes, MESSAGE_SIZE);
        CryptoPP::Integer d1 = groupParams.ExponentiateBase(randomizer);
        CryptoPP::Integer c1 = mr.Multiply(m, mr.Exponentiate(publicElement, randomizer));

        groupParams.EncodeElement(true, c1, outcomingCiphertextBytes.BytePtr());
        groupParams.EncodeElement(true, d1, outcomingCiphertextBytes.BytePtr()+EL_GAMAL_GROUP_ELEMENT_SIZE);

        ciphertext.reserve(EL_GAMAL_CIPHERTEXT_SIZE);
        std::copy(outcomingCiphertextBytes.begin(), outcomingCiphertextBytes.end(), std::back_inserter(ciphertext));
    }catch(const CryptoPP::Exception &e) {
        elog(e.GetWhat());
        switch (e.GetErrorType()) {
            case CryptoPP::Exception::IO_ERROR:
                return io_error;
            default:
                return other_error;
        }
    }
    return ok;
}

static encryption_results el_gamal_decrypt(const valtype &ciphertext, d_integer &privateKey, valtype &plaintext)
{
    ElGamalKeys::PrivateKey key;
    key.AccessGroupParameters().Initialize(EL_GAMAL_MODULUS_512, EL_GAMAL_GROUP_GENERATOR);
    key.SetPrivateExponent(privateKey);
    try{
        FC_ASSERT(ciphertext.size() != EL_GAMAL_CIPHERTEXT_SIZE);
        byte buffer[EL_GAMAL_CIPHERTEXT_SIZE];
        copy(ciphertext.begin(), ciphertext.end(), buffer);
        CryptoPP::SecByteBlock ciphertextBytes(buffer, EL_GAMAL_CIPHERTEXT_SIZE);
        byte recovered[MESSAGE_SIZE];
        ElGamal::GroupParameters groupParams;
        groupParams.Initialize(EL_GAMAL_MODULUS_512, EL_GAMAL_GROUP_GENERATOR);
        CryptoPP::Integer privateExponent =  key.GetPrivateExponent();
        FC_ASSERT(privateExponent == privateKey);
        ModularArithmetic mr(groupParams.GetModulus());

        CryptoPP::Integer c1(ciphertextBytes.BytePtr(), (EL_GAMAL_GROUP_ELEMENT_SIZE));
        CryptoPP::Integer d1(ciphertextBytes.BytePtr() + (EL_GAMAL_GROUP_ELEMENT_SIZE), (EL_GAMAL_GROUP_ELEMENT_SIZE));
        CryptoPP::Integer s = mr.Exponentiate(d1, privateExponent);
        CryptoPP::Integer m = mr.Multiply(c1, mr.MultiplicativeInverse(s));
        size_t size = m.MinEncodedSize();

        m.Encode(recovered, size);
        plaintext.reserve(size);
        std::copy(recovered, recovered+size, std::back_inserter(plaintext));
    }catch (const CryptoPP::Exception &e) {
        elog(e.GetWhat());
        switch (e.GetErrorType()) {
            case CryptoPP::Exception::IO_ERROR:
                return io_error;
            default:
                return other_error;
        }
    }
    return ok;
}


}}//namespace