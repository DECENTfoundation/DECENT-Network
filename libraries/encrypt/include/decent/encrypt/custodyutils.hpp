//
// Created by Vazgen Manukyan on 12/5/16.
//

#pragma once
#define _PAIRING_PARAM_ "type a\n\
q 1729714013306464005330608405552380509755064820985046187891780198054445447554836677119858851399910513692414145560893075795183191121682380376026907533385851\n\
h 29876203542046996426175609850525021550250184148929105350006475655637240127612\n\
r 57896044618658097711785492504343953926634992332820282019728792006155588075521\n\
exp2 255\n\
exp1 41\n\
sign1 1\n\
sign0 1"

#define _GENERATOR_ "[65919675513796648672789818923465948251795268192765238019657366518918945557340570440796163625394782700898920959988147581140543372885475021419922767690877, \
725089450026118661311466522546391850117349026288377802150526825626904906986593597060680210192148148306223890888453862642254073877301185952403458909660563]"

#include <pbc.h>
#include <fstream>

#include <vector>
#include <decent/encrypt/crypto_types.hpp>

#define SIZE_OF_NUMBER_IN_THE_FIELD 32
#define SIZE_OF_POINT_ON_CURVE_COMPRESSED 65
#define SECTORS 8

namespace decent{
namespace crypto{


class CustodyUtils
{
public:
    CustodyUtils();

    int GetRandomU(element_t **out);

    int GetUFromSeed(const mpz_t &seedU, element_t **out);
    int SplitFile(std::fstream &file, unsigned int &n, element_t ***out);
    int GetSigmas(element_t **m, unsigned int n, element_t *u, element_t pk, element_t **sigmas);
    int GenerateQueryFromSeed(mpz_t seed, unsigned int &q, unsigned int n, int **indices, element_t **v);
    int ComputeMu(element_t **m, unsigned int q, int *indices, element_t *v, element_t **mu);
    int ComputeSigma(element_t *sigmas, unsigned int q, int *indices, element_t *v, element_t &sigma);
    int Verify(element_t sigma, unsigned int q, int *indices, element_t *v, element_t *u, element_t *mu, element_t pubk);
    int UnpackData(valtype data, element_t &pubk, unsigned int &n, element_t **u);
    int ClearElements(element_t *array, int size);
    int UnpackProof(valtype proof, element_t &sigma, element_t **mu);
    int CompressElements(element_t *array, unsigned int size, unsigned char **compressed);
    int RemoveSigmasFromFile(std::string pathIn, std::string pathOut);
    int GetNumberOfQuery(int blocks);
    int VerifyByMiner(valtype custodyData, valtype proof, mpz_t seed);
    int CreateCustodyData(std::string path, valtype &data, mpz_t &sizeOfFile);
    int CreateProofOfCustody(std::string path, valtype data, valtype &proof, mpz_t seed);
private:
    element_t generator;
    pairing_t pairing;
};


}}