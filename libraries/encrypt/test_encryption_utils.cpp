#include <decent/encrypt/encryptionutils.hpp>
#include <decent/encrypt/custodyutils.hpp>

#include <fc/exception/exception.hpp>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>


using namespace std;

using decent::crypto::d_integer;
void test_aes(decent::crypto::aes_key k)
{
   decent::crypto::AES_encrypt_file("/tmp/test_file.txt","/tmp/test_file.out", k);
   decent::crypto::AES_decrypt_file("/tmp/test_file.out","/tmp/test_file.orig",k);

}

void test_el_gamal(decent::crypto::aes_key k)
{
   cout<<"Catchpoint 0 \n";
   d_integer pk1 = decent::crypto::generate_private_el_gamal_key();
   d_integer pk2 = decent::crypto::generate_private_el_gamal_key();
   d_integer pubk1 = decent::crypto::get_public_el_gamal_key(pk1);
   d_integer pubk2 = decent::crypto::get_public_el_gamal_key(pk2);

   cout <<"pk1 = " << pk1.to_string();
   cout <<"pk2 = " << pk2.to_string();
   cout<<"Catchpoint 1 \n";
   decent::crypto::point secret;
   secret.first = d_integer(10000);
   secret.second = d_integer(1000000000);

   cout<<"Catchpoint 2 \n";
   decent::crypto::ciphertext ct1, ct2;
   decent::crypto::el_gamal_encrypt(secret, pubk1, ct1);

   decent::crypto::point received_secret;
   decent::crypto::el_gamal_decrypt(ct1,pk1,received_secret);
   cout<<"Catchpoint 3 \n";

   cout <<"Secret is: "<<secret.first.to_string()<<" "<<secret.second.to_string();

   cout <<"\n";

   cout <<"recovered secret is "<<received_secret.first.to_string()<<" "<<received_secret.second.to_string() <<"\n";

   decent::crypto::delivery_proof proof(CryptoPP::Integer::One(),CryptoPP::Integer::One(),CryptoPP::Integer::One(),CryptoPP::Integer::One(),CryptoPP::Integer::One());
   cout<<"Catchpoint 4 \n";
   decent::crypto::encrypt_with_proof(received_secret, pk1, pubk2, ct1, ct2, proof);
   cout<<"Catchpoint 5 \n";
   decent::crypto::point received_secret2;
   decent::crypto::el_gamal_decrypt(ct2,pk2,received_secret2);

   cout <<"recovered secret is "<<received_secret.first.to_string()<<" "<<received_secret.second.to_string() <<"\n";

   for (int i=0; i<1; i++)
      bool ret_val = decent::crypto::verify_delivery_proof(proof, ct1,ct2,pubk1,pubk2);
   /*if(ret_val)
      cout<< "everything OK!\n";*/

}

void test_shamir(decent::crypto::d_integer secret)
{
   decent::crypto::shamir_secret ss(5,9,secret);
   decent::crypto::point x0 = ss.split[0];
   decent::crypto::point x1 = ss.split[1];
   decent::crypto::point x2 = ss.split[2];
   decent::crypto::point x3 = ss.split[3];
   decent::crypto::point x4 = ss.split[6];

   decent::crypto::shamir_secret rs(5,9);
   rs.add_point(x0);
   rs.add_point(x1);
   rs.add_point(x2);
   rs.add_point(x3);
   rs.add_point(x4);
   if(rs.resolvable())
      rs.calculate_secret();
   cout << "Original secret: "<< secret.to_string() <<"\nReconstructed_secret: "<<rs.secret.to_string() <<"\n";
}

void test_custody(){
   char u_seed[16];
   uint32_t n;
   decent::crypto::custody_utils c;
   cout<< "zr lenght: "<<pairing_length_in_bytes_Zr(c.pairing)<<"\n";
   element_t test, test2, test3;

   element_init_G1(test, c.pairing);
   element_init_G1(test2, c.pairing);
   element_init_G1(test3, c.pairing);
   element_set_si(test2, 398421377431);
   element_set_si(test3, 398421377431);

   element_mul(test, test2, test3);

   //pbc_param_t par;
   //pbc_param_init_a_gen( par, 320, 1024 );
   //pbc_param_out_str(stdout,par);
   unsigned char pubKey[DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED];
   unsigned char sigma[DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED];
   std::vector<std::vector<unsigned char>> mus;
   mpz_t seed;
   mpz_init_set_ui(seed, 12458342);
   c.create_custody_data(boost::filesystem::path("/tmp/content.zip"),n, u_seed, pubKey );
   std::cout <<"done creating custody data, "<<n<<" signatures generated\n";

   c.create_proof_of_custody(boost::filesystem::path("/tmp/content.zip"),n, pubKey, u_seed, sigma, mus, seed);
   if(c.verify_by_miner(n, u_seed, pubKey, sigma, mus, seed))
      std::cout <<"Something wrong during verification...\n";
   mpz_clear (seed);
}

int main(int argc, char**argv)
{
   decent::crypto::aes_key k;
   for (int i=0; i<CryptoPP::AES::MAX_KEYLENGTH; i++)
      k.key_byte[i]=i;
 //  test_aes(k);
   cout<<"AES finished \n";
 //  test_el_gamal(k);
   const CryptoPP::Integer secret("12354678979464");
 //  test_shamir(secret);

   test_custody();
}
