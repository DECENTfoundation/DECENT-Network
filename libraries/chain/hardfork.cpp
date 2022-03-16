#include <graphene/chain/hardfork.hpp>

namespace graphene { namespace chain {

fork_times_t fork_times = {
    fc::time_point_sec::from_iso_string("2017-11-08T12:00:00"),
    fc::time_point_sec::from_iso_string("2018-04-23T08:00:00"),
    fc::time_point_sec::from_iso_string("2018-09-13T08:00:00"),
    fc::time_point_sec::from_iso_string("2019-07-11T08:00:00"),
    fc::time_point_sec::from_iso_string("2022-03-17T08:00:00")
};

} }
