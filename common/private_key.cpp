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

#include <openssl/ecdsa.h>
#include <openssl/rsa.h>
#include <openssl/ripemd.h>
#include <utils/logger.h>
#include <utils/crypto.h>
#include <utils/random.h>
#include <utils/sm3.h>
#include <utils/strings.h>
#include "general.h"
#include "private_key.h"

namespace phantom {

	std::string EncodeAddress(const std::string& address){
		return utils::Base58::Encode(address);
	}
	std::string DecodeAddress(const std::string& address){
		return utils::Base58::Decode(address);
	}
	std::string EncodePublicKey(const std::string& key){
		return utils::String::BinToHexString(key);
	}
	std::string DecodePublicKey(const std::string& key){
		return utils::String::HexStringToBin(key);
	}
	std::string EncodePrivateKey(const std::string& key){
		return utils::Base58::Encode(key);
	}
	std::string DecodePrivateKey(const std::string& key){
		return utils::Base58::Decode(key);
	}

	std::string CalcHash(const std::string &value,const SignatureType &sign_type) {
		std::string hash;
		if (sign_type == SIGNTYPE_CFCASM2) {
			hash = utils::Sm3::Crypto(value);
		}
		else {
			hash = utils::Sha256::Crypto(value);
		}
		return hash;
	}

	bool GetPublicKeyElement(const std::string &encode_pub_key, PrivateKeyPrefix &prefix, SignatureType &sign_type, std::string &raw_data){
		std::string buff = DecodePublicKey(encode_pub_key);
		if (buff.size() < 6)
			return false;

		uint8_t a = (uint8_t)buff.at(0);
		uint8_t b = (uint8_t)buff.at(1);
		
		PrivateKeyPrefix prefix_tmp = (PrivateKeyPrefix)a;
		if (prefix_tmp != PUBLICKEY_PREFIX)
			return false;

		SignatureType sign_type_tmp = (SignatureType)b;
		size_t datalen = buff.size() - 6;

		bool ret = true;
		switch (sign_type_tmp) {
		case SIGNTYPE_ED25519:{
			ret = (ED25519_PUBLICKEY_LENGTH == datalen);
			break;
		}
		case SIGNTYPE_CFCASM2:{
			ret = (SM2_PUBLICKEY_LENGTH == datalen);
			break;
		}
		default:
			ret = false;
		}

		if (ret){
			//Check checksum
			std::string checksum = buff.substr(buff.size() - 4);
			std::string hash1 = CalcHash(buff.substr(0, buff.size() - 4), sign_type_tmp);
			std::string hash2 = CalcHash(hash1, sign_type_tmp);
			if (checksum.compare(hash2.substr(0, 4)))
				return false;

			prefix = prefix_tmp;
			sign_type = sign_type_tmp;
			raw_data = buff.substr(2, buff.size() - 6);
		}
		return ret;
	}

