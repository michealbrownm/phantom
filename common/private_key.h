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

#ifndef PRIVATE_KEY_H_
#define PRIVATE_KEY_H_

#include <utils/headers.h>
#include <3rd/ed25519-donna/ed25519.h>
#include <utils/ecc_sm2.h>

namespace phantom {
	typedef unsigned char sm2_public_key[65];
	typedef const unsigned char* puchar;
	enum SignatureType {
		SIGNTYPE_NONE,
		SIGNTYPE_ED25519 = 1,
		SIGNTYPE_CFCASM2 = 2,
	};

	enum PrivateKeyPrefix {
		ADDRESS_PREFIX = 0xa0, //0xa0
		PUBLICKEY_PREFIX = 0xb0, //0xb0
		PRIVATEKEY_PREFIX = 0xc0  //0xc0
	};

	enum Ed25519KeyLength {
		ED25519_ADDRESS_LENGTH = 20, // 2+1+20+4
		ED25519_PUBLICKEY_LENGTH = 32, //1+1+32+4
		ED25519_PRIVATEKEY_LENGTH = 32, //3+1+32+1+4
	};

	enum Sm2KeyLength {
		SM2_ADDRESS_LENGTH = 20, //2+1+20+4
		SM2_PUBLICKEY_LENGTH = 65, //1+1+65+4
		SM2_PRIVATEKEY_LENGTH = 32 //3+1+32+1+4
	};
	
	std::string EncodeAddress(const std::string& address);
	std::string DecodeAddress(const std::string& address);
	std::string EncodePublicKey(const std::string& key);
	std::string DecodePublicKey(const std::string& key);
	std::string EncodePrivateKey(const std::string& key);
	std::string DecodePrivateKey(const std::string& key);


	std::string CalcHash(const std::string &value, const SignatureType &sign_type);
	bool GetPublicKeyElement(const std::string &encode_pub_key, PrivateKeyPrefix &prefix, SignatureType &sign_type, std::string &raw_data);
	bool GetKeyElement(const std::string &encode_key, PrivateKeyPrefix &prefix, SignatureType &sign_type, std::string &raw_data);
	std::string GetSignTypeDesc(SignatureType type);
	SignatureType GetSignTypeByDesc(const std::string &desc);
	

	class PublicKey {
		DISALLOW_COPY_AND_ASSIGN(PublicKey);
		friend class PrivateKey;

	public:
		PublicKey();
		PublicKey(const std::string &encode_pub_key);
		~PublicKey();

		void Init(std::string rawpkey);

		std::string GetEncAddress() const;

		std::string GetEncPublicKey() const;

		std::string GetRawPublicKey() const;

		bool IsValid() const { return valid_; }

		SignatureType GetSignType() { return type_; };

		static bool Verify(const std::string &data, const std::string &signature, const std::string &encode_public_key);
		static bool IsAddressValid(const std::string &encode_address);
	private:
		std::string raw_pub_key_;
		bool valid_;
		SignatureType type_;
	};

	class PrivateKey {
		DISALLOW_COPY_AND_ASSIGN(PrivateKey);
	public:
		PrivateKey(SignatureType type);
		PrivateKey(const std::string &encode_private_key);
		bool From(const std::string &encode_private_key);
		~PrivateKey();


		std::string	Sign(const std::string &input) const;
		std::string GetEncPrivateKey() const;
		std::string GetEncAddress() const;
		std::string GetEncPublicKey() const;
		std::string GetRawPublicKey() const;
		bool IsValid() const { return valid_; }
		std::string GetRawPrivateKey() {
			return utils::String::BinToHexString(raw_priv_key_);
		}
		SignatureType GetSignType() { return type_; };
		

	private:
		std::string raw_priv_key_;
		bool valid_;
		SignatureType type_;
		PublicKey pub_key_;
		static utils::Mutex lock_;
	};
};

#endif
