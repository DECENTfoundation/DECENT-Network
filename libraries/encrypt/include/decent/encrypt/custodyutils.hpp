//
// Created by Vazgen Manukyan on 12/5/16.
//

#pragma once

#include <pbc/pbc.h>
#include <fstream>

#include <vector>
#include <decent/encrypt/crypto_types.hpp>
#include <boost/filesystem.hpp>


#ifndef SHORT_CURVE
#define _DECENT_PAIRING_PARAM_ "type a\n\
q 1729714013306464005330608405552380509755064820985046187891780198054445447554836677119858851399910513692414145560893075795183191121682380376026907533385851\n\
h 29876203542046996426175609850525021550250184148929105350006475655637240127612\n\
r 57896044618658097711785492504343953926634992332820282019728792006155588075521\n\
exp2 255\n\
exp1 41\n\
sign1 1\n\
sign0 1"

#define _DECENT_GENERATOR_ "[65919675513796648672789818923465948251795268192765238019657366518918945557340570440796163625394782700898920959988147581140543372885475021419922767690877, \
725089450026118661311466522546391850117349026288377802150526825626904906986593597060680210192148148306223890888453862642254073877301185952403458909660563]"
#define DECENT_SIZE_OF_NUMBER_IN_THE_FIELD 32
#else
#define _DECENT_PAIRING_PARAM_ "type a\n\
q 107469721672869524998588652624299090254588259753590905940381448229303192908187\n\
h 83459336727718274173193788390204479901284\n\
r 1287689620916637251875563089646583807\n\
exp2 120\n\
exp1 115\n\
sign1 -1\n\
sign0 -1"

#define _DECENT_GENERATOR_ "[86840749517316137157891261963398293121077652946631672762188488864778675462754, \
46298439815083954540948251518245740272972067256063234341481114422918560267725]"
//Make it 16 to better fit into mpz
#define DECENT_SIZE_OF_NUMBER_IN_THE_FIELD 32
#define DECENT_MP_SIZE_OF_NUMBER_IN_THE_FIELD 4
#endif



namespace decent{
namespace crypto{

using namespace boost::filesystem;




class custody_utils
{
public:
   custody_utils();
   ~custody_utils();

   int verify_by_miner(custody_data cd, custody_proof proof){
      mpz_t s;
      mpz_init(s);
      mpz_import(s, 5, 1, sizeof(uint32_t), 0, 0, proof.seed.data);
      int ret=verify_by_miner(cd.n, (char*)cd.u_seed.data, cd.pubKey.data, proof.sigma.data, proof.mus, s);
      mpz_clear(s);
      return ret;
   }

   int create_custody_data(boost::filesystem::path content, custody_data & cd){
      return create_custody_data(content, cd.n, (char*)cd.u_seed.data, cd.pubKey.data);
   }

   int create_proof_of_custody(boost::filesystem::path content, custody_data cd, custody_proof& proof){
      mpz_t s;
      mpz_init(s);
      mpz_import(s, 5, 1, sizeof(uint32_t), 0, 0, proof.seed.data);
      int ret = create_proof_of_custody(content, cd.n, (char*)cd.u_seed.data, cd.pubKey.data, proof.sigma.data, proof.mus, s);
      mpz_clear(s);
      return ret;
   }
   /**
    * Verifies proof of delivery
    * @param n number of signatures
    * @param u_seed seed used to generate U's
    * @param pubKey uploader's public key
    * @param sigma calcuated sigma
    * @param mus calculated mu's
    * @param seed seed used for proof calculation - taken from latest block hash
    * @return 0 if success
    */
   int verify_by_miner(const uint32_t &n, const char *u_seed, unsigned char *pubKey, unsigned char sigma[],
                       std::vector<std::string> mus, mpz_t seed);
   /**
    * Creates custody signatures in file content.cus;
    * @param content
    * @param n the number of signatures
    * @param u_seed is the generator for u. There must be at least 16 bytes allocated in the u array
    * @param pubKey is generated public key. There must be at least DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED bytes allocated in the pubKey
    * @return 0 if success
    */
   int create_custody_data(boost::filesystem::path content, uint32_t& n, char u_seed[], unsigned char pubKey[]);
   /**
    * Create proof of custody out of content.zip stored in path. content.cus must exist in the same directory
    * @param content path to content.zip
    * @param n number of signatures
    * @param u_seed seed used to generate U's
    * @param pubKey uploader's public key
    * @param sigma calcuated sigma
    * @param mus output - calculated mu's
    * @param seed seed used for proof calculation - taken from latest block hash
    * @return 0 if success
    */
   int create_proof_of_custody(boost::filesystem::path content, const uint32_t n, const char u_seed[], unsigned char pubKey[],
                               unsigned char sigma[], std::vector<std::string> &mus, mpz_t seed);

//private:
   element_t generator;
   pairing_t pairing;
   /*
    * Split the files to array of m_ij elements
    * TODO_DECENT rework to stram version
    */
   int split_file(std::fstream &file, unsigned int &n, element_t ***out);
   /*
    * Calculate sigmas based on formula
    * TODO_DECENT rework to stram version
    */
   int get_sigmas(std::fstream &file, const unsigned int n, element_t *u, element_t pk, element_t **sigmas);
   /*
    * Generates u from seed seedU. The array must be initalized to at least DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED elements
    */
   int get_u_from_seed(const mpz_t &seedU, element_t out[]);
   int generate_query_from_seed(mpz_t seed, unsigned int q, unsigned int n, uint64_t indices[], element_t* v[]);
   int compute_mu(std::fstream& file, unsigned int q, uint64_t indices[], element_t v[], element_t mu[]);
   int compute_sigma(element_t *sigmas, unsigned int q, uint64_t *indices, element_t *v, element_t &sigma);
   int get_sigma( uint64_t pidx, mpz_t mi[], element_pp_t u_pp[], element_t pk, element_t out[]);
   int verify(element_t sigma, unsigned int q, uint64_t *indices, element_t *v, element_t *u, element_t *mu, element_t pubk);
   int clear_elements(element_t *array, int size);
   int unpack_proof(valtype proof, element_t &sigma, element_t **mu);
   int get_number_of_query(int blocks);
   int get_n(std::fstream &file);
   inline int get_m(std::fstream &file, uint32_t i, uint32_t j, mpz_t& out);
   inline int get_data(std::fstream &file, uint32_t i, char buffer[]);
};


}}
