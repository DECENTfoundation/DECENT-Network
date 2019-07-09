#pragma once

#include <fc/time.hpp>
#include <array>

namespace graphene { namespace chain {

using fork_times_t = std::array<fc::time_point_sec, 4>;
extern fork_times_t fork_times;

} }

// #1 - UIA introduced, seeder regions, instant messaging
#ifndef HARDFORK_1_TIME
#define HARDFORK_1_TIME graphene::chain::fork_times[0]
#endif

// #2 - simple content, payment to content, option to fix asset max supply, new wallet file format
#ifndef HARDFORK_2_TIME
#define HARDFORK_2_TIME graphene::chain::fork_times[1]
#endif

// #3 - UIA: the option to change the precision and to fix max supply
#ifndef HARDFORK_3_TIME
#define HARDFORK_3_TIME graphene::chain::fork_times[2]
#endif

// #4 - NFT and submit content: removed 10 co-authors constraint, allowed CDN expiration date change
#ifndef HARDFORK_4_TIME
#define HARDFORK_4_TIME graphene::chain::fork_times[3]
#endif
