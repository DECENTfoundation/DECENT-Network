//#define _CUSTODY_STATS
#include <decent/encrypt/encryptionutils.hpp>
#include <decent/encrypt/custodyutils.hpp>

#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>
//#include <graphene/chain/protocol/decent.hpp>
#include <fc/io/raw.hpp>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <fc/thread/thread.hpp>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <gmp.h>
#include <thread>


using namespace std;

decent::encrypt::CustodyUtils c;

namespace {

std::string bytes_to_string(unsigned char *data, int len) {
   std::stringstream ss;
   ss << std::hex << std::setfill('0');;
   for( int i=0; i < len; ++i )
      ss << std::setw(2) << (int) data[i];
   return ss.str();
}

void string_to_bytes(std::string& in, unsigned char *data, int len){
   std::istringstream hex_chars_stream(in);
   unsigned int c;
   int i = 0;
   while (hex_chars_stream >> std::hex >> c && i < len)
   {
      data[i] = c;
      ++i;
   }
}
}

using decent::encrypt::DInteger;
void test_aes(decent::encrypt::AesKey k)

{
   decent::encrypt::AES_encrypt_file("/tmp/test_file.txt","/tmp/test_file.out", k);
   decent::encrypt::AES_decrypt_file("/tmp/test_file.out","/tmp/test_file.orig",k);

}

void test_el_gamal(decent::encrypt::AesKey k)
{
   cout<<"Catchpoint 0 \n";
   DInteger pk1 = decent::encrypt::generate_private_el_gamal_key();
   DInteger pk2 = decent::encrypt::generate_private_el_gamal_key();
   DInteger pubk1 = decent::encrypt::get_public_el_gamal_key(pk1);
   DInteger pubk2 = decent::encrypt::get_public_el_gamal_key(pk2);


   cout <<"pk1 = " << pk1.to_string();
   cout <<"pk2 = " << pk2.to_string();
   cout<<"Catchpoint 1 \n";
   decent::encrypt::point secret;
   secret.first = DInteger(10000);
   secret.second = DInteger(1000000000);

   cout<<"Catchpoint 2 \n";
   decent::encrypt::Ciphertext ct1, ct2;
   decent::encrypt::el_gamal_encrypt(secret, pubk1, ct1);

   decent::encrypt::point received_secret;
   decent::encrypt::el_gamal_decrypt(ct1,pk1,received_secret);
   cout<<"Catchpoint 3 \n";

   cout <<"Secret is: "<<secret.first.to_string()<<" "<<secret.second.to_string();

   cout <<"\n";

   cout <<"recovered secret is "<<received_secret.first.to_string()<<" "<<received_secret.second.to_string() <<"\n";

   decent::encrypt::DeliveryProof proof(CryptoPP::Integer::One(),CryptoPP::Integer::One(),CryptoPP::Integer::One(),CryptoPP::Integer::One(),CryptoPP::Integer::One());

   cout<<"Catchpoint 4 \n";
   decent::encrypt::encrypt_with_proof(received_secret, pk1, pubk2, ct1, ct2, proof);
   cout<<"Catchpoint 5 \n";
   decent::encrypt::point received_secret2;
   decent::encrypt::el_gamal_decrypt(ct2,pk2,received_secret2);

   cout <<"recovered secret is "<<received_secret.first.to_string()<<" "<<received_secret.second.to_string() <<"\n";

   for (int i=0; i<1; i++)
      bool ret_val = decent::encrypt::verify_delivery_proof(proof, ct1,ct2,pubk1,pubk2);
   /*if(ret_val)
      cout<< "everything OK!\n";*/

}

void test_shamir(decent::encrypt::DInteger secret)
{
   decent::encrypt::ShamirSecret ss(5,9,secret);
   ss.calculate_split();
   decent::encrypt::point x0 = ss.split[0];
   decent::encrypt::point x1 = ss.split[1];
   decent::encrypt::point x2 = ss.split[2];
   decent::encrypt::point x3 = ss.split[3];
   decent::encrypt::point x4 = ss.split[6];

   decent::encrypt::ShamirSecret rs(5,9);

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
   std::cout << GMP_NUMB_BITS <<"\n";
   std::cout <<"number of cores "<< std::thread::hardware_concurrency()<<"\n";

   decent::encrypt::CustodyData cd;
   decent::encrypt::CustodyProof proof;

   proof.seed.data[0]=21; proof.seed.data[1] =155; proof.seed.data[2] = 231; proof.seed.data[3] = 98; proof.seed.data[4] = 1;

   c.create_custody_data(boost::filesystem::path("/tmp/content.zip"),cd );
   std::cout <<"done creating custody data, "<<cd.n<<" signatures generated\n";
   idump((cd));

   c.create_proof_of_custody(boost::filesystem::path("/tmp/content.zip"), cd,proof);
   idump((proof.mus));

   std::cout <<"done creating proof of custody \n";
   cout<<"\n\n";
  // fc::raw::pack(cout, mus);
   if(c.verify_by_miner(cd, proof))
      std::cout <<"Something wrong during verification...\n";
   else
      std::cout <<"Verify sucessful!\n";
}

void generate_params(){
   int rbits = 120;
   int qbits = 256;

   pbc_param_t par;
   pairing_t pairing;
   element_t generator;
   element_t zr;

   pbc_param_init_a_gen(par, rbits, qbits);
   pairing_init_pbc_param(pairing, par);
   element_init_G1(generator, pairing);
   element_random(generator);
   element_init_Zr(zr, pairing);

   pbc_param_out_str(stdout, par);
   element_printf("generator: %B\n",generator);
   element_printf("size of compressed: %i\n", element_length_in_bytes_compressed(generator));
   element_printf("size of element: %i\n", element_length_in_bytes(generator));
   element_printf("size of Zr element: %i\n", element_length_in_bytes(zr));
   pbc_param_clear(par);

}

