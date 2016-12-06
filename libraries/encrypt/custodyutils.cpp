// Copyright (c) 2015-2015 Decent developers

#include <iostream>
#include "decent/encrypt/custodyutils.hpp"
#include "openssl/sha.h"
#include <cmath>
#include <openssl/rand.h>

namespace decent{
namespace crypto{


void GetRandBytes(unsigned char* buf, int num)
{
    if (RAND_bytes(buf, num) != 1) {
        abort();
    }
}

void GetRandHash(mpz_t &hash)
{
    char unsigned *tmp;
    GetRandBytes((unsigned char*)&tmp, sizeof(hash));

    mpz_set_str(hash, (char *)tmp, 16);
}

CustodyUtils::CustodyUtils()
{
    pairing_init_set_str(pairing, _PAIRING_PARAM_);
    element_t private_key, public_key;

    element_init_G1(generator, pairing);
    element_init_Zr(private_key, pairing);
    element_init_G1(public_key, pairing);

    element_set_str(generator, _GENERATOR_, 10);
    element_random(private_key);
    element_pow_zn(public_key, generator, private_key);
}

int CustodyUtils::GetRandomU(element_t **out)
{
    element_t *u = new element_t[SECTORS];
    for (int i = 0; i < SECTORS; i++)
    {
        element_init_G1(u[i], pairing);
        element_random(u[i]);
    }
    *out = u;
    return 0;
}

int CustodyUtils::GetUFromSeed(const mpz_t &seedU, element_t **out)
{
    mpz_t seed;
    mpz_init_set(seed, seedU);

    element_t *u = new element_t[SECTORS];
    unsigned char digest[32];

    for (int i = 0; i < SECTORS; i++)
    {
        mpz_add_ui(seed, seed, i);

        element_init_G1(u[i], pairing);

//        SHA256((unsigned char *)seed.ToString().c_str(), 64, digest);
        char *seed_str;
        mpz_get_str(seed_str, 16, seed);
        SHA256((unsigned char *)seed_str, 64, digest);

        element_from_hash(u[i], digest, 32);
    }
    *out = u;
    return 0;
}

int CustodyUtils::SplitFile(std::fstream &file, unsigned int &n, element_t ***out)
{
    if(!file.is_open())
        return -1;
    long long position = file.tellg();
    file.seekg (0, file.end);
    long long length = (long long)file.tellg() - position;
    long long realLen = file.tellg();
    file.seekg (position, file.beg);

    n = length / (SIZE_OF_NUMBER_IN_THE_FIELD * SECTORS);
    if (length % (SIZE_OF_NUMBER_IN_THE_FIELD * SECTORS))
        n += 1;

    element_t **m = new element_t*[n];
    for(int i = 0; i < n; i++)
        m[i] = new element_t[SECTORS];

    char buffer[SIZE_OF_NUMBER_IN_THE_FIELD];

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < SECTORS; j++)
        {
            element_init_Zr(m[i][j], pairing);
            if (realLen > ((long long)(file.tellg()) + SIZE_OF_NUMBER_IN_THE_FIELD))
            {
                file.read(buffer, SIZE_OF_NUMBER_IN_THE_FIELD);
            } else if (file.eof()) {
                for (int k = 0; k < SIZE_OF_NUMBER_IN_THE_FIELD; k++)
                {
                    buffer[k] = 0;
                }
            } else
            {
                int left = realLen - (long long)(file.tellg());
                file.read(buffer, left);
                for (int k = left; k < SIZE_OF_NUMBER_IN_THE_FIELD; k++)
                {
                    buffer[k] = 0;
                }
            }
            element_from_bytes(m[i][j], (unsigned char *)buffer);
        }
    }
    *out = m;

    return 0;
}


int CustodyUtils::GetSigmas(element_t **m, unsigned int n, element_t *u, element_t pk, element_t **sigmas)
{
    element_t *ret = new element_t[n];

    for (int i = 0; i < n; i++)
    {
        element_init_G1(ret[i], pairing);
        for(int j = 0; j < SECTORS; j++)
        {
            element_t temp;
            element_init_G1(temp, pairing);
            element_pow_zn(temp, u[j], m[i][j]);
            if (j)
                element_mul(ret[i], ret[i], temp);
            else
                element_set(ret[i], temp);
        }

        element_t hash;
        element_init_G1(hash, pairing);
        unsigned char buf[256] = "";
        char index[4] = "";
        sprintf(index, "%d", i);
        SHA256((unsigned char *)index, 4, buf);
        element_from_hash(hash, buf, 256);
        element_mul(ret[i], ret[i], hash);
        element_pow_zn(ret[i], ret[i], pk);
    }
    *sigmas = ret;
    return 0;
}

