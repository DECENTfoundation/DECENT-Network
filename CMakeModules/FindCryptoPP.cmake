# try to include Crypto++ library

MESSAGE (STATUS "Checking CryptoPP")

FIND_LIBRARY(CRYPTOPP_LIBRARIES NAMES cryptopp )

if(NOT CRYPTOPP_LIBRARIES)
    message(FATAL_ERROR "crypto++ library not found")
endif()