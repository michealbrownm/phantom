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
#include <3rd/libscrypt/libscrypt.h>
#include <json/json.h>
#include <utils/headers.h>
#include <utils/random.h>
#include "private_key.h"
#include "key_store.h"

namespace phantom {
	KeyStore::KeyStore() {}

	KeyStore::~KeyStore() {}

	/*
	{
	"version" : 1,
	"scrypt_params" : {"n": 16384, "p": 8, "r" : 1,"salt" :"1234"},
	"aes128ctr_iv" : "",
	"cypher_text": ""
	}
	*/

	bool KeyStore::Generate(const std::string &password, Json::Value &key_store, std::string &new_priv_key) {
		//produce 32 byte random
		std::string salt;
		utils::GetStrongRandBytes(salt);

		std::string aes_iv;
		utils::GetStrongRandBytes(aes_iv);
		aes_iv.resize(16);

		//produce
		uint64_t n = 16384;
		uint32_t r = 8;
		uint32_t p = 1;

		std::string dk;
		dk.resize(32);
		libscrypt_scrypt((uint8_t *)password.c_str(), password.size(), (uint8_t *)salt.c_str(), salt.size(), n, r, p, (uint8_t *)dk.c_str(), dk.size());

		//printf("%s\n", utils::String::BinToHexString(dk).c_str());

		std::string address;
		if (new_priv_key.empty()) {
			PrivateKey priv_key(SIGNTYPE_ED25519);
			if (!priv_key.IsValid()) {
				return false;
			}
			new_priv_key = priv_key.GetEncPrivateKey();
			address = priv_key.GetEncAddress();
		}
		else {
			PrivateKey priv_key(new_priv_key);
			if (!priv_key.IsValid()) {
				return false;
			}

			address = priv_key.GetEncAddress();
		}

		utils::AesCtr aes((uint8_t *)aes_iv.c_str(), dk);
		
		std::string cyper_text;
		aes.Encrypt(new_priv_key, cyper_text);

		key_store["version"] = 2;
		Json::Value &scrypt_params = key_store["scrypt_params"];
		scrypt_params["n"] = n;
		scrypt_params["r"] = r;
		scrypt_params["p"] = p;
		scrypt_params["salt"] = utils::String::BinToHexString(salt);
		key_store["aesctr_iv"] = utils::String::BinToHexString(aes_iv);
		key_store["cypher_text"] = utils::String::BinToHexString(cyper_text);
		key_store["address"] = address;

		return true;
	}

	bool KeyStore::From(const Json::Value &key_store, const std::string &password, std::string &priv_key) {

		int32_t version = key_store["version"].asInt();
		const Json::Value &scrypt_params = key_store["scrypt_params"];
		uint64_t n = scrypt_params["n"].asUInt64();
		uint32_t r = scrypt_params["r"].asUInt();
		uint32_t p = scrypt_params["p"].asUInt();
		std::string salt = utils::String::HexStringToBin(scrypt_params["salt"].asString());
	
		std::string aes_iv;
		int32_t nkeylen = 16;
		if (version == 1) {
			aes_iv = utils::String::HexStringToBin(key_store["aes128ctr_iv"].asString());
			nkeylen = 16;
		}
		else if (version == 2) {
			aes_iv = utils::String::HexStringToBin(key_store["aesctr_iv"].asString());
			nkeylen = 32;
		}
		std::string cypher_text = utils::String::HexStringToBin(key_store["cypher_text"].asString());
		std::string address = key_store["address"].asString();

		if (aes_iv.size() != 16){
			return false;
		}

		std::string dk;
		dk.resize(32);
		int32_t ret = libscrypt_scrypt((uint8_t *)password.c_str(), password.size(), (uint8_t *)salt.c_str(), salt.size(), n, r, p, (uint8_t *)dk.c_str(), dk.size());
		if (ret != 0 ){
			return false;
		}

		//printf("%s\n", utils::String::BinToHexString(dk).c_str());

		dk.resize(nkeylen);
		utils::AesCtr aes((uint8_t *)aes_iv.c_str(), dk);
		std::string priv_key_de;
		aes.Encrypt(cypher_text, priv_key_de);
		
		PrivateKey key(priv_key_de);
		if (!key.IsValid()){
			return false;
		} 

		if (key.GetEncAddress() != address ){
			return false;
		} 

		priv_key = priv_key_de;
		return true;
	}
}