int CustodyUtils::GenerateQueryFromSeed(mpz_t seed, unsigned int &q, unsigned int n, int **indices, element_t **v)
{
    q = GetNumberOfQuery(n);

    element_t *retv = new element_t[q];
    int *reti = new int[q];

    for (int i = 0; i < q; i++)
    {
//        uint256 seedForIteration;
//        seedForIteration = seed + i;

        mpz_t seedForIteration;
        mpz_add_ui(seedForIteration, seed, i);

        unsigned char digest[32];

        char *seed_str;
        mpz_get_str(seed_str, 16, seedForIteration);
        SHA256((unsigned char *)seed_str, 64, digest);
        if (q < 16)
        {
            reti[i] = i;
        }
        else
        {
            memcpy(&reti[i], digest, 8);
            reti[i] = reti[i] % n;
        }
        element_init_Zr(retv[i], pairing);
        element_from_hash(retv[i], digest, 32);
    }
    *v = retv;
    *indices = reti;
    return 0;
}


int CustodyUtils::ComputeMu(element_t **m, unsigned int q, int *indices, element_t *v, element_t **mu)
{
    element_t *ret = new element_t[SECTORS];

    for (int j = 0; j < SECTORS; j++)
    {
        element_init_Zr(ret[j], pairing);
        for (int i = 0; i < q; i++)
        {
            element_t temp;
            element_init_Zr(temp, pairing);
            element_mul(temp, v[i], m[indices[i]][j]);
            if (i)
                element_add(ret[j], ret[j], temp);
            else
                element_set(ret[j], temp);
        }
    }
    *mu = ret;
    return 0;
}


int CustodyUtils::Verify(element_t sigma, unsigned int q, int *indices, element_t *v, element_t *u, element_t *mu, element_t pubk)
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
        char index[4] = "";
        sprintf(index, "%d", indices[i]);
        SHA256((unsigned char *)index, 4, buf);
        element_from_hash(hash, buf, 256);
        element_pow_zn(temp, hash, v[i]);
        if (i)
            element_mul(multi1, multi1, temp);
        else
            element_set(multi1, temp);
    }

    element_clear(hash);
    element_t multi2;
    element_init_G1(multi2, pairing);

    for (int i = 0; i < SECTORS; i++)
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
    element_clear(res1);
    element_clear(res2);
    element_clear(multi1);
    element_clear(multi2);
    return res;
}


int CustodyUtils::UnpackData(valtype data, element_t &pubk, unsigned int &n, element_t **u)
{
    int pointer = 0;
    n = (unsigned int)(data[0]);
    n += ((unsigned int)(data[1])<<8);
    n += ((unsigned int)(data[2])<<16);
    n += ((unsigned int)(data[3])<<24);

    pointer += 4;
    element_init_G1(pubk, pairing);
    element_from_bytes_compressed(pubk, data.data() + pointer);
    pointer += SIZE_OF_POINT_ON_CURVE_COMPRESSED;

    mpz_t seed;
    std::string seedStr(data.begin() + pointer, data.end());
//    seed.SetHex(seedStr);
    mpz_set_str(seed, seedStr.c_str(), 16);

    element_t *ret;
    GetUFromSeed(seed, &ret);
    *u = ret;
    return 0;
}

int CustodyUtils::ClearElements(element_t *array, int size)
{
    for (int i = 0; i < size; i++)
    {
        element_clear(array[i]);
    }
    delete[] array;
    return 0;
}

int CustodyUtils::UnpackProof(valtype proof, element_t &sigma, element_t **mu)
{
    int pointer = 0;
    element_init_G1(sigma, pairing);
    element_from_bytes_compressed(sigma, proof.data() + pointer);
    pointer += SIZE_OF_POINT_ON_CURVE_COMPRESSED;

    element_t *ret = new element_t[SECTORS];
    for (int i = 0; i < SECTORS; i++)
    {
        element_init_Zr(ret[i], pairing);
        element_from_bytes(ret[i], proof.data() + pointer);
        pointer += SIZE_OF_NUMBER_IN_THE_FIELD;
    }
    *mu = ret;
    return 0;
}

