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

#include "common.h"
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/ecdsa.h>
#include <openssl/ecdh.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/obj_mac.h>
#include <openssl/x509.h>
#include "ecc_sm2.h"
#include "sm3.h"
#include "strings.h"
namespace utils {

#define free_bn(x) do{ 	\
	if (x != NULL)		\
		BN_free (x);	\
	}while (0) 

#define free_ec_point(x) do{	\
	if (x != NULL)				\
		EC_POINT_free (x);		\
}while (0)	


	//#define ABORT do { \
	//	fflush(stdout); \
	//	fdebug_prt(stderr, "%s:%d: ABORT\n", __FILE__, __LINE__); \
	//	exit (1);	\
	//} while (0)


#define  handle_err {printf("error\n"); assert (false);}

	EC_GROUP* EccSm2::cfca_group_ = NULL;
	EccSm2::EccSm2(EC_GROUP* curv) :group_(curv) {
		valid_ = false;
		dA_ = BN_new();
		pkey_ = EC_POINT_new(curv);
	}

	EccSm2::~EccSm2() {
		free_bn(dA_);
		free_ec_point(pkey_);
	}

	EC_GROUP* EccSm2::GetCFCAGroup() {
		if (cfca_group_ != NULL) {
			return cfca_group_;
		}
		BN_CTX* ctx = BN_CTX_new();
		BN_CTX_start(ctx);
		EC_POINT* G = NULL;
		BIGNUM* p = BN_CTX_get(ctx);
		BIGNUM* a = BN_CTX_get(ctx);
		BIGNUM* b = BN_CTX_get(ctx);
		BIGNUM* xG = BN_CTX_get(ctx);
		BIGNUM* yG = BN_CTX_get(ctx);
		BIGNUM* n = BN_CTX_get(ctx);
		do {
			BN_hex2bn(&p,  "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF");
			BN_hex2bn(&a,  "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC");
			BN_hex2bn(&b,  "28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93");
			BN_hex2bn(&xG, "32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7");
			BN_hex2bn(&yG, "BC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0");
			BN_hex2bn(&n,  "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123");
			cfca_group_ = EC_GROUP_new(EC_GFp_mont_method());
			if (!EC_GROUP_set_curve_GFp(cfca_group_, p, a, b, ctx)) {
				break;
			}
			G = EC_POINT_new(cfca_group_);
			EC_POINT_set_affine_coordinates_GFp(cfca_group_, G, xG, yG, ctx);
			if (!EC_GROUP_set_generator(cfca_group_, G, n, BN_value_one())) {
				break;
			}
		} while (false);
		free_ec_point(G);
		return cfca_group_;
	}
	 
	std::string EccSm2::Bn2FixedString(BIGNUM* bn, int len){
		std::string result("");
		
		unsigned char tmp[1024];

		int l = BN_bn2bin(bn, tmp);
		if (l <= len){
			result.append(len - l, (char)0);
			result.append((char*)tmp, l);
		}
		else{
			result.append((char*)(tmp + l - len), len);
		}

		return result;
	}

	EC_GROUP* EccSm2::NewGroup(GROUP_TYPE type, std::string phex, std::string ahex, std::string bhex, std::string xGhex, std::string yGhex, std::string nhex) {
		EC_POINT *G = NULL;
		EC_POINT *R = NULL;
		EC_GROUP* group = NULL;
		BN_CTX *ctx = BN_CTX_new();
		BN_CTX_start(ctx);
		BIGNUM* p = BN_CTX_get(ctx);
		BIGNUM* a = BN_CTX_get(ctx);
		BIGNUM* b = BN_CTX_get(ctx);
		BIGNUM* xG = BN_CTX_get(ctx);
		BIGNUM* yG = BN_CTX_get(ctx);
		BIGNUM* n = BN_CTX_get(ctx);
		BIGNUM* bn_4a3 = BN_CTX_get(ctx);
		BIGNUM* bn_27b2 = BN_CTX_get(ctx);
		BIGNUM* bn_4a3_add_27b2 = BN_CTX_get(ctx);
		BIGNUM* bn_191 = BN_CTX_get(ctx);

		BN_hex2bn(&p, phex.c_str());
		BN_hex2bn(&a, ahex.c_str());
		BN_hex2bn(&b, bhex.c_str());
		BN_hex2bn(&xG, xGhex.c_str());
		BN_hex2bn(&yG, yGhex.c_str());
		BN_hex2bn(&n, nhex.c_str());
		std::string err_desc;
		BN_hex2bn(&bn_191, "400000000000000000000000000000000000000000000000");
		bool ret = false;
		do {
			if (type == GFP) {
				//n>2^191
				if (BN_cmp(n, bn_191) <= 0) {
					err_desc = "n is smaller than 2^191";
					break;
				}
				if (!BN_is_prime_ex(p, BN_prime_checks, NULL, NULL)) {
					err_desc = "p is not a prime number";
					break;
				}
				if (!BN_is_odd(p)) {
					err_desc = "p is not a odd number";
					break;
				}
				group = EC_GROUP_new(EC_GFp_mont_method());
				if (group == NULL) {
					err_desc = "internel error";
					break;
				}
				if (!EC_GROUP_set_curve_GFp(group, p, a, b, ctx)) {
					err_desc = "internel error";
					break;
				}
				G = EC_POINT_new(group);
				EC_POINT_set_affine_coordinates_GFp(group, G, xG, yG, ctx);

				if (!EC_GROUP_set_generator(group, G, n, BN_value_one())) {
					err_desc = "internel error";
					break;
				}
				//bn_4a3=4*a^3
				BN_sqr(bn_4a3, a, ctx);
				BN_mul(bn_4a3, bn_4a3, a, ctx);
				BN_mul_word(bn_4a3, 4);
				//bn_27b2=27*b^2
				BN_mul(bn_27b2, b, b, ctx);
				BN_mul_word(bn_27b2, 27);
				//bn_4a3_add_27b2=(4*a^3 + 27*b^)2 mod p
				BN_mod_add(bn_4a3_add_27b2, bn_4a3, bn_27b2, p, ctx);
				if (BN_is_zero(bn_4a3_add_27b2)) {
					err_desc = "(4*a^3 + 27*b^2) mod p = 0";
					break;
				}
				BIGNUM* y2modp = BN_CTX_get(ctx);
				BN_mod_mul(y2modp, yG, yG, p, ctx);
				BIGNUM* tmp = BN_CTX_get(ctx);
				BN_mul(tmp, xG, xG, ctx);
				BN_mul(tmp, tmp, xG, ctx);
				BIGNUM* tmp2 = BN_CTX_get(ctx);
				BN_mul(tmp2, a, xG, ctx);
				BN_add(tmp2, tmp2, b);
				BIGNUM* x3axb = BN_CTX_get(ctx);
				BN_mod_add(x3axb, tmp, tmp2, p, ctx);
				if (BN_cmp(y2modp, x3axb) != 0) {
					err_desc = "y2!=x3+ax+b mod p";
					break;
				}
				//n is a prime number
				if (!BN_is_prime_ex(n, BN_prime_checks, NULL, NULL)) {
					err_desc = "n is not a prime number";
					break;
				}
				R = EC_POINT_new(group);
				EC_POINT_mul(group, R, n, NULL, NULL, ctx);
				if (EC_POINT_is_at_infinity(group, R) != 1) {
					err_desc = "nG != O";
					break;
				}
				ret = true;
			}
			else {
				group = EC_GROUP_new(EC_GF2m_simple_method());
				if (group == NULL) {
					err_desc = "internel error";
					break;
				}
				if (!EC_GROUP_set_curve_GF2m(group, p, a, b, ctx)) {
					err_desc = "internel error";
					break;
				}
				G = EC_POINT_new(group);
				EC_POINT_set_affine_coordinates_GF2m(group, G, xG, yG, ctx);

				if (!EC_GROUP_set_generator(group, G, n, BN_value_one())) {
					err_desc = "internel error";
					break;
				}
			}
			
			if (!EC_GROUP_check(group, ctx)){
				EC_GROUP_free(group);
				return NULL;
				break;
			}
			
		} while (false);
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		free_ec_point(R);
		free_ec_point(G);
		return group;
	}

	std::string EccSm2::getZA(EC_GROUP* group, std::string id, const EC_POINT* pkey) {
		BN_CTX *ctx = BN_CTX_new();
		BN_CTX_start(ctx);

		BIGNUM *xA = BN_CTX_get(ctx);
		BIGNUM* yA = BN_CTX_get(ctx);
		unsigned char bin[MAX_BITS];
		int len = 0;
		if (EC_METHOD_get_field_type(EC_GROUP_method_of(group)) == NID_X9_62_prime_field) {
			EC_POINT_get_affine_coordinates_GFp(group, pkey, xA, yA, NULL);
		}
		else {
			EC_POINT_get_affine_coordinates_GF2m(group, pkey, xA, yA, NULL);
		}

		const EC_POINT* G = EC_GROUP_get0_generator(group);
		BIGNUM* xG = BN_CTX_get(ctx);
		BIGNUM* yG = BN_CTX_get(ctx);
		if (EC_METHOD_get_field_type(EC_GROUP_method_of(group)) == NID_X9_62_prime_field)
			EC_POINT_get_affine_coordinates_GFp(group, G, xG, yG, ctx);
		else
			EC_POINT_get_affine_coordinates_GF2m(group, G, xG, yG, ctx);

		BIGNUM* a = BN_CTX_get(ctx);
		BIGNUM* b = BN_CTX_get(ctx);
		EC_GROUP_get_curve_GFp(group, NULL, a, b, ctx);
		///The national standard does not have English annotations
		//////////////////////////////////////////////////////////////////////////
		uint32_t entla = id.length() * 8;
		std::string za = "";
		//Combine ENTLA
		unsigned char c1 = entla >> 8;
		unsigned char c2 = entla & 0xFF;
		za.push_back(c1);
		za.push_back(c2);
		//Combine user ID
		za += id;

		//Combine a
		za += Bn2FixedString(a, 32);

		//Combine b
		za += Bn2FixedString(b, 32);

		//Combine xG
		za += Bn2FixedString(xG, 32);

		//Combine yG
		za += Bn2FixedString(yG, 32);

		//Combine xA
		za += Bn2FixedString(xA, 32);

		//Combine xA
		za += Bn2FixedString(yA, 32);
		
		std::string ZA = utils::Sm3::Crypto(za);
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		//printf("za=%s\n", String::BinToHexString(za).c_str());
		//printf("ZA=%s\n", String::BinToHexString(ZA).c_str());
		return ZA;
	}

	bool  EccSm2::From(std::string skey_bin) {
		valid_ = false;
		skey_bin_ = skey_bin;
		BN_CTX* ctx = BN_CTX_new();
		BN_CTX_start(ctx);
		BIGNUM* x = BN_CTX_get(ctx);
		BIGNUM* y = BN_CTX_get(ctx);
		BIGNUM* order = BN_CTX_get(ctx);
		EC_GROUP_get_order(group_, order, ctx);
		do {
			BN_bin2bn((const unsigned char*)skey_bin_.c_str(), skey_bin_.length(), dA_);
			if (BN_cmp(dA_, order) == 0) {
				error_ = "dA must be less than order i.e. n ";
				break;;
			}
			if (!EC_POINT_mul(group_, pkey_, dA_, NULL, NULL, NULL)) {
				error_ = "unknown error";
				break;
			}

			if (EC_METHOD_get_field_type(EC_GROUP_method_of(group_)) == NID_X9_62_prime_field) {
				if (!EC_POINT_get_affine_coordinates_GFp(group_, pkey_, x, y, ctx)) {
					error_ = "unknown error";
					break;
				}
			}
			else {
				if (!EC_POINT_get_affine_coordinates_GF2m(group_, pkey_, x, y, ctx)) {
					error_ = "unknown error";
					break;
				}
			}
			//The first version of the national standard requires that the first byte of the X, Y coordinate of the public key cannot be 0.
			//if (BN_num_bytes(order) > BN_num_bytes(x) || BN_num_bytes(order) > BN_num_bytes(y)) {
			//	error_ = "SM2 rule: the first byte of publickey can not be zero"; 
			//	break;
			//}
			valid_ = true;
		} while (false);
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);

		return valid_;
	}

	bool  EccSm2::NewRandom() {
		BN_CTX* ctx = BN_CTX_new();
		BN_CTX_start(ctx);
		BIGNUM* x = BN_CTX_get(ctx);
		BIGNUM* y = BN_CTX_get(ctx);
		BIGNUM* order = BN_CTX_get(ctx);
		EC_GROUP_get_order(group_, order, ctx);

		do {
			if (!BN_rand_range(dA_, order)) {
				continue;
			}
			if (BN_cmp(dA_, order) == 0) {
				continue;
			}
			if (!EC_POINT_mul(group_, pkey_, dA_, NULL, NULL, NULL))
				continue;

			if (EC_METHOD_get_field_type(EC_GROUP_method_of(group_)) == NID_X9_62_prime_field) {
				if (!EC_POINT_get_affine_coordinates_GFp(group_, pkey_, x, y, ctx)) {
					continue;
				}
			}
			else {
				if (!EC_POINT_get_affine_coordinates_GF2m(group_, pkey_, x, y, ctx)) {
					continue;
				}
			}
			
			//The first version of the national standard requires that the first byte of the X, Y coordinate of the public key cannot be 0.
			if (BN_num_bytes(order) != BN_num_bytes(x) || BN_num_bytes(order) != BN_num_bytes(y)) {
				continue;
			}
			break;
		} while (true);
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		valid_ = true;
		return valid_;
	}

	std::string EccSm2::getSkeyHex() {
		char* buff = BN_bn2hex(dA_);
		std::string str = "";
		str.append(buff);
		OPENSSL_free(buff);
		return str;
	}

	std::string EccSm2::getSkeyBin() {
		return Bn2FixedString(dA_, 32);
	}

	std::string EccSm2::Sign(const std::string& id, const std::string& msg) {
		std::string sigr;
		std::string sigs;

		if (!valid_) {
			return "";
		}
		bool ok = false;

		BN_CTX *ctx = BN_CTX_new();
		BN_CTX_start(ctx);
		EC_POINT* pt1 = EC_POINT_new(group_);
		std::string M, stre, ZA;
		int dgstlen;
		unsigned char dgst[32];
		BIGNUM* r = BN_CTX_get(ctx);
		BIGNUM* s = BN_CTX_get(ctx);
		BIGNUM* e = BN_CTX_get(ctx);
		BIGNUM* bn = BN_CTX_get(ctx);
		BIGNUM* k = BN_CTX_get(ctx);
		BIGNUM* x1 = BN_CTX_get(ctx);
		BIGNUM* order = BN_CTX_get(ctx);
		BIGNUM* p = BN_CTX_get(ctx);

		if (!group_ || !dA_) {
			goto end;
		}

		if (!r || !s || !ctx || !order || !e || !bn) {
			goto end;
		}
		EC_GROUP_get_order(group_, order, ctx);
		EC_GROUP_get_curve_GFp(group_, p, NULL, NULL, ctx);

		//Step 1  M^ = ZA||M
		ZA = getZA(group_, id, pkey_);
		M = ZA + msg;

		//Step 2 e=Hv(M^)
		stre = utils::Sm3::Crypto(M);

		dgstlen = sizeof(dgst) / sizeof(unsigned char);
		memcpy(dgst, stre.c_str(), dgstlen);
		if (!BN_bin2bn(dgst, dgstlen, e)) {
			goto end;
		}

		do {
			//Step 3  generate random k [1,n-1]
			do {
				do {
					if (!BN_rand_range(k, order)) {
						goto end;
					}
				} while (BN_is_zero(k) || (BN_ucmp(k, order) == 0));

				//Step 4  calculate node G pt1(x1,y1) = [K]
				if (!EC_POINT_mul(group_, pt1, k, NULL, NULL, ctx)) {
					goto end;
				}

				//Obtain the coordinate for pt1
				if (EC_METHOD_get_field_type(EC_GROUP_method_of(group_)) == NID_X9_62_prime_field) {
					if (!EC_POINT_get_affine_coordinates_GFp(group_, pt1, x1, NULL, ctx)) {
						goto end;
					}
				}
				else /* NID_X9_62_characteristic_two_field */ {
					if (!EC_POINT_get_affine_coordinates_GF2m(group_, pt1, x1, NULL, ctx)) {
						goto end;
					}
				}

				if (!BN_nnmod(x1, x1, order, ctx)) {
					goto end;
				}

			} while (BN_is_zero(x1));

			//Step 5  calculate r = (e + x1) mod n
			BN_copy(r, x1);
			if (!BN_mod_add(r, r, e, order, ctx)) {
				goto end;
			}

			if (!BN_mod_add(bn, r, k, order, ctx)) {
				goto end;
			}

			//Ensure r!=0 and r+k!=n namely (r+k) != 0 mod n 
			if (BN_is_zero(r) || BN_is_zero(bn)) {
				continue;
			}

			//Step 6  calculate s = ((1 + d)^-1 * (k - rd)) mod n 
			if (!BN_one(bn)) {
				goto end;
			}

			if (!BN_mod_add(s, dA_, bn, order, ctx)) {
				goto end;
			}
			if (!BN_mod_inverse(s, s, order, ctx)) {
				goto end;
			}

			if (!BN_mod_mul(bn, r, dA_, order, ctx)) {
				goto end;
			}
			if (!BN_mod_sub(bn, k, bn, order, ctx)) {
				goto end;
			}
			if (!BN_mod_mul(s, s, bn, order, ctx)) {
				goto end;
			}

			//Ensure s != 0 
			if (!BN_is_zero(s)) {
				break;
			}
			//Step seven Output r and s
		} while (1);

		ok = true;
	end:

		int olen = BN_num_bytes(p);

		sigr.resize(0);
		sigs.resize(0);
		sigr = Bn2FixedString(r, 32);
		sigs = Bn2FixedString(s, 32);
		free_ec_point(pt1);
		BN_CTX_free(ctx);
		return sigr + sigs;
	}


	int EccSm2::verify(EC_GROUP* group, const std::string& pkey, 
		const std::string& id, const std::string& msg, const std::string& strsig) {
		std::string px = "";
		std::string py = "";
		int len = (pkey.size() - 1) / 2;
		px = pkey.substr(1, len);
		py = pkey.substr(1 + len, len);

		std::string sigr = strsig.substr(0, strsig.size() / 2);
		std::string sigs = strsig.substr(strsig.size() / 2, strsig.size() / 2);

		int ret = -1;
		EC_POINT *pub_key = NULL;
		EC_POINT *point = NULL;
		BN_CTX *ctx = NULL;

		ECDSA_SIG* sig = NULL;

		std::string M, ZA, stre;

		pub_key = EC_POINT_new(group);
		point = EC_POINT_new(group);
		unsigned char dgst[32];
		int dgstlen;

		ctx = BN_CTX_new();
		BN_CTX_start(ctx);

		BIGNUM* xp = BN_CTX_get(ctx);
		BIGNUM* yp = BN_CTX_get(ctx);
		BIGNUM*x1 = BN_CTX_get(ctx);
		BIGNUM*R = BN_CTX_get(ctx);
		BIGNUM *order = BN_CTX_get(ctx);
		BIGNUM *e = BN_CTX_get(ctx);
		BIGNUM *t = BN_CTX_get(ctx);

		EC_GROUP_get_order(group, order, ctx);
		BN_bin2bn((const unsigned char*)px.c_str(), px.size(), xp);
		BN_bin2bn((const unsigned char*)py.c_str(), py.size(), yp);
		if (EC_METHOD_get_field_type(EC_GROUP_method_of(group)) == NID_X9_62_prime_field) {
			EC_POINT_set_affine_coordinates_GFp(group, pub_key, xp, yp, NULL);
		}
		else {
			EC_POINT_set_affine_coordinates_GF2m(group, pub_key, xp, yp, NULL);
		}

		sig = ECDSA_SIG_new();
		BN_bin2bn((const unsigned char*)sigr.c_str(), sigr.size(), sig->r);
		BN_bin2bn((const unsigned char*)sigs.c_str(), sigs.size(), sig->s);

		e = BN_CTX_get(ctx);
		t = BN_CTX_get(ctx);
		if (!ctx || !order || !e || !t) {
			goto end;
		}

		// Step 1 and 2: r, s are in the range of [1, n-1] and r + s != 0 (mod n) 
		if (BN_is_zero(sig->r) ||
			BN_is_negative(sig->r) ||
			BN_ucmp(sig->r, order) >= 0 ||
			BN_is_zero(sig->s) ||
			BN_is_negative(sig->s) ||
			BN_ucmp(sig->s, order) >= 0) {
			ret = 0;
			goto end;
		}

		//Step 5  (r' + s') != 0 mod n
		if (!BN_mod_add(t, sig->r, sig->s, order, ctx)) {
			goto end;
		}
		if (BN_is_zero(t)) {
			ret = 0;
			goto end;
		}

		//Step 3  Calculate _M = ZA||M'
		ZA = getZA(group, id, pub_key);
		M = ZA + msg;

		//Step 4  calculate e' = Hv(_M)
		stre = utils::Sm3::Crypto(M);
		memcpy(dgst, stre.c_str(), stre.length());
		dgstlen = stre.length();

		if (!BN_bin2bn(dgst, dgstlen, e)) {
			goto end;
		}

		//Step 6 calculate point (x',y')=sG + tP  P is public key point

		if (!EC_POINT_mul(group, point, sig->s, pub_key, t, ctx)) {
			goto end;
		}
		if (EC_METHOD_get_field_type(EC_GROUP_method_of(group)) == NID_X9_62_prime_field) {
			if (!EC_POINT_get_affine_coordinates_GFp(group, point, x1, NULL, ctx)) {
				goto end;
			}
		}
		else /* NID_X9_62_characteristic_two_field */ {
			if (!EC_POINT_get_affine_coordinates_GF2m(group, point, x1, NULL, ctx)) {
				goto end;
			}
		}
		if (!BN_nnmod(x1, x1, order, ctx)) {
			goto end;
		}

		//Step 7  R=(e+x') mod n

		if (!BN_mod_add(R, x1, e, order, ctx)) {
			goto end;
		}

		BN_nnmod(R, R, order, ctx);

		if (BN_ucmp(R, sig->r) == 0) {
			ret = 1;
		}
		else {
			//printf("%s:%s\n", BN_bn2hex(R), BN_bn2hex(sig->r));
			//printf("ZA=%s\n", utils::String::BinToHexString(ZA).c_str());
			//printf("e=%s\n", utils::String::BinToHexString(stre).c_str());
			ret = 0;
		}

	end:
		free_ec_point(point);
		free_ec_point(pub_key);

		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		ECDSA_SIG_free(sig);
		if (ret != 1) {
			int x = 2;
		}

		return ret;
	}


	std::string EccSm2::GetPublicKey() {
		std::string xPA("");
		std::string yPA("");
		if (!valid_) {
			return "";
		}
		BN_CTX *ctx = BN_CTX_new();
		BN_CTX_start(ctx);
		BIGNUM *bn_x = BN_CTX_get(ctx);
		BIGNUM *bn_y = BN_CTX_get(ctx);

		if (EC_METHOD_get_field_type(EC_GROUP_method_of(group_)) == NID_X9_62_prime_field)
			EC_POINT_get_affine_coordinates_GFp(group_, pkey_, bn_x, bn_y, NULL);
		else
			EC_POINT_get_affine_coordinates_GF2m(group_, pkey_, bn_x, bn_y, NULL);

		unsigned char xx[MAX_BITS];
		BIGNUM* order = BN_CTX_get(ctx);
		EC_GROUP_get_order(group_, order, ctx);

		BIGNUM* p = BN_CTX_get(ctx);
		EC_GROUP_get_curve_GFp(group_, p, NULL, NULL, ctx);
		int olen = BN_num_bytes(p);

		xPA = Bn2FixedString(bn_x, 32);
		yPA = Bn2FixedString(bn_y, 32);

		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		std::string out;
		out.push_back(4);
		out += xPA;
		out += yPA;
		return out;
	}

}