	bool GetKeyElement(const std::string &encode_key, PrivateKeyPrefix &prefix, SignatureType &sign_type, std::string &raw_data) {
		PrivateKeyPrefix prefix_tmp;
		SignatureType sign_type_tmp = SIGNTYPE_NONE;
		std::string buff = DecodeAddress(encode_key);
		/***************************************
        modified  by luquanhui   modify 20181109

		if (buff.size() == 27 && (uint8_t)buff.at(0) == 0X01 && (uint8_t)buff.at(1) == 0X56){// Address
			prefix_tmp = ADDRESS_PREFIX;
		}
		***************************************/
	    
       /***************************************
        modified  by luquanhui   begin 20181109
		//
		***************************************/

		if (buff.size() == 29 && (uint8_t)buff.at(0) == 0X12 && (uint8_t)buff.at(1) == 0X1E  && (uint8_t)buff.at(2) == 0X0D && (uint8_t)buff.at(3) == 0XA4){// Address
			prefix_tmp = ADDRESS_PREFIX;
		}/****  modified  by luquanhui   end 20181109   *****/
		else if (buff.size() == 41 && (uint8_t)buff.at(0) == 0XDA && (uint8_t)buff.at(1) == 0X37 && (uint8_t)buff.at(2) == 0X9F){//private key
			prefix_tmp = PRIVATEKEY_PREFIX;
		}
		else{
			return false;
		}     
		
	   

		bool ret = true;
		if (prefix_tmp == ADDRESS_PREFIX) {
        /***************************************
        modified  by luquanhui   modify 20181109
		//
		
			uint8_t a = (uint8_t)buff.at(2); 
			sign_type_tmp = (SignatureType)a;   
			size_t datalen = buff.size() - 7;
        ***************************************/

		/***************************************
        modified  by luquanhui   begin 20181109
		//
		***************************************/
			uint8_t a = (uint8_t)buff.at(4); 
			sign_type_tmp = (SignatureType)a;   
			size_t datalen = buff.size() - 9;
        /***************************************
        modified  by luquanhui   end 20181109
		//
		***************************************/

			switch (sign_type_tmp) {
			case SIGNTYPE_ED25519:{
				ret = (ED25519_ADDRESS_LENGTH == datalen);
				break;
			}
			case SIGNTYPE_CFCASM2:{
				ret = (SM2_ADDRESS_LENGTH == datalen);
				break;
			}
			default:
				ret = false;
			}
		}
		else if (prefix_tmp == PRIVATEKEY_PREFIX) {
			uint8_t a = (uint8_t)buff.at(3);  
			sign_type_tmp = (SignatureType)a;
			size_t datalen = buff.size() - 9;
			switch (sign_type_tmp) {
			case SIGNTYPE_ED25519:{
				ret = (ED25519_PRIVATEKEY_LENGTH == datalen);
				break;
			}
			case SIGNTYPE_CFCASM2:{
				ret = (SM2_PRIVATEKEY_LENGTH == datalen);
				break;
			}
			default:
				ret = false;
			}
		}
		else {
			ret = false;
		}

		if (ret){
			//Checksum
			std::string checksum = buff.substr(buff.size() - 4);
			std::string hash1 = CalcHash(buff.substr(0, buff.size() - 4), sign_type_tmp);
			std::string hash2 = CalcHash(hash1, sign_type_tmp);
			if (checksum.compare(hash2.substr(0, 4)))
				return false;

			prefix = prefix_tmp;
			sign_type = sign_type_tmp;
			if (prefix_tmp == ADDRESS_PREFIX) {

				
		/***************************************
        modified  by luquanhui    20181109
		//

        raw_data = buff.substr(3, buff.size() - 7);

		***************************************/


		/***************************************
        modified  by luquanhui   begin 20181109
		//
		***************************************/
				raw_data = buff.substr(5, buff.size() - 9);

		/***************************************
        modified  by luquanhui   end 20181109
		//
		***************************************/

			}
			else if (prefix_tmp == PRIVATEKEY_PREFIX) {
				raw_data = buff.substr(4, buff.size() - 9);
			}
		} 

		return ret;
	}

	std::string GetSignTypeDesc(SignatureType type) {
		switch (type) {
		case SIGNTYPE_CFCASM2: return "sm2";
		case SIGNTYPE_ED25519: return "ed25519";
		}

		return "";
	}

	SignatureType GetSignTypeByDesc(const std::string &desc) {
		
		if (desc == "sm2") {
			return SIGNTYPE_CFCASM2;
		}
		else if (desc == "ed25519") {
			return SIGNTYPE_ED25519;
		}
		return SIGNTYPE_NONE;
	}

	PublicKey::PublicKey() :valid_(false), type_(SIGNTYPE_ED25519) {}

	PublicKey::~PublicKey() {}

	PublicKey::PublicKey(const std::string &encode_pub_key) {
		do {
			PrivateKeyPrefix prefix;
			if (GetPublicKeyElement(encode_pub_key, prefix, type_, raw_pub_key_)){
				valid_ = (prefix == PUBLICKEY_PREFIX);
			}
		} while (false);
	}

	void PublicKey::Init(std::string rawpkey) {
		raw_pub_key_ = rawpkey;
	}

