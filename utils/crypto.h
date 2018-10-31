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

#ifndef CRYPTO_H
#define CRYPTO_H

#include <vector>
#include <string>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ecdsa.h>
#include <openssl/sha.h>
#include <openssl/aes.h>


namespace utils {
	//typedef std::vector<unsigned char>  std::string;

	std::string Char2Hex(std::string &blob);

	class Base58 {
	public:
		Base58() {}
		~Base58() {}

		//static std::string Encode(unsigned char*buff, int len, std::string& strOut);
		//static std::string Encode(unsigned char*begin, unsigned char *end);

		static std::string Encode(const std::string &buff);

		//static std::string Encode(std::string buff) {
		//	std::string str;
		//	return Base58::Encode(buff, str);
		//}

		//static int Decode(std::string strIn, unsigned char *out);
		static int Decode(const std::string &strIn, std::string &out);
		static int Decode_old(const std::string &strIn, std::string &out);
		static std::string Decode(const std::string &strIn) {
			std::string out = "";
			Decode(strIn, out);
			return out;
		}
	};

	uint8_t Crc8(uint8_t *ptr, uint16_t len);
	uint8_t Crc8(const std::string &data);

	class Hash {
	public:
		Hash() {};
		~Hash() {};

		virtual void Update(const std::string &input) = 0;
		virtual void Update(const void *buffer, size_t len) = 0;
		virtual std::string Final() = 0;
	};

	class Sha256 : public Hash {
		SHA256_CTX ctx_;
	public:
		Sha256();
		~Sha256();

		void Update(const std::string &input);
		void Update(const void *buffer, size_t len);
		std::string Final();

		static std::string CryptoBase58(const std::string &input) {
			return utils::Base58::Encode(Crypto(input));
		}

		static std::string Crypto(const std::string &input);
		static void Crypto(unsigned char* str, int len, unsigned char *buf);

		static void Crypto(const std::string &input, std::string &str);
		//static void Crypto(unsigned char* str1, int len1, unsigned char *str2, int len2, unsigned char *buf);
		//static void Crypto(unsigned char* str1, int len1, unsigned char* str2, int len2, unsigned char *str3, int len3, unsigned char *buf);
	public:
		static const int SIZE = 32;
	};

	class MD5 {
	public:
		static std::string GenerateMd5File(const char* filename);
		static std::string GenerateMd5File(std::FILE* file);

		static std::string GenerateMD5(const void* dat, size_t len);
		static std::string GenerateMD5(std::string dat);

		static std::string GenerateMD5Sum6(const void* dat, size_t len);
		static std::string GenerateMD5Sum6(std::string dat);

	private:
		static char hb2hex(unsigned char hb);
		static void md5bin(const void* dat, size_t len, unsigned char out[16]);
	};


	class AesCtr {
		struct ctr_state {
			unsigned char ivec[AES_BLOCK_SIZE];
			unsigned int num;
			unsigned char ecount[AES_BLOCK_SIZE];
		};

	public:
		bool key_valid_;
		AES_KEY key;
		int BYTES_SIZE = 1024;
		std::string ckey_;
		unsigned char iv_[16];
		int InitCtr(struct ctr_state *state, const unsigned char iv[16]);
		void Encrypt(unsigned char *indata, unsigned char *outdata, int bytes_read);
		void Encrypt(const std::string &indata, std::string &outdata);
		bool IsValid();
		//AesCtr();
		AesCtr(unsigned char* iv, const std::string &ckey);
		~AesCtr();
	};

	class Aes {
	public:
		Aes() {};
		~Aes() {};
		static std::string Crypto(const std::string &input, const std::string &key);
		static std::string Decrypto(const std::string &input, const std::string &key);
		static std::string CryptoHex(const std::string &input, const std::string &key);
		static std::string HexDecrypto(const std::string &input, const std::string &key);
	};
}

#endif //CRYPTO_H

