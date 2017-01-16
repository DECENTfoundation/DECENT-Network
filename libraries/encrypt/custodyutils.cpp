// Copyright (c) 2015-2015 Decent developers

#include <iostream>
#include "decent/encrypt/custodyutils.hpp"
#include "openssl/sha.h"
#include <cmath>
#include <openssl/rand.h>
#include <fc/exception/exception.hpp>
#include <fc/crypto/sha256.hpp>

namespace decent{
namespace crypto{



custody_utils::custody_utils()
{
   pairing_init_set_str(pairing, _DECENT_PAIRING_PARAM_);
   element_t private_key, public_key;

   element_init_G1(generator, pairing);
   //element_init_Zr(private_key, pairing);
   //element_init_G1(public_key, pairing);

   element_set_str(generator, _DECENT_GENERATOR_, 10);
   //element_random(private_key);
   //element_pow_zn(public_key, generator, private_key);
}

custody_utils::~custody_utils()
{
   element_clear(generator);
   pairing_clear(pairing);
}


int custody_utils::get_u_from_seed( const mpz_t &seedU, element_t out[])
{
   mpz_t seed;
   mpz_t seed_tmp;
   mpz_init(seed_tmp);
   mpz_init_set(seed, seedU);

   unsigned char digest[256];

   for (int i = 0; i < DECENT_SECTORS; i++)
   {
      mpz_add_ui(seed_tmp, seed, i);
      mpz_set(seed, seed_tmp);
      element_init_G1(out[i], pairing);

      char *seed_str=mpz_get_str(NULL, 16, seed);
      SHA256((unsigned char *)seed_str, strlen(seed_str), digest);
      free(seed_str);
      element_from_hash(out[i], digest, 32);
   }
   mpz_clear(seed);
   mpz_clear(seed_tmp);
   return 0;
}

int custody_utils::split_file(std::fstream &file, unsigned int &n, element_t ***out)
{
   if(!file.is_open())
      return -1;
   long long position = file.tellg();
   file.seekg (0, file.end);
   long long length = (long long)file.tellg() - position;
   long long realLen = file.tellg();
   file.seekg (position, file.beg);

   n = length / (DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * DECENT_SECTORS);
   if (length % (DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * DECENT_SECTORS))
      n += 1;
   element_t **m = new element_t*[n];
   element_printf("Size of n is: %d\n",n);
   for(int i = 0; i < n; i++)
      m[i] = new element_t[DECENT_SECTORS];

   char buffer[DECENT_SIZE_OF_NUMBER_IN_THE_FIELD];

   for (int i = 0; i < n; i++)
   {
      for (int j = 0; j < DECENT_SECTORS; j++)
      {
         element_init_Zr(m[i][j], pairing);
         if (realLen > ((long long)(file.tellg()) + DECENT_SIZE_OF_NUMBER_IN_THE_FIELD))
         {
            file.read(buffer, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD);
         } else if (file.eof()) {
            memset(buffer,0,DECENT_SIZE_OF_NUMBER_IN_THE_FIELD);
         } else
         {
            int left = realLen - (long long)(file.tellg());
            file.read(buffer, left);
            memset(buffer+left,0, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD-left );
         }
         element_from_bytes(m[i][j], (unsigned char *)buffer);
      }
   }
   *out = m;

   return 0;
}


int custody_utils::get_sigmas(element_t **m, const unsigned int n, element_t u[], element_t& pk, element_t **sigmas)
{
   element_t *ret = new element_t[n];
   element_pp_t * u_pp = new element_pp_t[DECENT_SECTORS];
   for( int k =0; k < DECENT_SECTORS; k++)
      element_pp_init(u_pp[k],u[k]);
   for (int i = 0; i < n; i++)
   {
      element_init_G1(ret[i], pairing);
      element_t temp;
      element_init_G1(temp, pairing);
      for(int j = 0; j < DECENT_SECTORS; j++)
      {
         element_pp_pow_zn(temp, m[i][j], u_pp[j]);
         if (j)
            element_mul(ret[i], ret[i], temp);
         else
            element_set(ret[i], temp);
      }
      element_clear(temp);

      element_t hash;
      element_init_G1(hash, pairing);
      unsigned char buf[256] = "";
      char index[4];
      memset(index,0,4);
      sprintf(index, "%d", i);
      SHA256((unsigned char *)index, 4, buf);
      element_from_hash(hash, buf, 256);
      element_mul(ret[i], ret[i], hash);
      element_pow_zn(ret[i], ret[i], pk);
      element_clear(hash);
   }
   *sigmas = ret;
   for( int k =0; k < DECENT_SECTORS; k++)
      element_pp_clear(u_pp[k]);
   delete[] u_pp;
   return 0;
}

int custody_utils::generate_query_from_seed(mpz_t seed, unsigned int q, unsigned int n, int indices[], element_t* v[])
{
   element_t* ret = new element_t[q]; 
   for (int i = 0; i < q; i++)
   {
      mpz_t seedForIteration;
      mpz_init(seedForIteration);
      mpz_add_ui(seedForIteration, seed, i);

      unsigned char * digest = (unsigned char* ) calloc(32,1);

      char seed_str[mpz_sizeinbase(seedForIteration,16)+1];
      memset((char*) seed_str, 0, mpz_sizeinbase(seedForIteration,16)+1);
      mpz_get_str(seed_str, 16, seedForIteration);
      fc::sha256 temp = fc::sha256::hash( seed_str, mpz_sizeinbase(seedForIteration,16));
      memcpy(digest, temp._hash, (4*sizeof(uint64_t)));

      if (q < 16) //TODO_DECENT
      {
         indices[i] = i;
      }
      else
      {
         memcpy(&indices[i], digest, 8);
         indices[i] = indices[i] % n;
      }
      element_init_Zr(ret[i], pairing);
      element_from_hash(ret[i], digest, 32);
      mpz_clear(seedForIteration);
      free(digest);
   }
   *v = ret;
   return 0;
}


int custody_utils::compute_mu(element_t **m, unsigned int q, int indices[], element_t v[], element_t mu[])
{

   for (int j = 0; j < DECENT_SECTORS; j++)
   {
      element_init_Zr(mu[j], pairing);
      for (int i = 0; i < q; i++)
      {
         element_t temp;
         element_init_Zr(temp, pairing);
         element_mul(temp, v[i], m[indices[i]][j]);
         if ( i > 0 )
            element_add(mu[j], mu[j], temp);
         else
            element_set(mu[j], temp);
         element_clear(temp);
      }
   }

   return 0;
}


int custody_utils::verify(element_t sigma, unsigned int q, int *indices, element_t *v, element_t *u, element_t *mu,
                          element_t pubk)
{
   element_t res1;
   element_init_GT(res1, pairing);
   element_pairing(res1, sigma, generator);

   element_t multi1;
   element_init_G1(multi1, pairing);

   element_t temp;
   element_init_G1(temp, pairing);

   element_t hash;
   element_init_G1(hash, pairing);
   for (int i = 0; i < q; i++)
   {
      unsigned char buf[256] = "";
      char index[4];
      memset(index,0,4);
      sprintf(index, "%d", indices[i]);
      SHA256((unsigned char *)index, 4, buf);
      element_from_hash(hash, buf, 256);
      element_pow_zn(temp, hash, v[i]); //TODO_DECENT optimize
      if (i)
         element_mul(multi1, multi1, temp);
      else
         element_set(multi1, temp);
   }

   element_t multi2;
   element_init_G1(multi2, pairing);

   for (int i = 0; i < DECENT_SECTORS; i++)
   {
      element_pow_zn(temp, u[i], mu[i]);
      if (i)
         element_mul(multi2, multi2, temp);
      else
         element_set(multi2, temp);
   }
   element_clear(temp);

   element_t res2;
   element_init_GT(res2, pairing);
   element_t left2;
   element_init_G1(left2, pairing);

   element_mul(left2, multi1, multi2);
   element_pairing(res2, left2, pubk);

   int res = element_cmp(res1, res2);
   element_clear(hash);
   element_clear(res1);
   element_clear(res2);
   element_clear(multi1);
   element_clear(multi2);
   element_clear(left2);
   return res;
}



int custody_utils::clear_elements(element_t *array, int size)
{
   for (int i = 0; i < size; i++)
   {
      element_clear(array[i]);
   }
   return 0;
}

int custody_utils::unpack_proof(valtype proof, element_t &sigma, element_t **mu)
{
   int pointer = 0;
   element_init_G1(sigma, pairing);
   element_from_bytes_compressed(sigma, proof.data() + pointer);
   pointer += DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED;

   element_t *ret = new element_t[DECENT_SECTORS];
   for (int i = 0; i < DECENT_SECTORS; i++)
   {
      element_init_Zr(ret[i], pairing);
      element_from_bytes(ret[i], proof.data() + pointer);
      pointer += DECENT_SIZE_OF_NUMBER_IN_THE_FIELD;
   }
   *mu = ret;
   return 0;
}

int custody_utils::compute_sigma(element_t sigmas[], unsigned int q, int indices[], element_t v[], element_t &sigma)
{
   element_init_G1(sigma, pairing);

   for (int i = 0; i < q; i++)
   {
      element_t temp;
      element_init_G1(temp ,pairing);
      element_pow_zn(temp, sigmas[indices[i]], v[i]);
      if (i)
         element_mul(sigma, sigma, temp);
      else
         element_set(sigma, temp);
      element_clear(temp);
   }
   return 0;
}


int custody_utils::get_number_of_query(int blocks)
{
   int questions = blocks;
   // TODO: needs research and optimization
   if (questions > 16)
      questions = 16;
   return questions;
}


int custody_utils::verify_by_miner(const uint32_t &n, const char *u_seed, unsigned char *pubKey, unsigned char sigma[],
                                   std::vector<std::vector<unsigned char>> mus, mpz_t seed)
{
   //prepate public_key and u
   element_t public_key;
   element_init_G1(public_key, pairing);
   element_from_bytes_compressed(public_key, pubKey);
   element_t u[DECENT_SECTORS];

   mpz_t seedForU;

   char* buf_str = (char*) malloc (32 + 1);
   char* buf_ptr = buf_str;

   for (int i = 0; i < 16; i++)
   {
      buf_ptr += sprintf(buf_ptr, "%X", (unsigned char)u_seed[i]);
   }

   mpz_init_set_str(seedForU, buf_str, 16);
   free(buf_str);
   get_u_from_seed(seedForU, u);


   //prepare sigma and mu
   element_t _sigma;
   element_t mu[DECENT_SECTORS];

   element_init_G1(_sigma, pairing);
   element_from_bytes_compressed(_sigma, sigma);
   for (int i = 0; i < DECENT_SECTORS; i++)
   {
      element_init_Zr(mu[i], pairing);
      element_from_bytes(mu[i], reinterpret_cast<unsigned char*>(mus[i].data())); 
   }


   unsigned int q = get_number_of_query( n );
   int indices[q];
   element_t* v;

   generate_query_from_seed(seed, q, n, indices, &v);

   int res = verify(_sigma, q, indices, v, u, mu, public_key);

   clear_elements(u, DECENT_SECTORS);
   clear_elements(mu, DECENT_SECTORS);
   clear_elements(v, q);
   element_clear(public_key);
   mpz_clear(seedForU);
   element_clear( _sigma );
   delete(v);

   return res;
}

int custody_utils::create_custody_data(path content, uint32_t& n, char u_seed[], unsigned char pubKey[])
{
   //prepare the files
   std::fstream infile(content.c_str(), std::fstream::binary | std::fstream::in );
   std::ofstream outfile ( (content.parent_path() / "content.cus").c_str(), std::fstream::binary | std::ios_base::trunc );
   infile.seekg(0, infile.beg);
   outfile.seekp(0);

   //std:: cout<< "Pairing export lenght: "<<pairing_length_in_bytes_compressed_G1(pairing)<<"\n";
   //prepare elements _u, m, seedForU and keys
   element_t **m;
   element_t u[DECENT_SECTORS];
   element_t private_key, public_key;
   element_t *sigmas;  //TODO_DECENT
   mpz_t seedForU;

   element_init_Zr(private_key, pairing);
   element_init_G1(public_key, pairing);

   //Set the elements
   if (RAND_bytes((unsigned char*) u_seed, 16) != 1) {
      abort();
   }
   char* buf_str = (char*) malloc (32 + 1);
   char* buf_ptr = buf_str;

   for (int i = 0; i < 16; i++)
   {
      buf_ptr += sprintf(buf_ptr, "%X", (unsigned char)u_seed[i]);
   }
   buf_str[32]=0;

   mpz_init_set_str(seedForU, buf_str, 16);
   free(buf_str);

   get_u_from_seed(seedForU, u);


   element_random(private_key);
   element_pow_zn(public_key, generator, private_key);

   //create the actual signatures in sigmas

   split_file(infile, n, &m);

   get_sigmas(m, n, u, private_key, &sigmas);

   //save the values to u_seed and pubKey
   element_to_bytes_compressed(pubKey, public_key);

   //Save the signatures file
   char buffer[DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED];
   for (int i = 0; i < n; i++)
   {
      element_to_bytes_compressed((unsigned char *)buffer, sigmas[i]);
      outfile.write(buffer, DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED);
   }

   //Clean
   element_printf("Size of n is: %d\n",n);

   for (int i = 0; i < n; i++)
   {
      clear_elements(m[i], DECENT_SECTORS);
      delete[](m[i]);
   }
   delete[] m;
   clear_elements(u, DECENT_SECTORS);
   element_clear(private_key);
   element_clear(public_key);
   clear_elements(sigmas, n);
   delete[](sigmas);
   mpz_clear(seedForU);
   outfile.close();
   infile.close();
   return 0;
}

int custody_utils::create_proof_of_custody(path content, const uint32_t n, unsigned char pubKey[], const char u_seed[],
                                           unsigned char sigma[],
                                           std::vector<std::vector<unsigned char>>& mus, mpz_t seed)
{
   //open files
   std::fstream infile(content.c_str(), std::fstream::binary | std::fstream::in);
   std::fstream cusfile((content.parent_path() / "content.cus").c_str(),std::fstream::binary | std::fstream::in);
   if (!infile.is_open() || !cusfile.is_open())
      return -1;
   cusfile.seekg(0);
   infile.seekg(0);

   //prepate public_key and u
   element_t public_key;
   element_init_G1(public_key, pairing);
   element_from_bytes_compressed(public_key, pubKey);
   element_t u[DECENT_SECTORS];

   char* buf_str = (char*) malloc (32 + 1);
   char* buf_ptr = buf_str;

   for (int i = 0; i < 16; i++)
   {
      buf_ptr += sprintf(buf_ptr, "%X", (unsigned char)u_seed[i]);
   }
   buf_str[32]=0;

   mpz_t seedForU;

   mpz_init_set_str(seedForU, buf_str, 16);
   free(buf_str);
   get_u_from_seed(seedForU, u);


   //read sigmas
   element_t sigmas[n];
   char buffer[DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED];
   for (int i = 0; i < n; i++)
   {
      cusfile.read(buffer, DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED);
      element_init_G1(sigmas[i], pairing);
      element_from_bytes_compressed(sigmas[i], (unsigned char *)buffer);
   }

   //read the file and get m's
   element_t **m;
   unsigned int nReal;
   split_file(infile, nReal, &m);
   if(nReal!=n)
      return -1;

   //generate query
   unsigned int q = get_number_of_query(n);
   int indices[16];
   element_t * v;
   generate_query_from_seed(seed, q, n, indices, &v);

   //calculate mu and sigma
   element_t mu[DECENT_SECTORS];
   compute_mu(m, q, indices, v, mu);

   element_t _sigma;
   compute_sigma(sigmas, q, indices, v, _sigma);

   //pack the proof
   element_to_bytes_compressed(sigma, _sigma);
   mus.clear();
   unsigned char buffer_mu [DECENT_SIZE_OF_NUMBER_IN_THE_FIELD];
   for (int i = 0; i < DECENT_SECTORS; i++)
   {
      element_to_bytes(buffer_mu, mu[i]);
      std::vector<unsigned char> tmp(buffer_mu, buffer_mu + DECENT_SIZE_OF_NUMBER_IN_THE_FIELD);
      mus.push_back(tmp);
   }

   //TODO_DECENT
   int res = verify_by_miner(n, u_seed, pubKey, sigma, mus, seed);

   //clean the mess
   for (int i = 0; i < n; i++)
   {
      clear_elements(m[i], DECENT_SECTORS);
      delete[](m[i]);
   }
   delete[] m;

   clear_elements(sigmas, n);
   element_clear(_sigma);

   element_clear(public_key);
   mpz_clear(seedForU);
   clear_elements(u, DECENT_SECTORS);
   clear_elements(mu, DECENT_SECTORS);
   clear_elements(v, q);
   delete(v);
   return res;
}


}}
