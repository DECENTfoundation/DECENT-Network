#include <decent/encrypt/encryptionutils.hpp>
#include <decent/encrypt/custodyutils.hpp>

#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>
#include <graphene/chain/protocol/decent.hpp>
#include <fc/io/raw.hpp>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <fc/thread/thread.hpp>
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



void test_passed_op(graphene::chain::ready_to_publish_operation& op){
   std::vector<char> data = fc::raw::pack(op);
   idump((op));

   graphene::chain::ready_to_publish_operation tmp;
   fc::datastream<const char*> ds( data.data(), data.size() );
   fc::raw::unpack( ds, tmp );
   idump((tmp));
}

void test_passing_add_level_reference(graphene::chain::ready_to_publish_operation& op){
   std::shared_ptr<fc::thread> new_thread = std::make_shared<fc::thread>("p2p");
   new_thread->async([&](){ return test_passed_op(op);}).wait();

}


void test_move(){


   graphene::chain::ready_to_publish_operation op;
   op.space = 1000;
   decent::crypto::d_integer a = decent::crypto::d_integer::from_string("12132131.");
   op.pubKey = a;
   op.price_per_MByte = 1;
   idump((op));
   test_passing_add_level_reference(op);
}

void test_custody(){

   decent::crypto::custody_utils c;

   //pbc_param_t par;
   //pbc_param_init_a_gen( par, 320, 1024 );
   //pbc_param_out_str(stdout,par);

   decent::crypto::custody_data cd;
   decent::crypto::custody_proof proof;
   proof.seed.data[0]=21; proof.seed.data[1] =155; proof.seed.data[2] = 231; proof.seed.data[3] = 98; proof.seed.data[4] = 1;

   c.create_custody_data(boost::filesystem::path("/tmp/content.zip"),cd );
   std::cout <<"done creating custody data, "<<cd.n<<" signatures generated\n";

   c.create_proof_of_custody(boost::filesystem::path("/tmp/content.zip"), cd,proof);
 //  idump((proof.mus));

  // cout<<"\n\n";
  // fc::raw::pack(cout, mus);
   if(c.verify_by_miner(cd, proof))
      std::cout <<"Something wrong during verification...\n";
}

int main(int argc, char**argv)
{
//  decent::crypto::aes_key k;
//   for (int i=0; i<CryptoPP::AES::MAX_KEYLENGTH; i++)
//      k.key_byte[i]=i;
 //  test_aes(k);
   cout<<"AES finished \n";

//   test_el_gamal(k);
//   const CryptoPP::Integer secret("12354678979464");
 //  test_shamir(secret);
   test_move();

 //  test_el_gamal(k);
   const CryptoPP::Integer secret("12354678979464");
 //  test_shamir(secret);

   test_custody();

}
