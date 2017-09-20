/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
// Copyright (c) 2015-2015 Decent developers
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
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
namespace encrypt {

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

CustodyUtils::CustodyUtils() {
   pairing_init_set_str(pairing, _DECENT_PAIRING_PARAM_);

   element_init_G1(generator, pairing);

   element_set_str(generator, _DECENT_GENERATOR_, 10);
}

CustodyUtils::~CustodyUtils() {
   element_clear(generator);
   pairing_clear(pairing);
#ifdef _CUSTODY_STATS
   std::cout <<"Custodyuitls stats: mul: " << mul <<" pow: "<<pow<<" pow_pp: "<<pow_pp<< " add: "<<add<<"\n";
#endif
}


int CustodyUtils::get_u_from_seed(const mpz_t &seedU, element_s out[][1], uint32_t size) {
   mpz_t seed;
   mpz_t seed_tmp;
   mpz_init(seed_tmp);
   mpz_init_set(seed, seedU);

   unsigned char digest[32];
   memset(digest, 0, 32);

   for( int i = 0; i < size; i++ ) {
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

int CustodyUtils::get_n(std::fstream &file, uint32_t size) {

   if( !file.is_open())
      return -1;
   file.seekg(0, file.end);
   long long length = (long long) file.tellg();
   int n = length / (DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * size);
   if( length % (DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * size))
      n += 1;

   return n;
}

inline int CustodyUtils::get_data(std::fstream &file, uint32_t i, char buffer[], uint32_t size) {
   uint64_t position = DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * size * i;

   file.seekg(0, file.end);
   uint64_t realLen = file.tellg();
   file.seekg(std::min(position, realLen), file.beg);

   if( realLen > ((long long) (file.tellg()) + DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * size))
      file.read(buffer, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * size);
   else {
      if( file.eof())
         memset(buffer, 0, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * size);
      else {
         int left = realLen - (long long) (file.tellg());
         file.read(buffer, left);
         memset(buffer + left, 0, (DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * size) - left);
      }
   }
   return 0;
}


inline int CustodyUtils::get_m(std::fstream &file, uint32_t i, uint32_t j, mpz_t &out, uint32_t size) {
   mpz_init2(out, DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * 8);
   uint64_t position = DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * (j + size * i);
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

int CustodyUtils::get_sigma(uint64_t idx, __mpz_struct mi[][1], element_pp_s u_pp[][1], element_t pk, element_s out[][1],
                            uint32_t size) {
   element_t temp;
   element_init_G1(temp, pairing);
   element_init_G1(out[idx], pairing);
   {
      int j=0;
      element_pp_pow(out[idx], mi[j], u_pp[j]);
#ifdef _CUSTODY_STATS
      pow_pp++;
#endif
   }
   for( int j = 1; j < size; j++ ) {
      element_pp_pow(temp, mi[j], u_pp[j]);
#ifdef _CUSTODY_STATS
      pow_pp++;
#endif
      mpz_clear(mi[j]);
      element_mul(out[idx], out[idx], temp);
#ifdef _CUSTODY_STATS
      mul++;
#endif
   }
   element_clear(temp);

   element_t hash;
   element_init_G1(hash, pairing);

   char index[16];
   memset(index, 0, 16);
   sprintf(index, "%llu", idx);

   unsigned char buf[32];
   memset(buf, 0, 32);
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
CustodyUtils::get_sigmas(std::fstream &file, const unsigned int n, element_t *u, element_t pk, element_t **sigmas,
                         uint32_t size) {
   element_t *ret = new element_t[n];
   //start threads
   fc::thread t[DECENT_CUSTODY_THREADS];
   fc::future<int> fut[DECENT_CUSTODY_THREADS];
   fc::future<void> fut_pp[DECENT_CUSTODY_THREADS];

   element_pp_t *u_pp = new element_pp_t[size];
   for( int k = 0; k < size; k++ ) {
      int idx = k % DECENT_CUSTODY_THREADS;
      fut_pp[idx] = t[idx].async([=](){ element_pp_init(u_pp[k], u[k]); return; });
   }
   //wait for the threads...
   for( int k = 0; k < DECENT_CUSTODY_THREADS; ++k )
      fut_pp[k].wait();

//   std::cout<<"get_sigmas: n = "<<n<<" , cycles = " << cycles <<"\n";
   int total_thread_to_wait_for = std::min( n, (const unsigned int) DECENT_CUSTODY_THREADS );

   for( uint64_t i = 0; i < n; i += DECENT_CUSTODY_THREADS ) {
      int iterations = std::min ((uint64_t) DECENT_CUSTODY_THREADS, n - i ); 
      for( int k = 0; k < iterations ; ++k ) {
         uint64_t idx = i + k;

         //we read the file in the main thread...
         char *buffer = new char[(DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * size)];
         get_data(file, idx, buffer, size);
         //and distribute the tasks
         fut[k] = t[k].async([=]() {

              mpz_t * m = new mpz_t[size];
              for( int i = 0; i < size; ++i ) {
                 mpz_init2(m[i], DECENT_SIZE_OF_NUMBER_IN_THE_FIELD * 8);
                 //mpz_import is too slow for our purposes - since we don't care about the exact parameters as much as about the uniqueness of the import, let's replace it with memcpy
                 memcpy((char *) m[i]->_mp_d, buffer + i * DECENT_SIZE_OF_NUMBER_IN_THE_FIELD,
                        DECENT_SIZE_OF_NUMBER_IN_THE_FIELD);
                 m[i]->_mp_size = DECENT_MP_SIZE_OF_NUMBER_IN_THE_FIELD;
                 //mpz_import(m[i], DECENT_SIZE_OF_NUMBER_IN_THE_FIELD, 1, 1, 1, 0, buffer + i * DECENT_SIZE_OF_NUMBER_IN_THE_FIELD);
              }
              delete[] buffer;
              int retval =  get_sigma(idx, m, u_pp, pk, ret, 0);
              delete[]m;
              return retval;
         });
      }
   }
   //wait for the threads...
   for( int k = 0; k < total_thread_to_wait_for; ++k )
      fut[k].wait();

   *sigmas = ret;
   for( int k = 0; k < size; k++ )
      element_pp_clear(u_pp[k]);
   delete[] u_pp;
   return 0;
}


int CustodyUtils::compute_mu(std::fstream &file, unsigned int q, uint64_t indices[], element_s v[][1], element_s mu[][1],
                             uint32_t size) {

   for( int j = 0; j < size; j++ ) {
      element_init_Zr(mu[j], pairing);
      for( int i = 0; i < q; i++ ) {
         element_t temp;
         mpz_t m;
         element_init_Zr(temp, pairing);
         get_m(file, indices[ i ], j, m, size);
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

int CustodyUtils::generate_query_from_seed(mpz_t seed, unsigned int q, unsigned int n, uint64_t indices[],
                                            element_t *v[]) {
   element_t *ret = new element_t[q];
   for( int i = 0; i < q; i++ ) {
      mpz_t seedForIteration;
      mpz_init(seedForIteration);
      mpz_add_ui(seedForIteration, seed, i);

      unsigned char *digest = (unsigned char *) calloc(32, 1);
      char* seed_str = new char[mpz_sizeinbase(seedForIteration, 16) + 1];
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
      delete[](seed_str);
   }
   *v = ret;
   return 0;
}


int CustodyUtils::verify(element_t sigma, unsigned int q, uint64_t *indices, element_t *v, element_t *u, element_t *mu,
                          element_t pubk, uint32_t size) {
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
      sprintf(index, "%llu", indices[i]);
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

   for( int i = 0; i < size; i++ ) {
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


int CustodyUtils::clear_elements(element_t *array, int size) {
   for( int i = 0; i < size; i++ ) {
      element_clear(array[i]);
   }
   return 0;
}



int
CustodyUtils::compute_sigma(element_t sigmas[], unsigned int q, uint64_t indices[], element_t v[], element_t &sigma) {
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


int CustodyUtils::get_number_of_query(int blocks) {
   int questions = blocks;
   // TODO: needs research and optimization
   if( questions > 16 )
      questions = 16;
   return questions;
}


int CustodyUtils::verify_by_miner(const uint32_t &n, const char *u_seed, unsigned char *pubKey, unsigned char sigma[],
                                   std::vector<std::string> mus, mpz_t seed) {
   //prepate public_key and u
   element_t public_key;
   element_init_G1(public_key, pairing);
   element_from_bytes_compressed(public_key, pubKey);
   element_t * u = new element_t[mus.size()];

   mpz_t seedForU;

   char *buf_str = (char *) malloc(32 + 1);
   char *buf_ptr = buf_str;

   for( int i = 0; i < 16; i++ ) {
      buf_ptr += sprintf(buf_ptr, "%X", (unsigned char) u_seed[i]);
   }

   mpz_init_set_str(seedForU, buf_str, 16);
   free(buf_str);
   get_u_from_seed(seedForU, u, mus.size());


   //prepare sigma and mu
   element_t _sigma;
   element_t *mu = new element_t[mus.size()];

   element_init_G1(_sigma, pairing);
   element_from_bytes_compressed(_sigma, sigma);

   unsigned char buffer[DECENT_SIZE_OF_MU];

   for( int i = 0; i < mus.size(); i++ ) {
      element_init_Zr(mu[i], pairing);
      string_to_bytes(mus[i], buffer, DECENT_SIZE_OF_MU);
      element_from_bytes(mu[i], buffer);
   }

   unsigned int q = get_number_of_query(n);
   uint64_t* indices = new uint64_t[q];
   element_t *v;

   generate_query_from_seed(seed, q, n, indices, &v);

   int res = verify(_sigma, q, indices, v, u, mu, public_key, mus.size());

   clear_elements(u, mus.size());
   clear_elements(mu, mus.size());
   clear_elements(v, q);
   element_clear(public_key);
   mpz_clear(seedForU);
   element_clear(_sigma);
   delete[](v);
   delete[](indices);
   delete[](mu);
   delete[](u);
   return res;
}

int CustodyUtils::create_custody_data(path content, uint32_t &n, char u_seed[], unsigned char pubKey[], uint32_t sectors) {
   //prepare the files
   std::fstream infile(content.c_str(), std::fstream::binary | std::fstream::in);
   std::ofstream outfile((content.parent_path() / "content.cus").c_str(), std::fstream::binary | std::ios_base::trunc);

   infile.seekg(0, infile.beg);
   outfile.seekp(0);

   //prepare elements _u, m, seedForU and keys

   element_t* u = new element_t[sectors];
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

   get_u_from_seed(seedForU, u, sectors);


   element_random(private_key);
   element_pow_zn(public_key, generator, private_key);
#ifdef _CUSTODY_STATS
   pow++;
#endif

   //create the actual signatures in sigmas

   //split_file(infile, n, &m);
   n = get_n(infile, sectors);

   get_sigmas(infile, n, u, private_key, &sigmas, sectors);

   //save the values to u_seed and pubKey
   element_to_bytes_compressed(pubKey, public_key);

   //Save the signatures file
   char buffer[DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED];
   for( int i = 0; i < n; i++ ) {
      element_to_bytes_compressed((unsigned char *) buffer, sigmas[i]);
      outfile.write(buffer, DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED);
   }

   clear_elements(u, sectors);
   delete[](u);
   element_clear(private_key);
   element_clear(public_key);
   clear_elements(sigmas, n);
   delete[](sigmas);
   mpz_clear(seedForU);
   outfile.close();
   infile.close();
   return 0;
}

int CustodyUtils::create_proof_of_custody(path content, const uint32_t n, const char u_seed[], unsigned char pubKey[],
                                           unsigned char sigma[], std::vector<std::string> &mus, mpz_t seed) {
   //open files
   std::fstream infile(content.c_str(), std::fstream::binary | std::fstream::in);
   std::fstream cusfile((content.parent_path() / "content.cus").c_str(), std::fstream::binary | std::fstream::in);
   if( !infile.is_open() || !cusfile.is_open())
      return -10;
   cusfile.seekg(0);
   infile.seekg(0);

   //read the file and get sector size

   unsigned int nReal1, nReal2, size;
   nReal1 = get_n(infile, DECENT_SECTORS_BIG);
   nReal2 = get_n(infile, DECENT_SECTORS);
   //split_file(infile, nReal, &m);

   if( nReal1 == n )
      size = DECENT_SECTORS_BIG;
   else
   if ( nReal2 == n ) { //TODO_DECENT && HARDFORK_1
      size = DECENT_SECTORS;
      ilog("calculating custody proof with new sectors count");
   }
   else
      return -7;


   //prepate public_key and u
   element_t public_key;
   element_init_G1(public_key, pairing);
   element_from_bytes_compressed(public_key, pubKey);
   element_t * u = new element_t [size];

   char *buf_str = (char *) malloc(32 + 1);
   char *buf_ptr = buf_str;

   for( int i = 0; i < 16; i++ ) {
      buf_ptr += sprintf(buf_ptr, "%X", (unsigned char) u_seed[i]);
   }
   buf_str[32] = 0;

   mpz_t seedForU;

   mpz_init_set_str(seedForU, buf_str, 16);
   free(buf_str);
   get_u_from_seed(seedForU, u, size);


   //read sigmas
   element_t* sigmas = new element_t[n];
   char buffer[DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED];
   try {
   for( int i = 0; i < n; i++ ) {
      cusfile.read(&buffer[0], DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED);
//      if(cusfile.readsome(buffer, DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED) != DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED )
//         return -9;
      element_init_G1(sigmas[i], pairing);
      element_from_bytes_compressed(sigmas[i], (unsigned char *) buffer);
   } 
   } catch (std::exception e){
      wlog("Exception caught: ${s}", ("s", e.what() )); 
      return -8;
   }



   //generate query
   unsigned int q = get_number_of_query(n);
   uint64_t indices[16];
   element_t *v;
   generate_query_from_seed(seed, q, n, indices, &v);

   //calculate mu and sigma
   element_t * mu = new element_t[size];
   compute_mu(infile, q, indices, v, mu, size);

   element_t _sigma;
   compute_sigma(sigmas, q, indices, v, _sigma);

   //pack the proof
   element_to_bytes_compressed(sigma, _sigma);
   mus.clear();
   //TODO_DECENT change the following code to store in hexadecimal string
   unsigned char buffer_mu[DECENT_SIZE_OF_MU];

   for( int i = 0; i < size; i++ ) {
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
   clear_elements(u, size);
   clear_elements(mu, size);
   clear_elements(v, q);
   delete[] (sigmas);
   delete[](v);
   delete[](mu);
   delete[](u);
   return res;
}


}
}
