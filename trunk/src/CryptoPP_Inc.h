#ifdef EMBEDDED_CRYPTO
	#include "CryptoPP.h"
#else
	#ifdef __CRYPTO_DEBIAN_GENTOO__
		#include <crypto++/config.h>
		#include <crypto++/md4.h>
		#include <crypto++/rsa.h>
		#include <crypto++/base64.h>
		#include <crypto++/osrng.h>
		#include <crypto++/files.h>
		#include <crypto++/sha.h>
	#elif __CRYPTO_SOURCE__
		#include <crypto-5.1/config.h>
		#include <crypto-5.1/md4.h>
		#include <crypto-5.1/rsa.h>
		#include <crypto-5.1/base64.h>
		#include <crypto-5.1/osrng.h>
		#include <crypto-5.1/files.h>
		#include <crypto-5.1/sha.h>
	#else 
		#include <cryptopp/config.h>
		#include <cryptopp/md4.h>
		#include <cryptopp/rsa.h>
		#include <cryptopp/base64.h>
		#include <cryptopp/osrng.h>
		#include <cryptopp/files.h>
		#include <cryptopp/sha.h>
	#endif
#endif

