// Copyright (c) 2015-2015 Decent developers

#include <cstddef>
#include <iostream>
#include "decent/encrypt/custodyutils.hpp"
#include <openssl/rand.h>
#include <fc/crypto/sha256.hpp>
#include <sstream>
#include <iomanip>
#include <fc/thread/thread.hpp>

#define DECENT_CUSTODY_THREADS 4
//#define _CUSTODY_STATS
namespace decent {
namespace crypto {

namespace {


std::string bytes_to_string(unsigned char *data, int len) {
   std::stringstream ss;
   ss << std::hex << std::setfill('0');;
   for( int i = 0; i < len; ++i )
      ss << std::setw(2) << (int) data[i];
   return ss.str();
}

void string_to_bytes(std::string &in, unsigned char data[], int len) {

   const char *s = in.c_str();
   for( int i = 0; i < len && i * 2 < in.size(); ++i ) {
      char ch1 = s[i * 2];
      char ch2 = s[i * 2 + 1];
      unsigned char res =
            ((ch1 >= 'a') ? (ch1 - 'a' + 10) : (ch1 - '0')) * 16 + ((ch2 >= 'a') ? (ch2 - 'a' + 10) : (ch2 - '0'));
      data[i] = res;
   }
}
#ifdef _CUSTODY_STATS
int mul = 0;
int pow = 0;
int pow_pp = 0;
int add = 0;
#endif
}

custody_utils::custody_utils() {
   pairing_init_set_str(pairing, _DECENT_PAIRING_PARAM_);

   element_init_G1(generator, pairing);

   element_set_str(generator, _DECENT_GENERATOR_, 10);
}

custody_utils::~custody_utils() {
   element_clear(generator);
   pairing_clear(pairing);
#ifdef _CUSTODY_STATS
   std::cout <<"Custodyuitls stats: mul: " << mul <<" pow: "<<pow<<" pow_pp: "<<pow_pp<< " add: "<<add<<"\n";
#endif
}


int custody_utils::get_u_from_seed(const mpz_t &seedU, element_t out[]) {
   mpz_t seed;
   mpz_t seed_tmp;
   mpz_init(seed_tmp);
   mpz_init_set(seed, seedU);

   unsigned char digest[32];
   memset(digest, 0, 32);

   for( int i = 0; i < DECENT_SECTORS; i++ ) {
      mpz_add_ui(seed_tmp, seed, i);
      mpz_set(seed, seed_tmp);
      element_init_G1(out[i], pairing);

      char *seed_str = mpz_get_str(NULL, 16, seed);
      fc::sha256 temp = fc::sha256::hash(seed_str, mpz_sizeinbase(seed, 16));
      memcpy(digest, temp._hash, (4 * sizeof(uint64_t)));
      free(seed_str);
      element_from_hash(out[i], digest, 32);
   }
   mpz_clear(seed);
   mpz_clear(seed_tmp);
   return 0;
}

int custody_utils::get_n(std::fstream &file) {
   if( !file.is_open())
      return -1;
   file.seekg(0, file.end);
   long long length = (long long) file.tellg();
   int n = length / (DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * DECENT_SECTORS);
   if( length % (DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * DECENT_SECTORS))
      n += 1;
   return n;
}

int custody_utils::split_file(std::fstream &file, unsigned int &n, element_t ***out) {
   n = get_n(file);
   file.seekg(0, file.end);
   long long realLen = file.tellg();
   file.seekg(0);

   element_t **m = new element_t *[n];
   element_printf("Size of n is: %d\n", n);
   for( int i = 0; i < n; i++ )
      m[i] = new element_t[DECENT_SECTORS];

   char buffer[DECENT_SIZE_OF_NUMBER_IN_THE_FIELD];

   for( int i = 0; i < n; i++ ) {
      for( int j = 0; j < DECENT_SECTORS; j++ ) {
         element_init_Zr(m[i][j], pairing);
         if( realLen > ((long long) (file.tellg()) + DECENT_SIZE_OF_NUMBER_IN_THE_FIELD)) {
            file.read(buffer, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD);
         } else if( file.eof()) {
            memset(buffer, 0, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD);
         } else {
            int left = realLen - (long long) (file.tellg());
            file.read(buffer, left);
            memset(buffer + left, 0, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD - left);
         }
         element_from_bytes(m[i][j], (unsigned char *) buffer);
      }
   }
   *out = m;

   return 0;
}

inline int custody_utils::get_data(std::fstream &file, uint32_t i, char buffer[]) {
   uint64_t position = DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * DECENT_SECTORS * i;

   file.seekg(0, file.end);
   uint64_t realLen = file.tellg();
   file.seekg(std::min(position, realLen), file.beg);

   if( realLen > ((long long) (file.tellg()) + DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * DECENT_SECTORS))
      file.read(buffer, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * DECENT_SECTORS);
   else {
      if( file.eof())
         memset(buffer, 0, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * DECENT_SECTORS);
      else {
         int left = realLen - (long long) (file.tellg());
         file.read(buffer, left);
         memset(buffer + left, 0, (DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * DECENT_SECTORS) - left);
      }
   }

}


inline int custody_utils::get_m(std::fstream &file, uint32_t i, uint32_t j, mpz_t &out) {
   mpz_init2(out, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * 8);
   uint64_t position = DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * (j + DECENT_SECTORS * i);
   char buffer[DECENT_SIZE_OF_NUMBER_IN_THE_FIELD];
   file.seekg(0, file.end);
   uint64_t realLen = file.tellg();
   if( realLen < position ) { //we are either close or behinf EoF
      memset(buffer, 0, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD);
   } else {
      file.seekg(position, file.beg);
      if( realLen < position + DECENT_SIZE_OF_NUMBER_IN_THE_FIELD ) { //we can't read the whole string
         int left = realLen - (uint64_t) (file.tellg());
         file.read(buffer, left);
         memset(buffer + left, 0, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD - left);
      } else {
         file.read(buffer, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD);
      }
   }

   //mpz_import is too slow for our purposes - since we don't care about the exact parameters as much as about the uniqueness of the import, let's replace it with memcpy
   memcpy((char *) out->_mp_d, buffer, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD);
   out->_mp_size = DECENT_MP_SIZE_OF_NUMBER_IN_THE_FIELD;
   //mpz_import( out, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD, 1, 1, 1, 0, buffer );
   return 1;
}

int custody_utils::get_sigma(uint64_t idx, mpz_t mi[], element_pp_t u_pp[], element_t pk, element_t out[]) {
   element_t temp;
   element_init_G1(temp, pairing);
   element_init_G1(out[idx], pairing);
   for( int j = 0; j < DECENT_SECTORS; j++ ) {
      element_pp_pow(temp, mi[j], u_pp[j]);
#ifdef _CUSTODY_STATS
      pow_pp++;
#endif
      mpz_clear(mi[j]);
      if( j ) {
         element_mul(out[idx], out[idx], temp);
#ifdef _CUSTODY_STATS
         mul++;
#endif
      }else
         element_set(out[idx], temp);
   }
   element_clear(temp);

   element_t hash;
   element_init_G1(hash, pairing);
   unsigned char buf[32];
   memset(buf, 0, 32);
   char index[16];
   memset(index, 0, 16);
   sprintf(index, "%llu", idx);
   fc::sha256 stemp = fc::sha256::hash(index, 16);
   memcpy(buf, stemp._hash, (4 * sizeof(uint64_t)));
   element_from_hash(hash, buf, 32);
   element_mul(out[idx], out[idx], hash);
   element_pow_zn(out[idx], out[idx], pk);
#ifdef _CUSTODY_STATS
   pow++;
   mul++;
#endif
   element_clear(hash);
   return 1;
}

int
custody_utils::get_sigmas(std::fstream &file, const unsigned int n, element_t *u, element_t pk, element_t **sigmas) {
   element_t *ret = new element_t[n];
   element_pp_t *u_pp = new element_pp_t[DECENT_SECTORS];
   for( int k = 0; k < DECENT_SECTORS; k++ )
      element_pp_init(u_pp[k], u[k]);
   //start threads
   fc::thread t[DECENT_CUSTODY_THREADS];
   fc::future<int> fut[DECENT_CUSTODY_THREADS];

//   std::cout<<"get_sigmas: n = "<<n<<" , cycles = " << cycles <<"\n";
   int total_thread_to_wait_for = std::min( n, (const unsigned int) DECENT_CUSTODY_THREADS );

   for( uint64_t i = 0; i < n; i += DECENT_CUSTODY_THREADS ) {
      int iterations = std::min ((uint64_t) DECENT_CUSTODY_THREADS, n - i ); 
      for( int k = 0; k < iterations ; ++k ) {
         uint64_t idx = i + k;

         //we read the file in the main thread...
         char *buffer = new char[(DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * DECENT_SECTORS)];
         get_data(file, idx, buffer);
         //and distribute the tasks
         fut[k] = t[k].async([=]() {
              mpz_t m[DECENT_SECTORS];
              for( int i = 0; i < DECENT_SECTORS; ++i ) {
                 mpz_init2(m[i], DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * 8);
                 //mpz_import is too slow for our purposes - since we don't care about the exact parameters as much as about the uniqueness of the import, let's replace it with memcpy
                 memcpy((char *) m[i]->_mp_d, buffer + i * DECENT_SIZE_OF_NUMBER_IN_THE_FIELD,
                        DECENT_SIZE_OF_NUMBER_IN_THE_FIELD);
                 m[i]->_mp_size = DECENT_MP_SIZE_OF_NUMBER_IN_THE_FIELD;
                 //mpz_import(m[i], DECENT_SIZE_OF_NUMBER_IN_THE_FIELD, 1, 1, 1, 0, buffer + i * DECENT_SIZE_OF_NUMBER_IN_THE_FIELD);
              }
              delete[] buffer;
              return get_sigma(idx, m, u_pp, pk, ret);
         });
      }
   }

   for( int k = 0; k < total_thread_to_wait_for; ++k )
      fut[k].wait();


   *sigmas = ret;
   for( int k = 0; k < DECENT_SECTORS; k++ )
      element_pp_clear(u_pp[k]);
   delete[] u_pp;
   return 0;
}


int custody_utils::compute_mu(std::fstream &file, unsigned int q, uint64_t indices[], element_t v[], element_t mu[]) {

   for( int j = 0; j < DECENT_SECTORS; j++ ) {
      element_init_Zr(mu[j], pairing);
      for( int i = 0; i < q; i++ ) {
         element_t temp;
         mpz_t m;
         element_init_Zr(temp, pairing);
         get_m(file, indices[i], j, m);
         element_mul_mpz(temp, v[i], m);
#ifdef _CUSTODY_STATS
         mul++;
#endif
         if( i > 0 ) {
            element_add(mu[j], mu[j], temp);
#ifdef _CUSTODY_STATS
            add++;
#endif
         }else
            element_set(mu[j], temp);
         element_clear(temp);
         mpz_clear(m);
      }
   }

   return 0;
}

int custody_utils::generate_query_from_seed(mpz_t seed, unsigned int q, unsigned int n, uint64_t indices[],
                                            element_t *v[]) {
   element_t *ret = new element_t[q];
   for( int i = 0; i < q; i++ ) {
      mpz_t seedForIteration;
      mpz_init(seedForIteration);
      mpz_add_ui(seedForIteration, seed, i);

      unsigned char *digest = (unsigned char *) calloc(32, 1);

      char seed_str[mpz_sizeinbase(seedForIteration, 16) + 1];
      memset((char *) seed_str, 0, mpz_sizeinbase(seedForIteration, 16) + 1);
      mpz_get_str(seed_str, 16, seedForIteration);
      fc::sha256 temp = fc::sha256::hash(seed_str, mpz_sizeinbase(seedForIteration, 16));
      memcpy(digest, temp._hash, (4 * sizeof(uint64_t)));

      if( q < 16 ) //TODO_DECENT
      {
         indices[i] = i;
      } else {
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


int custody_utils::verify(element_t sigma, unsigned int q, uint64_t *indices, element_t *v, element_t *u, element_t *mu,
                          element_t pubk) {
   element_t res1;
   element_init_GT(res1, pairing);
   element_pairing(res1, sigma, generator);

   element_t multi1;
   element_init_G1(multi1, pairing);

   element_t temp;
   element_init_G1(temp, pairing);

   element_t hash;
   element_init_G1(hash, pairing);
   for( int i = 0; i < q; i++ ) {
      unsigned char buf[32];
      memset(buf, 0, 32);
      char index[16];
      memset(index, 0, 16);
      sprintf(index, "%lu", indices[i]);
      fc::sha256 stemp = fc::sha256::hash(index, 16);
      memcpy(buf, stemp._hash, (4 * sizeof(uint64_t)));
      element_from_hash(hash, buf, 32);
      element_pow_zn(temp, hash, v[i]); //TODO_DECENT optimize
#ifdef _CUSTODY_STATS
      pow++;
#endif
      if( i ) {
         element_mul(multi1, multi1, temp);
#ifdef _CUSTODY_STATS
         mul++;
#endif
      }else
         element_set(multi1, temp);
   }

   element_t multi2;
   element_init_G1(multi2, pairing);

   for( int i = 0; i < DECENT_SECTORS; i++ ) {
      element_pow_zn(temp, u[i], mu[i]);
#ifdef _CUSTODY_STATS
      pow++;
#endif
      if( i ) {
         element_mul(multi2, multi2, temp);
#ifdef _CUSTODY_STATS
         mul++;
#endif
      }else
         element_set(multi2, temp);
   }
   element_clear(temp);

   element_t res2;
   element_init_GT(res2, pairing);
   element_t left2;
   element_init_G1(left2, pairing);

   element_mul(left2, multi1, multi2);
#ifdef _CUSTODY_STATS
   mul++;
#endif
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


int custody_utils::clear_elements(element_t *array, int size) {
   for( int i = 0; i < size; i++ ) {
      element_clear(array[i]);
   }
   return 0;
}

int custody_utils::unpack_proof(valtype proof, element_t &sigma, element_t **mu) {
   int pointer = 0;
   element_init_G1(sigma, pairing);
   element_from_bytes_compressed(sigma, proof.data() + pointer);
   pointer += DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED;

   element_t *ret = new element_t[DECENT_SECTORS];
   for( int i = 0; i < DECENT_SECTORS; i++ ) {
      element_init_Zr(ret[i], pairing);
      element_from_bytes(ret[i], proof.data() + pointer);
      pointer += DECENT_SIZE_OF_NUMBER_IN_THE_FIELD;
   }
   *mu = ret;
   return 0;
}

int
custody_utils::compute_sigma(element_t sigmas[], unsigned int q, uint64_t indices[], element_t v[], element_t &sigma) {
   element_init_G1(sigma, pairing);
   element_set1(sigma);

   element_t temp;
   element_init_G1(temp, pairing);

   for( int i = 0; i < q; i++ ) {
      element_pow_zn(temp, sigmas[indices[i]], v[i]);
      element_mul(sigma, sigma, temp);
#ifdef _CUSTODY_STATS
      pow++;
      mul++;
#endif
   }
   element_clear(temp);
   return 0;
}


int custody_utils::get_number_of_query(int blocks) {
   int questions = blocks;
   // TODO: needs research and optimization
   if( questions > 16 )
      questions = 16;
   return questions;
}


int custody_utils::verify_by_miner(const uint32_t &n, const char *u_seed, unsigned char *pubKey, unsigned char sigma[],
                                   std::vector<std::string> mus, mpz_t seed) {
   //prepate public_key and u
   element_t public_key;
   element_init_G1(public_key, pairing);
   element_from_bytes_compressed(public_key, pubKey);
   element_t u[DECENT_SECTORS];

   mpz_t seedForU;

   char *buf_str = (char *) malloc(32 + 1);
   char *buf_ptr = buf_str;

   for( int i = 0; i < 16; i++ ) {
      buf_ptr += sprintf(buf_ptr, "%X", (unsigned char) u_seed[i]);
   }

   mpz_init_set_str(seedForU, buf_str, 16);
   free(buf_str);
   get_u_from_seed(seedForU, u);


   //prepare sigma and mu
   element_t _sigma;
   element_t mu[DECENT_SECTORS];

   element_init_G1(_sigma, pairing);
   element_from_bytes_compressed(_sigma, sigma);

   for( int i = 0; i < DECENT_SECTORS; i++ ) {
      element_init_Zr(mu[i], pairing);
      unsigned char buffer[DECENT_SIZE_OF_MU];
      string_to_bytes(mus[i], buffer, DECENT_SIZE_OF_MU);
      element_from_bytes(mu[i], buffer);
   }

   unsigned int q = get_number_of_query(n);
   uint64_t indices[q];
   element_t *v;

   generate_query_from_seed(seed, q, n, indices, &v);

   int res = verify(_sigma, q, indices, v, u, mu, public_key);

   clear_elements(u, DECENT_SECTORS);
   clear_elements(mu, DECENT_SECTORS);
   clear_elements(v, q);
   element_clear(public_key);
   mpz_clear(seedForU);
   element_clear(_sigma);
   delete[](v);

   return res;
}

int custody_utils::create_custody_data(path content, uint32_t &n, char u_seed[], unsigned char pubKey[]) {
   //prepare the files
   std::fstream infile(content.c_str(), std::fstream::binary | std::fstream::in);
   std::ofstream outfile((content.parent_path() / "content.cus").c_str(), std::fstream::binary | std::ios_base::trunc);
   infile.seekg(0, infile.beg);
   outfile.seekp(0);

   //prepare elements _u, m, seedForU and keys
   element_t **m;
   element_t u[DECENT_SECTORS];
   element_t private_key, public_key;
   element_t *sigmas;  //TODO_DECENT
   mpz_t seedForU;

   element_init_Zr(private_key, pairing);
   element_init_G1(public_key, pairing);

   //Set the elements
   if( RAND_bytes((unsigned char *) u_seed, 16) != 1 ) {
      FC_THROW("Error creating random data");
   }
   char *buf_str = (char *) malloc(32 + 1);
   char *buf_ptr = buf_str;

   for( int i = 0; i < 16; i++ ) {
      buf_ptr += sprintf(buf_ptr, "%X", (unsigned char) u_seed[i]);
   }
   buf_str[32] = 0;

   mpz_init_set_str(seedForU, buf_str, 16);
   free(buf_str);

   get_u_from_seed(seedForU, u);


   element_random(private_key);
   element_pow_zn(public_key, generator, private_key);
#ifdef _CUSTODY_STATS
   pow++;
#endif

   //create the actual signatures in sigmas

   //split_file(infile, n, &m);
   n = get_n(infile);

   get_sigmas(infile, n, u, private_key, &sigmas);

   //save the values to u_seed and pubKey
   element_to_bytes_compressed(pubKey, public_key);

   //Save the signatures file
   char buffer[DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED];
   for( int i = 0; i < n; i++ ) {
      element_to_bytes_compressed((unsigned char *) buffer, sigmas[i]);
      outfile.write(buffer, DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED);
   }

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

int custody_utils::create_proof_of_custody(path content, const uint32_t n, const char u_seed[], unsigned char pubKey[],
                                           unsigned char sigma[], std::vector<std::string> &mus, mpz_t seed) {
   //open files
   std::fstream infile(content.c_str(), std::fstream::binary | std::fstream::in);
   std::fstream cusfile((content.parent_path() / "content.cus").c_str(), std::fstream::binary | std::fstream::in);
   if( !infile.is_open() || !cusfile.is_open())
      return -1;
   cusfile.seekg(0);
   infile.seekg(0);

   //prepate public_key and u
   element_t public_key;
   element_init_G1(public_key, pairing);
   element_from_bytes_compressed(public_key, pubKey);
   element_t u[DECENT_SECTORS];

   char *buf_str = (char *) malloc(32 + 1);
   char *buf_ptr = buf_str;

   for( int i = 0; i < 16; i++ ) {
      buf_ptr += sprintf(buf_ptr, "%X", (unsigned char) u_seed[i]);
   }
   buf_str[32] = 0;

   mpz_t seedForU;

   mpz_init_set_str(seedForU, buf_str, 16);
   free(buf_str);
   get_u_from_seed(seedForU, u);


   //read sigmas
   element_t sigmas[n];
   char buffer[DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED];
   for( int i = 0; i < n; i++ ) {
      cusfile.read(buffer, DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED);
      element_init_G1(sigmas[i], pairing);
      element_from_bytes_compressed(sigmas[i], (unsigned char *) buffer);
   }

   //read the file and get m's
   element_t **m;
   unsigned int nReal;
   nReal = get_n(infile);
   //split_file(infile, nReal, &m);
   if( nReal != n )
      return -1;

   //generate query
   unsigned int q = get_number_of_query(n);
   uint64_t indices[16];
   element_t *v;
   generate_query_from_seed(seed, q, n, indices, &v);

   //calculate mu and sigma
   element_t mu[DECENT_SECTORS];
   compute_mu(infile, q, indices, v, mu);

   element_t _sigma;
   compute_sigma(sigmas, q, indices, v, _sigma);

   //pack the proof
   element_to_bytes_compressed(sigma, _sigma);
   mus.clear();
   //TODO_DECENT change the following code to store in hexadecimal string
   unsigned char buffer_mu[DECENT_SIZE_OF_MU];

   for( int i = 0; i < DECENT_SECTORS; i++ ) {
      memset(buffer_mu, 0, DECENT_SIZE_OF_MU);
      element_to_bytes(buffer_mu, mu[i]);
      std::string s = bytes_to_string(buffer_mu, DECENT_SIZE_OF_MU);
      mus.push_back(s);
   }

   //TODO_DECENT
   int res = verify_by_miner(n, u_seed, pubKey, sigma, mus, seed);

   clear_elements(sigmas, n);
   element_clear(_sigma);

   element_clear(public_key);
   mpz_clear(seedForU);
   clear_elements(u, DECENT_SECTORS);
   clear_elements(mu, DECENT_SECTORS);
   clear_elements(v, q);
   delete[](v);
   return res;
}


}
}
