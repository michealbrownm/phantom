/*
	phantom is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	phantom is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with phantom.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ECC_SM2_H_
#define ECC_SM2_H_

#include <memory>
#include <limits.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/ecdsa.h>
#include <openssl/ecdh.h>
#include <string>
#ifdef WIN32
#include <winbase.h>
#endif

namespace utils{

    class EccSm2 {
        BIGNUM *dA_;//Private key
        EC_POINT* pkey_;//Public key
        std::string skey_bin_;
        bool valid_;
		std::string error_;
    public:
		enum GROUP_TYPE {
			GFP = 0,
			F2M = 1
		};
		//Maximum support for 1024bit operations
		const static int MAX_BITS = 128;
        EccSm2(EC_GROUP* group);
        ~EccSm2();

        bool From(std::string skey_bin);
        bool NewRandom();

        //id_bin  User ID
        //msg_bin Message (byte stream)
        //sigr  Signature for part r (byte stream)
        //sigs  Signature for part s (byte stream)
		std::string Sign(const std::string& id_bin, const std::string& msg_bin);
        
		//Return an uncompressed public key
        std::string GetPublicKey();

        //px: x coordinate of public key 
        //py: y coordinate of public key 
        //msg: Validated message
        //id: User identity
        //r: Signature r
        //s: s value of signature
		/*static int verify(EC_GROUP* group, const std::string& px, const std::string& py,
			const std::string& id, const std::string& msg, const std::string& r, const std::string&  s);
			*/
		static int verify(EC_GROUP* group, const std::string& pkey,
			const std::string& id, const std::string& msg, const std::string& sig);

		//group Elliptic curve
		//id  Identity
		//pkey  Public key
		static std::string getZA(EC_GROUP* group, std::string id, const EC_POINT* pkey);

		//Return the hexadecimal private key
        std::string getSkeyHex();

		std::string getSkeyBin();

		//Return the curve selected by CFCA
		//Static variables do not need to be released
		static EC_GROUP* GetCFCAGroup();

		//Generate a new group. If you don't know what this sentence means, please don't call it.
		//The input parameters are the hexadecimal format of p, a, b, xG, yG, n respectively.
		//Return NULL if it fails
		//Successfully return a new group which needs to be released manually.
		static EC_GROUP* NewGroup(GROUP_TYPE type,std::string phex, std::string ahex, std::string bhex, std::string xGhex, std::string yGhex, std::string nhex);
		static std::string Bn2FixedString(BIGNUM* bn, int len);
    private:
        EC_GROUP* group_;

		static EC_GROUP* cfca_group_;
    };

}

#endif