	bool PublicKey::IsAddressValid(const std::string &encode_address) {
		do {
			PrivateKeyPrefix prefix;
			SignatureType sign_type;
			std::string raw_pub_key;
			if (GetKeyElement(encode_address, prefix, sign_type, raw_pub_key)){
				return (prefix == ADDRESS_PREFIX);
			}
		} while (false);

		return false;
	}

	std::string PublicKey::GetEncAddress() const {
		
		std::string str_result = "";
		//Append prefix (phantom 0XE6 0X9A 0X73 0XFF)

		/**************************************
	    modified  by luquanhui 20181109
		//Append prefix (bu)
		
		str_result.push_back((char)0X01);
		str_result.push_back((char)0X56);

		//Append version 1byte
		str_result.push_back((char)type_);
        
        ***************************************/


        /***************************************
        modified  by luquanhui   begin 20181109
		//Append prefix (phos)
		***************************************/
        str_result.push_back((char)0X12);
		str_result.push_back((char)0X1E);
		str_result.push_back((char)0X0D);
		str_result.push_back((char)0XA4);

        //Append version 1byte
		str_result.push_back((char)type_);

	    /**************************************
        modified  by luquanhui   end  20181109
		***************************************/

		//Append public key 20byte
		std::string hash = CalcHash(raw_pub_key_,type_);
		str_result.append(hash.substr(12));

		//Append checksum 4byte
		std::string hash1, hash2;
		hash1 = CalcHash(str_result, type_);
		hash2 = CalcHash(hash1, type_);

		str_result.append(hash2.c_str(), 4);
		return EncodeAddress(str_result);
	}

	std::string PublicKey::GetRawPublicKey() const {
		return raw_pub_key_;
	}

	std::string PublicKey::GetEncPublicKey() const {
		
		std::string str_result = "";
		//Append PrivateKeyPrefix
		str_result.push_back((char)PUBLICKEY_PREFIX);

		//Append version
		str_result.push_back((char)type_);

		//Append public key
		str_result.append(raw_pub_key_);

		std::string hash1, hash2;
		hash1 = CalcHash(str_result, type_);
		hash2 = CalcHash(hash1, type_);

		str_result.append(hash2.c_str(), 4);
		return EncodePublicKey(str_result);
	}
	//not modify
	bool PublicKey::Verify(const std::string &data, const std::string &signature, const std::string &encode_public_key) {
		PrivateKeyPrefix prefix;
		SignatureType sign_type;
		std::string raw_pubkey;
		bool valid = GetPublicKeyElement(encode_public_key, prefix, sign_type, raw_pubkey);
		if (!valid || prefix != PUBLICKEY_PREFIX) {
			return false;
		}

		if (signature.size() != 64) { return false; }

		if (sign_type == SIGNTYPE_ED25519 ) {
			return ed25519_sign_open((unsigned char *)data.c_str(), data.size(), (unsigned char *)raw_pubkey.c_str(), (unsigned char *)signature.c_str()) == 0;
		}
		else if (sign_type == SIGNTYPE_CFCASM2) {
			return utils::EccSm2::verify(utils::EccSm2::GetCFCAGroup(), raw_pubkey, "1234567812345678", data, signature) == 1;
		}
		else{
			LOG_ERROR("Failed to verify. Unknown signature type(%d)", sign_type);
		}
		return false;
	}

	//Generate keypair according to signature type.
	PrivateKey::PrivateKey(SignatureType type) {
		std::string raw_pub_key = "";
		type_ = type;
		if (type_ == SIGNTYPE_ED25519) {
			utils::MutexGuard guard_(lock_);
			// ed25519;
			raw_priv_key_.resize(32);
			//ed25519_randombytes_unsafe((void*)raw_priv_key_.c_str(), 32);
			if (!utils::GetStrongRandBytes(raw_priv_key_)){
				valid_ = false;
				return;
			}
			raw_pub_key.resize(32);
			ed25519_publickey((const unsigned char*)raw_priv_key_.c_str(), (unsigned char*)raw_pub_key.c_str());
		}
		else if (type_ == SIGNTYPE_CFCASM2) {
			utils::EccSm2 key(utils::EccSm2::GetCFCAGroup());
			key.NewRandom();
			raw_priv_key_ = key.getSkeyBin();
			raw_pub_key = key.GetPublicKey();
		}
		else{
			LOG_ERROR("Failed to verify.Unknown signature type(%d)", type_);
		}
		pub_key_.Init(raw_pub_key);
		pub_key_.type_ = type_;
		pub_key_.valid_ = true;
		valid_ = true;
	}

