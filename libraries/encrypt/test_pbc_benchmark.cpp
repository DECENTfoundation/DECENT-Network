#include <iostream>

#include <gmp.h>
#include <pbc/pbc.h>
#include <pbc/pbc_test.h>

/*
#define _DECENT_PAIRING_PARAM_ "type a\n\
q 8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791\n\
h 12016012264891146079388821366740534204802954401251311822919615131047207289359704531102844802183906537786776\n\
r 730750818665451621361119245571504901405976559617\n\
exp2 159\n\
exp1 107\n\
sign1 1\n\
sign0 1"
 //*/

#define _DECENT_PAIRING_PARAM_ "type a\n\
q 107469721672869524998588652624299090254588259753590905940381448229303192908187\n\
h 83459336727718274173193788390204479901284\n\
r 1287689620916637251875563089646583807\n\
exp2 120\n\
exp1 115\n\
sign1 -1\n\
sign0 -1"

int main(int argc, char **argv) {
   pairing_t pairing;
   pairing_init_set_str(pairing, _DECENT_PAIRING_PARAM_);
   element_t g;
   element_t tmp;
   element_init_G1(g, pairing);
   element_init_G1(tmp, pairing);
   element_random(g);
   element_printf("system parameter g = %B\n", g);
   element_random(tmp);
   for(int i=0; i<0; i++){

      element_t tmp2;
      element_init_G1(tmp2, pairing);
      element_set(tmp2, tmp);
      element_mul(tmp, tmp2, g);
      element_clear(tmp2);
   }
   element_pp_t g_pp;
   element_pp_init(g_pp, g);
   mpz_t power;
   mpz_init(power);
   mpz_set_str(power, "26219755532374060961822481953204487875830586022463559049872633319816613072",10);
   for(int i=0; i<300000; i++){
      element_pp_pow(tmp, power, g_pp);
   }

   return 0;



}