int CustodyUtils::CompressElements(element_t *array, unsigned int size, unsigned char **compressed)
{
    int sizeChar = SIZE_OF_POINT_ON_CURVE_COMPRESSED * size;
    unsigned char *ret = new unsigned char[sizeChar];
    for (int i = 0; i < size; i++)
    {
        element_to_bytes_compressed(ret + i*SIZE_OF_POINT_ON_CURVE_COMPRESSED, array[i]);
    }
    *compressed = ret;
    return 0;
}



int CustodyUtils::RemoveSigmasFromFile(std::string pathIn, std::string pathOut)
{
    std::ifstream in(pathIn.c_str(), std::fstream::binary);
    std::ofstream out(pathOut.c_str(), std::fstream::binary);

    in.seekg(0, in.end);
    int length = in.tellg();
    unsigned int n = ceil((((double)length / (double)(SIZE_OF_NUMBER_IN_THE_FIELD*SECTORS + SIZE_OF_POINT_ON_CURVE_COMPRESSED))));

    in.seekg(SIZE_OF_POINT_ON_CURVE_COMPRESSED*n, in.beg);
    char* temp = new char[length - SIZE_OF_POINT_ON_CURVE_COMPRESSED*n];
    in.read(temp, length - SIZE_OF_POINT_ON_CURVE_COMPRESSED*n);
    out.write(temp, length - SIZE_OF_POINT_ON_CURVE_COMPRESSED*n);
    in.close();
    out.close();
    return 0;
}


int CustodyUtils::ComputeSigma(element_t *sigmas, unsigned int q, int *indices, element_t *v, element_t &sigma)
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
    }
    return 0;
}


int CustodyUtils::GetNumberOfQuery(int blocks)
{
    int questions = blocks;
    // TODO: needs research and optimization
    if (questions > 16)
        questions = 16;
    return questions;
}


int CustodyUtils::VerifyByMiner(valtype custodyData, valtype proof, mpz_t seed)
{
    element_t *u;
    unsigned int n;
    element_t public_key;
    UnpackData(custodyData, public_key, n, &u);

    element_t sigma;
    element_t *mu;
    UnpackProof(proof, sigma, &mu);

    int *indices;
    element_t *v;
    unsigned int q;

    GenerateQueryFromSeed(seed, q, n, &indices, &v);
    int res = Verify(sigma, q, indices, v, u, mu, public_key);

    ClearElements(u, SECTORS);
    ClearElements(mu, SECTORS);
    ClearElements(v, q);
    delete[] indices;
    return res;
}