	PrivateKey::~PrivateKey() {}
	//not modify
	bool PrivateKey::From(const std::string &encode_private_key) {
		valid_ = false;
		std::string tmp;

		do {
			PrivateKeyPrefix prefix;
			std::string raw_pubkey;
			valid_ = GetKeyElement(encode_private_key, prefix, type_, raw_priv_key_);
			if (!valid_) {
				return false;
			}

			if (prefix != PRIVATEKEY_PREFIX) {
				valid_ = false;
				return false;
			}

			if (type_ == SIGNTYPE_ED25519) {
				tmp.resize(32);
				ed25519_publickey((const unsigned char*)raw_priv_key_.c_str(), (unsigned char*)tmp.c_str());
			}
			else if (type_ == SIGNTYPE_CFCASM2) {
				utils::EccSm2 skey(utils::EccSm2::GetCFCAGroup());
				skey.From(raw_priv_key_);
				tmp = skey.GetPublicKey();
			}
			else{
				LOG_ERROR("Failed to verify.Unknown signature type(%d)", type_);
			}
			//ToBase58();
			pub_key_.type_ = type_;
			pub_key_.Init(tmp);
			pub_key_.valid_ = true;
			valid_ = true;

		} while (false);
		return valid_;
	}

	PrivateKey::PrivateKey(const std::string &encode_private_key) {
		From(encode_private_key );
	}

	
	//not modify
	std::string PrivateKey::Sign(const std::string &input) const {
		unsigned char sig[10240];
		unsigned int sig_len = 0;

		if (type_ == SIGNTYPE_ED25519) {
			/*	ed25519_signature sig;*/
			ed25519_sign((unsigned char *)input.c_str(), input.size(), (const unsigned char*)raw_priv_key_.c_str(), (unsigned char*)pub_key_.GetRawPublicKey().c_str(), sig);
			sig_len = 64;
		}
		else if (type_ == SIGNTYPE_CFCASM2) {
			utils::EccSm2 key(utils::EccSm2::GetCFCAGroup());
			key.From(raw_priv_key_);
			std::string r, s;
			return key.Sign("1234567812345678", input);
		}
		else{
			LOG_ERROR("Failed to verify.Unknown signature type(%d)", type_);
		}
		std::string output;
		output.append((const char *)sig, sig_len);
		return output;
	}

	std::string PrivateKey::GetEncPrivateKey() const {
		std::string str_result;
		//Append prefix(priv)
		str_result.push_back((char)0XDA);
		str_result.push_back((char)0X37);
		str_result.push_back((char)0X9F);

		//Append version 1
		str_result.push_back((char)type_);

		//Append private key 32
		str_result.append(raw_priv_key_);

		//Append 0X00 to str_result
		str_result.push_back(0X00);

		//Like Bitcoin, we use 4 byte hash check.
		std::string hash1, hash2;
		hash1 = CalcHash(str_result, type_);
		hash2 = CalcHash(hash1, type_);

		str_result.append(hash2.c_str(),4);
		return EncodePrivateKey(str_result);
	}

	std::string PrivateKey::GetEncAddress() const {
		return pub_key_.GetEncAddress();
	}

	std::string PrivateKey::GetEncPublicKey() const {
		return pub_key_.GetEncPublicKey();
	}

	std::string PrivateKey::GetRawPublicKey() const {
		return pub_key_.GetRawPublicKey();
	}

	utils::Mutex PrivateKey::lock_;
}
