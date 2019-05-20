#pragma once

// #1 - UIA introduced; seeder regions; instant messaging.
#ifndef HARDFORK_1_TIME
#define HARDFORK_1_TIME (fc::time_point_sec( 1510228800 ))
#endif

// #2 - simple content; payment to content; option to fix asset max supply; new wallet file format.
#ifndef HARDFORK_2_TIME
#define HARDFORK_2_TIME (fc::time_point_sec( 1524470400 )) //2018-04-23 08:00:00
#endif

// #3 - UIA: the option to change the precision and to fix max supply;
#ifndef HARDFORK_3_TIME
#define HARDFORK_3_TIME (fc::time_point_sec( 1536825600 )) //2018-09-13 08:00:00 GMT
#endif

// #4 - NFT and submit content: removed 10 co-authors constraint, allowed CDN expiration date change
#ifndef HARDFORK_4_TIME
#define HARDFORK_4_TIME (fc::time_point_sec( 1559808000 )) //2019-06-06 08:00:00 GMT
#endif
