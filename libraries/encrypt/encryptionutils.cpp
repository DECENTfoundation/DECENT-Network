/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
//
// Created by Josef Sevcik on 25/11/2016.
//
#include <decent/encrypt/encryptionutils.hpp>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/files.h>
#include <cryptopp/ccm.h>
#include <cryptopp/md5.h>

#include <string>
#include <sstream>
#include <fstream>
#include <streambuf>
#include <fc/log/logger.hpp>
#include <fc/crypto/sha512.hpp>
#include <iostream>

Params::Params()
: DECENT_EL_GAMAL_MODULUS_512("11760620558671662461946567396662025495126946227619472274601251081547302009186313201119191293557856181195016058359990840577430081932807832465057884143546419.")
, DECENT_EL_GAMAL_GROUP_GENERATOR(3)
, DECENT_SHAMIR_ORDER("115792089237316195423570985008687907852837564279074904382605163141518161494337")
{
}

std::unique_ptr<Params> Params::_instance;

namespace decent{ namespace encrypt{

AutoSeededRandomPool rng;

DeliveryProofString::DeliveryProofString( DeliveryProof gf ):G1(gf.G1),G2(gf.G2),G3(gf.G3),s(gf.s),r(gf.r){};
CiphertextString::CiphertextString(Ciphertext ct) : C1(ct.C1), D1(ct.D1) {};
DIntegerString::DIntegerString(const DInteger& d) { s = d.to_string(); };
DIntegerString& DIntegerString::operator=(const DInteger& d) { s = d.to_string();return *this;};
DIntegerString::DIntegerString() { DInteger a=CryptoPP::Integer::Zero();s = a.to_string(); };

std::string DInteger::to_string() const
{
   std::ostringstream oss;
   oss<<*this;
   std::string tmp(oss.str());
   return tmp;
}


DInteger DInteger::from_string(std::string from)
{
   CryptoPP::Integer tmp(from.c_str());
   DInteger tmp2(tmp);
   return tmp2;
}

encryption_results AES_encrypt_file(const std::string &fileIn, const std::string &fileOut, const AesKey &key) {
    try {
#if CRYPTOPP_VERSION >= 600
        CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];
#else
        byte iv[CryptoPP::AES::BLOCKSIZE];
#endif
        memset(iv, 0, sizeof(iv));
        CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption e;
        e.SetKeyWithIV(key.key_byte, CryptoPP::AES::MAX_KEYLENGTH, iv);
        CryptoPP::FileSource fsource(fileIn.c_str(), true,
            new CryptoPP::StreamTransformationFilter(e,
                new CryptoPP::FileSink(fileOut.c_str(), true)));
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

encryption_results AES_decrypt_file(const std::string &fileIn, const std::string &fileOut, const AesKey &key) {
    try {
#if CRYPTOPP_VERSION >= 600
       CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];
#else
       byte iv[CryptoPP::AES::BLOCKSIZE];
#endif
       memset(iv, 0, sizeof(iv));
       CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption d;
       d.SetKeyWithIV(key.key_byte, CryptoPP::AES::MAX_KEYLENGTH, iv);
       const char* file_name = fileIn.c_str();
       CryptoPP::FileSink * fs = new CryptoPP::FileSink (fileOut.c_str(), true);
       CryptoPP::StreamTransformationFilter* filter = new CryptoPP::StreamTransformationFilter(d, fs);
       CryptoPP::FileSource fsource (file_name, true, filter);
    } catch (const CryptoPP::Exception &e) {
        elog(e.GetWhat());
        switch (e.GetErrorType()) {
           case CryptoPP::Exception::IO_ERROR:
              return io_error;
           case CryptoPP::Exception::INVALID_DATA_FORMAT:
              return key_error;
           default:
              return other_error;
        }
    }
    return ok;
}

DInteger generate_private_el_gamal_key()
{
    CryptoPP::Integer im (rng, CryptoPP::Integer::One(), Params::instance().DECENT_EL_GAMAL_MODULUS_512 -1);
    DInteger key = im;
    return key;
}

DInteger generate_private_el_gamal_key_from_secret(fc::sha256 secret)
{
   fc::sha512 hash = fc::sha512::hash( (char*) secret._hash, 32 );
#if CRYPTOPP_VERSION >= 600
   CryptoPP::Integer key( (CryptoPP::byte*)hash._hash, 64 );
#else
   CryptoPP::Integer key( (byte*)hash._hash, 64 );
#endif
   DInteger ret = key.Modulo(Params::instance().DECENT_EL_GAMAL_MODULUS_512);
   return key;
}

DInteger get_public_el_gamal_key(const DInteger &privateKey)
{
    ModularArithmetic ma(Params::instance().DECENT_EL_GAMAL_MODULUS_512);
    DInteger publicKey = ma.Exponentiate(Params::instance().DECENT_EL_GAMAL_GROUP_GENERATOR, privateKey);

    return publicKey;
}

encryption_results el_gamal_encrypt(const point &message, const DInteger &publicKey, Ciphertext &result)
{
    //elog("el_gamal_encrypt called ${m} ${pk} ",("m", message)("pk", publicKey));
    ElGamalKeys::PublicKey key;
    key.AccessGroupParameters().Initialize(Params::instance().DECENT_EL_GAMAL_MODULUS_512, Params::instance().DECENT_EL_GAMAL_GROUP_GENERATOR);
    key.SetPublicElement(publicKey);

    CryptoPP::Integer randomizer(rng, CryptoPP::Integer::One(), Params::instance().DECENT_EL_GAMAL_MODULUS_512 - 1);

    try{
#if CRYPTOPP_VERSION >= 600
        CryptoPP::byte buffer[DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE];
#else
        byte buffer[DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE];
#endif
        message.first.Encode(buffer, DECENT_MESSAGE_SIZE );
        message.second.Encode( buffer+DECENT_MESSAGE_SIZE, DECENT_MESSAGE_SIZE );
        SecByteBlock messageBytes(buffer, DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);

        ElGamal::GroupParameters groupParams;
        groupParams.Initialize(Params::instance().DECENT_EL_GAMAL_MODULUS_512, Params::instance().DECENT_EL_GAMAL_GROUP_GENERATOR);
        ModularArithmetic mr(Params::instance().DECENT_EL_GAMAL_MODULUS_512);

        CryptoPP::Integer m;
        m.Decode(messageBytes, DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);

        result.D1 = groupParams.ExponentiateBase(randomizer);
        result.C1 = mr.Multiply(m, mr.Exponentiate(publicKey, randomizer));
    }
    catch(const CryptoPP::Exception &e) {
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

encryption_results el_gamal_decrypt(const Ciphertext &input, const DInteger &privateKey, point &plaintext)
{
    ElGamalKeys::PrivateKey key;
    key.AccessGroupParameters().Initialize(Params::instance().DECENT_EL_GAMAL_MODULUS_512, Params::instance().DECENT_EL_GAMAL_GROUP_GENERATOR);
    key.SetPrivateExponent(privateKey);
    //elog("el_gamal_decrypt called ${i} ${pk} ",("i", input)("pk", privateKey));
    try{
#if CRYPTOPP_VERSION >= 600
        CryptoPP::byte recovered[DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE];
#else
        byte recovered[DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE];
#endif
        ElGamal::GroupParameters groupParams;
        groupParams.Initialize(Params::instance().DECENT_EL_GAMAL_MODULUS_512, Params::instance().DECENT_EL_GAMAL_GROUP_GENERATOR);

        ModularArithmetic mr(groupParams.GetModulus());

        CryptoPP::Integer s = mr.Exponentiate(input.D1, privateKey);
        CryptoPP::Integer m = mr.Multiply(input.C1, mr.MultiplicativeInverse(s));

        m.Encode(recovered, DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);

        plaintext.first.Decode( recovered, DECENT_MESSAGE_SIZE );
        plaintext.second.Decode( recovered+DECENT_MESSAGE_SIZE, DECENT_MESSAGE_SIZE  );
        //elog("el_gamal_decrypt returns ${m}",("m",plaintext));
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

DInteger hash_elements(Ciphertext t1, Ciphertext t2, DInteger key1, DInteger key2, DInteger G1, DInteger G2, DInteger G3)
{

   CryptoPP::Weak1::MD5 hashier;
   size_t hashSpace =9*(DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);
   hashier.CreateUpdateSpace(hashSpace);
#if CRYPTOPP_VERSION >= 600
   CryptoPP::byte tmp[DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE];
#else
   byte tmp[DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE];
#endif

   t1.D1.Encode(tmp,DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);
   hashier.Update(tmp, DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);

   t1.C1.Encode(tmp,DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);
   hashier.Update(tmp, DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);

   t2.D1.Encode(tmp,DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);
   hashier.Update(tmp, DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);

   t2.C1.Encode(tmp,DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);
   hashier.Update(tmp, DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);

   key1.Encode(tmp,DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);
   hashier.Update(tmp, DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);

   key2.Encode(tmp,DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);
   hashier.Update(tmp, DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);

   G1.Encode(tmp,DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);
   hashier.Update(tmp, DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);

   G2.Encode(tmp,DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);
   hashier.Update(tmp, DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);

   G3.Encode(tmp,DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);
   hashier.Update(tmp, DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);

#if CRYPTOPP_VERSION >= 600
   CryptoPP::byte digest[16];
#else
   byte digest[16];
#endif

   hashier.Final(digest);
   CryptoPP::Integer x(digest, 16);

   return x;
}

bool
verify_delivery_proof(const DeliveryProof &proof, const Ciphertext &first, const Ciphertext &second,
                      const DInteger &firstPulicKey, const DInteger &secondPublicKey)
{
   //elog("verify_delivery_proof called with params ${p} ${f} ${s} ${pk1} ${pk2}",("p", proof)("f", first)("s", second)("pk1", firstPulicKey)("pk2", secondPublicKey));

   ElGamalKeys::PublicKey key1;
   key1.AccessGroupParameters().Initialize(Params::instance().DECENT_EL_GAMAL_MODULUS_512, Params::instance().DECENT_EL_GAMAL_GROUP_GENERATOR);
   key1.SetPublicElement(firstPulicKey);

   ElGamalKeys::PublicKey key2;
   key2.AccessGroupParameters().Initialize(Params::instance().DECENT_EL_GAMAL_MODULUS_512, Params::instance().DECENT_EL_GAMAL_GROUP_GENERATOR);
   key2.SetPublicElement(secondPublicKey);

   DInteger x = hash_elements(first, second, firstPulicKey, secondPublicKey, proof.G1, proof.G2, proof.G3);

   ElGamal::GroupParameters groupParams;
   ModularArithmetic mr(Params::instance().DECENT_EL_GAMAL_MODULUS_512);

   DInteger t1 = mr.Exponentiate(Params::instance().DECENT_EL_GAMAL_GROUP_GENERATOR, proof.s);
   DInteger t2 = mr.Multiply(mr.Exponentiate(key1.GetPublicElement(), x), proof.G1);


   if(t1 != t2)
      return false;
   if (mr.Exponentiate(key1.GetGroupParameters().GetSubgroupGenerator(), proof.r) != mr.Multiply(mr.Exponentiate(second.D1, x), proof.G2))
      return false;

   CryptoPP::Integer c2v = mr.Exponentiate(second.C1, x);
   CryptoPP::Integer c1v = mr.MultiplicativeInverse(mr.Exponentiate(first.C1, x));
   CryptoPP::Integer d1v = mr.Exponentiate(first.D1, proof.s);
   CryptoPP::Integer r2v = mr.Exponentiate(key2.GetPublicElement(), proof.r);
   CryptoPP::Integer c2vc1v =  mr.Multiply(c2v, c1v);
   CryptoPP::Integer c2vc1vd1v = mr.Multiply(c2vc1v, d1v);

   bool ret = mr.Multiply(c2vc1vd1v, mr.MultiplicativeInverse(r2v)) == proof.G3;
   //elog("verify_delivery_proof returns ${r}",("r", ret));
   return ret;
}



encryption_results
encrypt_with_proof(const point &message, const DInteger &privateKey, const DInteger &destinationPublicKey,
                   const Ciphertext &incoming, Ciphertext &outgoing, DeliveryProof &proof)
{

   try{
      //elog("encrypt_with_proof called ${m} ${pk} ${dpk} ${i}",("m", message)("pk", privateKey)("dpk",destinationPublicKey)("i",incoming));
      ElGamalKeys::PrivateKey private_key;
      private_key.AccessGroupParameters().Initialize(Params::instance().DECENT_EL_GAMAL_MODULUS_512, Params::instance().DECENT_EL_GAMAL_GROUP_GENERATOR);
      private_key.SetPrivateExponent(privateKey);

      ElGamalKeys::PublicKey public_key;
      public_key.AccessGroupParameters().Initialize(Params::instance().DECENT_EL_GAMAL_MODULUS_512, Params::instance().DECENT_EL_GAMAL_GROUP_GENERATOR);
      public_key.SetPublicElement(destinationPublicKey);


      CryptoPP::Integer randomizer(rng, CryptoPP::Integer::One(), Params::instance().DECENT_EL_GAMAL_MODULUS_512 - 1);

#if CRYPTOPP_VERSION >= 600
      CryptoPP::byte messageBytes[DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE];
#else
      byte messageBytes[DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE];
#endif
      message.first.Encode(messageBytes, DECENT_MESSAGE_SIZE );
      message.second.Encode( messageBytes+DECENT_MESSAGE_SIZE, DECENT_MESSAGE_SIZE );

      //encrypt message to outgoing
      ElGamal::GroupParameters groupParams;
      groupParams.Initialize(Params::instance().DECENT_EL_GAMAL_MODULUS_512, Params::instance().DECENT_EL_GAMAL_GROUP_GENERATOR);
      ModularArithmetic mr(Params::instance().DECENT_EL_GAMAL_MODULUS_512);


      CryptoPP::Integer m;
      m.Decode(messageBytes, DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE);
      outgoing.D1 = groupParams.ExponentiateBase(randomizer);
      outgoing.C1 = mr.Multiply(m, mr.Exponentiate(destinationPublicKey, randomizer));
      //create delivery proof to proofOfDelivery
      CryptoPP::Integer subgroupOrder = groupParams.GetSubgroupOrder();
      CryptoPP::Integer subGroupGenerator = groupParams.GetSubgroupGenerator();
      CryptoPP::Integer privateExponent =  private_key.GetPrivateExponent();
      CryptoPP::Integer myPublicElement = groupParams.ExponentiateBase(privateExponent);

      CryptoPP::Integer b1(rng, CryptoPP::Integer::One(), subgroupOrder-1);
      CryptoPP::Integer b2(rng, CryptoPP::Integer::One(), subgroupOrder-1);

      proof.G1 = mr.Exponentiate(subGroupGenerator, b1);
      proof.G2 = mr.Exponentiate(subGroupGenerator, b2);
      proof.G3 = mr.Multiply(mr.Exponentiate(incoming.D1, b1), mr.MultiplicativeInverse(mr.Exponentiate(destinationPublicKey, b2)));

      DInteger x = hash_elements(incoming, outgoing, myPublicElement, public_key.GetPublicElement(), proof.G1, proof.G2, proof.G3);

      proof.s = privateExponent * x + b1;
      proof.r = randomizer * x + b2;
      //elog("encrypt_with_proof returns ${o} ${p}",("o", outgoing)("p", proof));

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

void ShamirSecret::calculate_split()
{
   split.clear();
   std::vector<DInteger> coef;
   coef.reserve(quorum);
   coef.push_back(secret);

   ModularArithmetic mr(Params::instance().DECENT_SHAMIR_ORDER);
   for(int i=1; i<quorum; i++)
   {
      CryptoPP::Integer a(rng, CryptoPP::Integer::One(), Params::instance().DECENT_SHAMIR_ORDER);
      coef.push_back(a);
   }

   for(DInteger x=DInteger::One(); x<=shares; x++)
   {
      DInteger y = coef[0];
      for (int i=1; i<quorum; i++)
         y = mr.Add(y, mr.Multiply(coef[i],mr.Exponentiate(x,i)));
      split.push_back(std::make_pair(x,y));
   }
}

void ShamirSecret::calculate_secret()
{
   DInteger res = DInteger::Zero();
   ModularArithmetic mr(Params::instance().DECENT_SHAMIR_ORDER);

   for( const auto& s: split )
   {
      DInteger numerator = DInteger::One();
      DInteger denominator = DInteger::One();
      for (const auto& p: split)
      {
         if ( p == s )
            continue;
         DInteger startposition = s.first;
         DInteger nextposition = mr.Inverse(p.first);
         numerator = mr.Multiply(nextposition, numerator);
         denominator = mr.Multiply(mr.Add(startposition, nextposition), denominator);
      }
      res = mr.Add(mr.Multiply(mr.Multiply(mr.MultiplicativeInverse(denominator), numerator), s.second), res);
   }
   secret = res;
}


}}//namespace
