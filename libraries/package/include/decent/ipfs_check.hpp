/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#pragma once

namespace decent {

/**
 * @brief checks IPFS version and compares with minimal supported
 * @param host [in] Hostname or IP address of the server to connect to.
 * @param port [in] Port to connect to.
 * @return returns true when check is OK, otherwise false
 */
void check_ipfs_minimal_version(const std::string& host, int port);

} //namespace decent
