//
// Created by Vazgen Manukyan on 12/5/16.
//

#pragma once
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

#include <pbc.h>
#include <fstream>

#include <vector>
#include <decent/encrypt/crypto_types.hpp>

#define DECENT_SIZE_OF_NUMBER_IN_THE_FIELD 32
#define DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED 65
#define DECENT_SECTORS 8

namespace decent{
namespace crypto{


class custody_utils
{
public:
    custody_utils();

    int get_random_u(element_t **out);
    int get_u_from_seed(const mpz_t &seedU, element_t **out);
    int split_file(std::fstream &file, unsigned int &n, element_t ***out);
    int get_sigmas(element_t **m, unsigned int n, element_t *u, element_t pk, element_t **sigmas);
    int generate_query_from_seed(mpz_t seed, unsigned int &q, unsigned int n, int **indices, element_t **v);
    int compute_mu(element_t **m, unsigned int q, int *indices, element_t *v, element_t **mu);
    int compute_sigma(element_t *sigmas, unsigned int q, int *indices, element_t *v, element_t &sigma);
    int verify(element_t sigma, unsigned int q, int *indices, element_t *v, element_t *u, element_t *mu, element_t pubk);
    int unpack_data(valtype data, element_t &pubk, unsigned int &n, element_t **u);
    int clear_elements(element_t *array, int size);
    int unpack_proof(valtype proof, element_t &sigma, element_t **mu);
    int compress_elements(element_t *array, unsigned int size, unsigned char **compressed);
    int remove_sigmas_from_file(std::string pathIn, std::string pathOut);
    int get_number_of_query(int blocks);
    int verify_by_miner(valtype custodyData, valtype proof, mpz_t seed);
    int create_custody_data(std::string path, valtype &data, mpz_t &sizeOfFile);
    int create_proof_of_custody(std::string path, valtype data, valtype &proof, mpz_t seed);
private:
    element_t generator;
    pairing_t pairing;
};


}}