int CustodyUtils::CreateCustodyData(std::string path, valtype &data, mpz_t &sizeOfFile)
{
    // File out, (pub | u)
    unsigned int n;
    data.clear();
    std::fstream file(path.c_str(), std::fstream::binary | std::fstream::in | std::fstream::out);
    std::ofstream outfile ("tmpfileDecent", std::fstream::binary);
    outfile << file.rdbuf();
    outfile.close();
    std::ifstream temp("tmpfileDecent", std::fstream::binary);

    if (!file.is_open())
        return -1;
    if (!temp.is_open())
        return -1;
    element_t **m;
    file.seekg(0, file.beg);
    SplitFile(file, n, &m);
    data.push_back((unsigned char)n);
    data.push_back((unsigned char)(n>>8));
    data.push_back((unsigned char)(n>>16));
    data.push_back((unsigned char)(n>>24));

    element_t private_key, public_key;
    element_init_Zr(private_key, pairing);
    element_init_G1(public_key, pairing);

    element_random(private_key);
    element_pow_zn(public_key, generator, private_key);

    /**
     * @todo GetRandHash is unknown
     */
    mpz_t seedForU;
    GetRandHash(seedForU);
//    mpz_t seedForU;

    element_t *u;
    GetUFromSeed(seedForU, &u);

    unsigned char* compressedPublicKey = new unsigned char[SIZE_OF_POINT_ON_CURVE_COMPRESSED];
    element_to_bytes_compressed(compressedPublicKey, public_key);

    // std::string uStr(seedForU.ToString());
    char *seed_c;
    mpz_get_str(seed_c, 16, seedForU);
    std::string uStr(seed_c);

    data.insert(data.end(), compressedPublicKey, compressedPublicKey + SIZE_OF_POINT_ON_CURVE_COMPRESSED);
    data.insert(data.end(), uStr.begin(), uStr.end());

    delete[] compressedPublicKey;

    element_t *sigmas;
    GetSigmas(m, n, u, private_key, &sigmas);

    for (int i = 0; i < n; i++)
    {
        ClearElements(m[i], SECTORS);
    }
    delete[] m;
    ClearElements(u, SECTORS);
    element_clear(private_key);
    element_clear(public_key);

//    std::ofstream out(pathOut.c_str(), std::ofstream::binary);
//    if (!out.is_open())
//        return -1;
    file.seekp(0, file.beg);
    char *buffer = new char[SIZE_OF_POINT_ON_CURVE_COMPRESSED];
    for (int i = 0; i < n; i++)
    {
        element_to_bytes_compressed((unsigned char *)buffer, sigmas[i]);
        for (int j = 0; j < SIZE_OF_POINT_ON_CURVE_COMPRESSED; j++)
        {
            file.write(&buffer[j], 1);
        }
    }
    delete[] buffer;
    ClearElements(sigmas, n);
    file << temp.rdbuf();
    temp.close();
    remove("tmpfileDecent");
//    in.seekg(0, in.beg);
//    out << in.rdbuf();
//    in.close();
    file.seekg(0, file.end);
    mpz_init_set_ui(sizeOfFile, file.tellg())
    ;
    file.close();
    return 0;
}

int CustodyUtils::CreateProofOfCustody(std::string path, valtype data, valtype &proof, mpz_t seed)
{
    unsigned int n;
    element_t public_key;
    element_t *u;
    UnpackData(data, public_key, n, &u);

    int *indices;
    element_t *v;

    std::fstream in(path.c_str(), std::fstream::binary | std::fstream::in);
    if (!in.is_open())
        return -1;
    element_t *sigmas = new element_t[n];

    char *buffer = new char[SIZE_OF_POINT_ON_CURVE_COMPRESSED];
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < SIZE_OF_POINT_ON_CURVE_COMPRESSED; j++)
        {
            in.read(&buffer[j], 1);
        }
        element_init_G1(sigmas[i], pairing);
        element_from_bytes_compressed(sigmas[i], (unsigned char *)buffer);
    }
    delete[] buffer;

    element_t **m;
    unsigned int nReal;
    SplitFile(in, nReal, &m);

    unsigned int q;
    GenerateQueryFromSeed(seed, q, n, &indices, &v);

    element_t *mu;
    ComputeMu(m, q, indices, v, &mu);
    for (int i = 0; i < n; i++)
    {
        ClearElements(m[i], SECTORS);
    }
    delete[] m;

    element_t sigma;
    ComputeSigma(sigmas, q, indices, v, sigma);
    ClearElements(sigmas, n);

    proof.clear();
    unsigned char* compressedSigma = new unsigned char[SIZE_OF_POINT_ON_CURVE_COMPRESSED];
    element_to_bytes_compressed(compressedSigma, sigma);
    unsigned char* charMu = new unsigned char[SIZE_OF_NUMBER_IN_THE_FIELD*SECTORS];
    for (int i = 0; i < SECTORS; i++)
    {
        element_to_bytes(&charMu[SIZE_OF_NUMBER_IN_THE_FIELD*i], mu[i]);
    }

    proof.insert(proof.end(), compressedSigma, compressedSigma + SIZE_OF_POINT_ON_CURVE_COMPRESSED);
    proof.insert(proof.end(), charMu, charMu + SIZE_OF_NUMBER_IN_THE_FIELD*SECTORS);
    delete[] compressedSigma;
    delete[] charMu;
    int res = VerifyByMiner(data, proof, seed);

    element_clear(public_key);
    ClearElements(u, SECTORS);
    ClearElements(mu, SECTORS);
    ClearElements(v, q);
    delete[] indices;
    return res;
}


}}