void test_generator(){
   pairing_t pairing;
   pairing_init_set_str(pairing, _DECENT_PAIRING_PARAM_);

   mpz_t d1, d2, d3, d4, d5, d6, d7, d8;
   mpz_init(d1);
   mpz_init(d2);
   mpz_init(d3);
   mpz_init(d4);
   mpz_init(d5);
   mpz_init(d6);
   mpz_init(d7);
   mpz_init(d8);

   mpz_set_str(d1,"2", 10);
   mpz_set_str(d2,"521", 10);
   mpz_set_str(d3,"1831", 10);
   mpz_set_str(d4,"3067", 10);
   mpz_set_str(d5,"1294097889777887", 10);
   mpz_set_str(d6,"1838050274902939515372107", 10);
   mpz_set_str(d7,"1384673317831887198890420341", 10);
   mpz_set_str(d8,"1287689620916637251875563089646583808", 10);

   gmp_printf("d 1: %Zd\n",d1);
   gmp_printf("d 2: %Zd\n",d2);
   gmp_printf("d 3: %Zd\n",d3);
   gmp_printf("d 4: %Zd\n",d4);
   gmp_printf("d 5: %Zd\n",d5);
   gmp_printf("d 6: %Zd\n",d6);
   gmp_printf("d 7: %Zd\n",d7);
   gmp_printf("d 8: %Zd\n",d8);

   mpz_t div1, div2, div3, div4, div5, div6, div7;
   mpz_init(div1);
   mpz_init(div2);
   mpz_init(div3);
   mpz_init(div4);
   mpz_init(div5);
   mpz_init(div6);
   mpz_init(div7);


   mpz_t generator, q_1, q;
   mpz_init(generator);
   mpz_set_str(generator, "7977292573950573139348745395838273061335633755132672699089713070964550373066", 10);
   mpz_init(q_1);
   mpz_init(q);
   mpz_set_str(q_1, "107469721672869524998588652624299090254588259753590905940381448229303192908188", 10);
   mpz_set_str(q, "107469721672869524998588652624299090254588259753590905940381448229303192908187", 10);

   gmp_printf("generator: %Zd\n",generator);
   gmp_printf("q: %Zd\n",q);
   gmp_printf("q-1: %Zd\n",q_1);

   mpz_cdiv_q(div1, q_1, d1 );
   mpz_cdiv_q(div2, q_1, d2 );
   mpz_cdiv_q(div3, q_1, d3 );
   mpz_cdiv_q(div4, q_1, d4 );
   mpz_cdiv_q(div5, q_1, d5 );
   mpz_cdiv_q(div6, q_1, d6 );
   mpz_cdiv_q(div7, q_1, d7 );

   gmp_printf("exp 1: %Zd\n",div1);
   gmp_printf("exp 2: %Zd\n",div2);
   gmp_printf("exp 3: %Zd\n",div3);
   gmp_printf("exp 4: %Zd\n",div4);
   gmp_printf("exp 5: %Zd\n",div5);
   gmp_printf("exp 6: %Zd\n",div6);
   gmp_printf("exp 7: %Zd\n",div7);

   element_t gen, out;
   element_init_G1(gen,pairing);
   element_set_str(gen, _DECENT_GENERATOR_, 10);
   element_init_G1(out, pairing);
   mpz_t r_1, pow;
   mpz_init(r_1);
   mpz_init(pow);
   mpz_set_str(r_1, "1287689620916637251875563089646583806", 10);

   mpz_set_str(d1,"2", 10);
   mpz_set_str(d2,"3", 10);
   mpz_set_str(d3,"11", 10);
   mpz_set_str(d4,"1231", 10);
   mpz_set_str(d5,"71333", 10);
   mpz_set_str(d6,"55291858733", 10);
   mpz_set_str(d7,"4018440366064049", 10);

   mpz_div(pow,r_1,d1);
   element_pow_mpz(out, gen, pow);
   element_printf("final result 1: %B", out);
   mpz_div(pow,r_1,d2);
   element_pow_mpz(out, gen, pow);
   element_printf("final result 2: %B", out);
   mpz_div(pow,r_1,d3);
   element_pow_mpz(out, gen, pow);
   element_printf("final result 3: %B", out);
   mpz_div(pow,r_1,d4);
   element_pow_mpz(out, gen, pow);
   element_printf("final result 4: %B", out);
   mpz_div(pow,r_1,d5);
   element_pow_mpz(out, gen, pow);
   element_printf("final result 5: %B", out);
   mpz_div(pow,r_1,d6);
   element_pow_mpz(out, gen, pow);
   element_printf("final result 6: %B", out);
   mpz_div(pow,r_1,d7);
   element_pow_mpz(out, gen, pow);
   element_printf("final result 7: %B", out);
   element_pow_mpz(out, gen, d8);
   element_printf("final result 8: %B", out);

}

int main(int argc, char**argv)
{

//  decent::encrypt::AesKey k;

//   for (int i=0; i<CryptoPP::AES::MAX_KEYLENGTH; i++)
//      k.key_byte[i]=i;
 //  test_aes(k);
//   cout<<"AES finished \n";
//   test_key_manipulation();

 //  test_el_gamal(k);
 //  const CryptoPP::Integer secret("123433334548654864334348647431133987884123665444598978777891235468864444444445445556666666987455334678979464");
//   test_shamir(secret);
//   generate_params();
//   test_generator();
  test_custody();
}
