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

#include <openssl/err.h>
#include <openssl/ecdsa.h>
#include <openssl/sha.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>

#include "crypto.h"
#include "strings.h"

namespace utils {
	//CRC high bit table
	const uint8_t auchCRCHi[] = {
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
		0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
		0x00, 0xC1, 0x81, 0x40
	};
	//CRC low bit table
	const uint8_t auchCRCLo[] = {
		0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
		0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
		0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
		0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
		0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
		0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
		0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
		0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
		0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
		0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
		0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
		0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
		0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB,
		0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
		0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
		0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
		0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
		0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
		0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
		0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
		0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
		0x41, 0x81, 0x80, 0x40
	};

	//Get crc16 value
	//puchMsg: array to be checked
	//usDataLen: array length
	//Initialize value 0XFFFF, XOR 0x0000, LSB First, polynomial 8005
	uint16_t Get_Crc16(uint8_t *puchMsg, uint16_t usDataLen) {
		uint8_t uchCRCHi = 0xFF; 	//High CRC
		uint8_t uchCRCLo = 0xFF; 	//Low CRC
		uint32_t uIndex; 		//CRC index
		while (usDataLen--) 	
		{
			uIndex = uchCRCHi^*puchMsg++; //Caculate CRC 
			uchCRCHi = uchCRCLo^auchCRCHi[uIndex];
			uchCRCLo = auchCRCLo[uIndex];
			//		uchCRCLo=uchCRCHi^auchCRCHi[uIndex];
			//		uchCRCHi=auchCRCLo[uIndex];
		}
		return (uchCRCHi << 8 | uchCRCLo);
	}
	//CRC8 check
	//ptr:array to be checked
	//len:array length
	//return:CRC8 
	//polynomial 0X31,LSB First,initialize 0X00  x8+x5+x4+1 CRC-8/MAXIM
	uint8_t Crc8(uint8_t *ptr, uint16_t len) {
		uint8_t crc;
		uint8_t i;
		crc = 0;
		while (len--) {
			crc ^= *ptr++;
			for (i = 0; i < 8; i++) {
				if (crc & 0x01)crc = (crc >> 1) ^ 0x8C;
				else crc >>= 1;
			}
		}
		return crc;
	}

	uint8_t Crc8(const std::string &data) {
		return Crc8((uint8_t *)data.c_str(), data.length());
	}
	//get CRC16
	//puchMsg:array to be checked
	//usDataLen:array length
	//Initialize 0XFFFF, XOR 0x0000, LSB First, polynomial A001
	uint16_t Crc16(uint8_t* pdata, uint16_t datalen) {
		uint8_t CRC16Lo, CRC16Hi, CL, CH, SaveHi, SaveLo;
		uint16_t i, Flag;

		CRC16Lo = 0xFF;
		CRC16Hi = 0xFF;
		CL = 0x01;
		CH = 0xA0;

		for (i = 0; i < datalen; i++) {
			CRC16Lo ^= *(pdata + i);
			for (Flag = 0; Flag < 8; Flag++) {
				SaveHi = CRC16Hi;
				SaveLo = CRC16Lo;
				CRC16Hi >>= 1;
				CRC16Lo >>= 1;
				if ((SaveHi & 0x01) == 0x01)
					CRC16Lo |= 0x80;
				if ((SaveLo & 0x01) == 0x01) {
					CRC16Hi ^= CH;
					CRC16Lo ^= CL;
				}
			}
		}
		return (CRC16Hi << 8) | CRC16Lo;
	}

	static const char* kBase58Dictionary = "123456789AbCDEFGHJKLMNPQRSTuVWXYZaBcdefghijkmnopqrstUvwxyz";
	static const int8_t kBase58digits[] = {
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1,
		-1, 9, 34, 11, 12, 13, 14, 15, 16, -1, 17, 18, 19, 20, 21, -1,
		22, 23, 24, 25, 26, 52, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1,
		-1, 33, 10, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46,
		47, 48, 49, 50, 51, 27, 53, 54, 55, 56, 57, -1, -1, -1, -1, -1
	};

	std::string Char2Hex(std::string &blob) {
		std::string str;
		for (size_t i = 0; i < blob.size(); i++) {
			str += String::Format("%02X", blob.at(i));
		}
		return str;
	}

	Sha256::Sha256() {
		SHA256_Init(&ctx_);
	}

	Sha256::~Sha256() {

	}

	void Sha256::Update(const std::string &input) {
		SHA256_Update(&ctx_, input.c_str(), input.size());
	}

	void Sha256::Update(const void *buffer, size_t len) {
		SHA256_Update(&ctx_, buffer, len);
	}

	std::string Sha256::Final() {
		std::string final_str;
		final_str.resize(32);
		SHA256_Final((unsigned char *)final_str.c_str(), &ctx_);
		return final_str;
	}

	std::string Sha256::Crypto(const std::string &input) {
		std::string str_out = "";
		str_out.resize(32);

		SHA256_CTX sha256;
		SHA256_Init(&sha256);
		SHA256_Update(&sha256, input.c_str(), input.size());
		SHA256_Final((unsigned char*)str_out.c_str(), &sha256);

		return str_out;
	}


	void Sha256::Crypto(const std::string &input, std::string &output) {
		output.resize(32);

		SHA256_CTX sha256;
		SHA256_Init(&sha256);
		SHA256_Update(&sha256, input.c_str(), input.size());
		SHA256_Final((unsigned char*)output.c_str(), &sha256);
	}



	void Sha256::Crypto(unsigned char* str, int len, unsigned char *buf) {
		SHA256_CTX sha256;
		SHA256_Init(&sha256);
		SHA256_Update(&sha256, str, len);
		SHA256_Final(buf, &sha256);
	}

	std::string Base58::Encode(const std::string &str_in) {
		std::string strOut;
		int zeros = 0;
		//while (begin != end && *begin == 0)
		//{
		//	begin++;
		//	zeros++;
		//}
		std::size_t first_no_zero = 0;
		for (std::size_t i = 0; i < str_in.size() && str_in.at(i) == 0; i++) {
			first_no_zero = i;
			zeros++;
		}

		std::vector< unsigned char > b58((str_in.size()) * 138 / 100 + 1);
		for (std::size_t i = first_no_zero; i < str_in.size(); i++) {
			int carry = (unsigned int)(str_in.at(i) & 0xFF);
			for (std::vector< unsigned char >::reverse_iterator it = b58.rbegin(); it != b58.rend(); it++) {
				carry += 256 * (*it);
				*it = carry % 58;
				carry /= 58;
			}
		}

		std::vector< unsigned char >::iterator it = b58.begin();
		while (it != b58.end() && *it == 0)
			it++;
		strOut = "";
		strOut.reserve(zeros + (b58.end() - it));
		strOut.assign(zeros, '1');
		while (it != b58.end()) {
			strOut += kBase58Dictionary[*(it++)];
		}
		return strOut;
	}

	int Base58::Decode(const std::string &strIn, std::string &strout) {
		std::size_t nZeros = 0;
		for (; nZeros < strIn.size() && strIn.at(nZeros) == kBase58Dictionary[0]; nZeros++);
		std::size_t left_size = strIn.size() - nZeros;
		std::size_t new_size = std::size_t(left_size * log2(58.0) / 8 + 2);
		std::string tmp_str(new_size, 0);
		int carry = 0;
		for (size_t i = nZeros; i < strIn.size(); i++) {
			carry = (unsigned char)kBase58digits[strIn[i]];
			for (int j = new_size - 1; j >= 0; j--) {
				int tmp = (unsigned char)tmp_str[j] * 58 + carry;
				tmp_str[j] = (unsigned char)(tmp % 256);
				carry = tmp / 256;
			}
		}
		strout.clear();
		for (size_t i = 0; i < nZeros; i++)
			strout.push_back((unsigned char)0);
		size_t k = 0;
		for (; k < tmp_str.size() && tmp_str[k] == 0; k++);
		for (; k < tmp_str.size(); k++)
			strout.push_back(tmp_str[k]);
		return nZeros + tmp_str.size() - k;
	}

	int Base58::Decode_old(const std::string &strIn, std::string &strOut) {
		strOut.clear();
		BIGNUM bn, bnchar;
		BIGNUM bn58, bn0;
		BN_CTX *ctx = BN_CTX_new();
		if (NULL == ctx)
			return 0;

		std::size_t nZeros = 0;
		for (nZeros = 0; nZeros < strIn.size(); nZeros++) {
			//58 base ，0
			if (strIn.at(nZeros) != kBase58Dictionary[0]) {
				break;
			}
		}
		std::string str_trim = strIn.substr(nZeros, strIn.size() - nZeros);


		BN_init(&bn58);
		BN_init(&bn0);
		BN_init(&bn);
		BN_init(&bnchar);

		BN_set_word(&bn58, 58);
		BN_zero(&bn0);
		for (std::size_t i = 0; i < str_trim.size(); i++) {
			//c = *p;
			unsigned char c = str_trim.at(i);
			if (c & 0x80) {
				if (NULL != ctx)
					BN_CTX_free(ctx);
				return 0;
			}

			if (-1 == kBase58digits[c]) {
				if (NULL != ctx)
					BN_CTX_free(ctx);
				return 0;
			}


			BN_set_word(&bnchar, kBase58digits[c]);

			if (!BN_mul(&bn, &bn, &bn58, ctx)) {
				if (NULL != ctx)
					BN_CTX_free(ctx);
				return 0;
			}

			BN_add(&bn, &bn, &bnchar);
		}
		int cb = BN_num_bytes(&bn);
		unsigned char * out = new unsigned char[cb];
		BN_bn2bin(&bn, out);
		BN_CTX_free(ctx);
		for (std::size_t i = 0; i < nZeros; i++) {
			strOut.push_back((char)0);
		}
		for (int i = 0; i < cb; i++)
			strOut.push_back(out[i]);
		delete[]out;

		BN_clear_free(&bn58);
		BN_clear_free(&bn0);
		BN_clear_free(&bn);
		BN_clear_free(&bnchar);
		return cb + nZeros;
		//label_errexit:
		//	if (NULL != ctx) 
		//		BN_CTX_free(ctx);
		//	return 0;
	}

	//////////////////////////////////MD5//////////////////////////////////////

#ifndef HAVE_OPENSSL

#define F(x, y, z)   ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z)   ((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z)   ((x) ^ (y) ^ (z))
#define I(x, y, z)   ((y) ^ ((x) | ~(z)))
#define STEP(f, a, b, c, d, x, t, s) \
		(a) += f((b), (c), (d)) + (x) + (t); \
		(a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
		(a) += (b);

#if defined(__i386__) || defined(__x86_64__) || defined(__vax__)
#define SET(n) \
			(*(MD5_u32 *)&ptr[(n) * 4])
#define GET(n) \
			SET(n)
#else
#define SET(n) \
			(ctx->block[(n)] = \
			(MD5_u32)ptr[(n) * 4] | \
			((MD5_u32)ptr[(n) * 4 + 1] << 8) | \
			((MD5_u32)ptr[(n) * 4 + 2] << 16) | \
			((MD5_u32)ptr[(n) * 4 + 3] << 24))
#define GET(n) \
			(ctx->block[(n)])
#endif

	typedef unsigned int MD5_u32;

	typedef struct {
		MD5_u32 lo, hi;
		MD5_u32 a, b, c, d;
		unsigned char buffer[64];
		MD5_u32 block[16];
	} MD5_CTX;

	static void MD5_Init(MD5_CTX *ctx);
	static void MD5_Update(MD5_CTX *ctx, const void *data, unsigned long size);
	static void MD5_Final(unsigned char *result, MD5_CTX *ctx);

	static const void *body(MD5_CTX *ctx, const void *data, unsigned long size) {
		const unsigned char *ptr;
		MD5_u32 a, b, c, d;
		MD5_u32 saved_a, saved_b, saved_c, saved_d;

		ptr = (const unsigned char*)data;

		a = ctx->a;
		b = ctx->b;
		c = ctx->c;
		d = ctx->d;

		do {
			saved_a = a;
			saved_b = b;
			saved_c = c;
			saved_d = d;

			STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7)
				STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12)
				STEP(F, c, d, a, b, SET(2), 0x242070db, 17)
				STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22)
				STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7)
				STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12)
				STEP(F, c, d, a, b, SET(6), 0xa8304613, 17)
				STEP(F, b, c, d, a, SET(7), 0xfd469501, 22)
				STEP(F, a, b, c, d, SET(8), 0x698098d8, 7)
				STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12)
				STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17)
				STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22)
				STEP(F, a, b, c, d, SET(12), 0x6b901122, 7)
				STEP(F, d, a, b, c, SET(13), 0xfd987193, 12)
				STEP(F, c, d, a, b, SET(14), 0xa679438e, 17)
				STEP(F, b, c, d, a, SET(15), 0x49b40821, 22)
				STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5)
				STEP(G, d, a, b, c, GET(6), 0xc040b340, 9)
				STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14)
				STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20)
				STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5)
				STEP(G, d, a, b, c, GET(10), 0x02441453, 9)
				STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14)
				STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20)
				STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5)
				STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9)
				STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14)
				STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20)
				STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5)
				STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9)
				STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14)
				STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20)
				STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4)
				STEP(H, d, a, b, c, GET(8), 0x8771f681, 11)
				STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16)
				STEP(H, b, c, d, a, GET(14), 0xfde5380c, 23)
				STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4)
				STEP(H, d, a, b, c, GET(4), 0x4bdecfa9, 11)
				STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16)
				STEP(H, b, c, d, a, GET(10), 0xbebfbc70, 23)
				STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4)
				STEP(H, d, a, b, c, GET(0), 0xeaa127fa, 11)
				STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16)
				STEP(H, b, c, d, a, GET(6), 0x04881d05, 23)
				STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4)
				STEP(H, d, a, b, c, GET(12), 0xe6db99e5, 11)
				STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16)
				STEP(H, b, c, d, a, GET(2), 0xc4ac5665, 23)
				STEP(I, a, b, c, d, GET(0), 0xf4292244, 6)
				STEP(I, d, a, b, c, GET(7), 0x432aff97, 10)
				STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15)
				STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21)
				STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6)
				STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10)
				STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15)
				STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21)
				STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6)
				STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10)
				STEP(I, c, d, a, b, GET(6), 0xa3014314, 15)
				STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21)
				STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6)
				STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10)
				STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15)
				STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21)

				a += saved_a;
			b += saved_b;
			c += saved_c;
			d += saved_d;

			ptr += 64;
		} while (size -= 64);

		ctx->a = a;
		ctx->b = b;
		ctx->c = c;
		ctx->d = d;

		return ptr;
	}

	void MD5_Init(MD5_CTX *ctx) {
		ctx->a = 0x67452301;
		ctx->b = 0xefcdab89;
		ctx->c = 0x98badcfe;
		ctx->d = 0x10325476;

		ctx->lo = 0;
		ctx->hi = 0;
	}

	void MD5_Update(MD5_CTX *ctx, const void *data, unsigned long size) {
		MD5_u32 saved_lo;
		unsigned long used, free;

		saved_lo = ctx->lo;
		if ((ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo)
			ctx->hi++;
		ctx->hi += size >> 29;
		used = saved_lo & 0x3f;

		if (used) {
			free = 64 - used;
			if (size < free) {
				memcpy(&ctx->buffer[used], data, size);
				return;
			}

			memcpy(&ctx->buffer[used], data, free);
			data = (unsigned char *)data + free;
			size -= free;
			body(ctx, ctx->buffer, 64);
		}

		if (size >= 64) {
			data = body(ctx, data, size & ~(unsigned long)0x3f);
			size &= 0x3f;
		}

		memcpy(ctx->buffer, data, size);
	}

	void MD5_Final(unsigned char *result, MD5_CTX *ctx) {
		unsigned long used, free;
		used = ctx->lo & 0x3f;
		ctx->buffer[used++] = 0x80;
		free = 64 - used;

		if (free < 8) {
			memset(&ctx->buffer[used], 0, free);
			body(ctx, ctx->buffer, 64);
			used = 0;
			free = 64;
		}

		memset(&ctx->buffer[used], 0, free - 8);

		ctx->lo <<= 3;
		ctx->buffer[56] = ctx->lo;
		ctx->buffer[57] = ctx->lo >> 8;
		ctx->buffer[58] = ctx->lo >> 16;
		ctx->buffer[59] = ctx->lo >> 24;
		ctx->buffer[60] = ctx->hi;
		ctx->buffer[61] = ctx->hi >> 8;
		ctx->buffer[62] = ctx->hi >> 16;
		ctx->buffer[63] = ctx->hi >> 24;
		body(ctx, ctx->buffer, 64);
		result[0] = ctx->a;
		result[1] = ctx->a >> 8;
		result[2] = ctx->a >> 16;
		result[3] = ctx->a >> 24;
		result[4] = ctx->b;
		result[5] = ctx->b >> 8;
		result[6] = ctx->b >> 16;
		result[7] = ctx->b >> 24;
		result[8] = ctx->c;
		result[9] = ctx->c >> 8;
		result[10] = ctx->c >> 16;
		result[11] = ctx->c >> 24;
		result[12] = ctx->d;
		result[13] = ctx->d >> 8;
		result[14] = ctx->d >> 16;
		result[15] = ctx->d >> 24;
		memset(ctx, 0, sizeof(*ctx));
	}
#else
#include <openssl/md5.h>
#endif

	void MD5::md5bin(const void* dat, size_t len, unsigned char out[16]) {
		MD5_CTX c;
		MD5_Init(&c);
		MD5_Update(&c, dat, len);
		MD5_Final(out, &c);
	}

	char MD5::hb2hex(unsigned char hb) {
		hb = hb & 0xF;
		return hb < 10 ? '0' + hb : hb - 10 + 'a';
	}

	std::string MD5::GenerateMd5File(const char* filename) {
		std::FILE* file = std::fopen(filename, "rb");
		std::string res = "";
		if (NULL == file) {
			res = "";
		}
		else {
			res = GenerateMd5File(file);
			std::fclose(file);
		}
		return res;
	}

	std::string MD5::GenerateMd5File(std::FILE* file) {
		MD5_CTX c;
		MD5_Init(&c);

		char buff[BUFSIZ];
		unsigned char out[16];
		size_t len = 0;
		while ((len = std::fread(buff, sizeof(char), BUFSIZ, file)) > 0) {
			MD5_Update(&c, buff, len);
		}
		MD5_Final(out, &c);

		std::string res = "";
		for (size_t i = 0; i < 16; ++i) {
			res.push_back(hb2hex(out[i] >> 4));
			res.push_back(hb2hex(out[i]));
		}
		return res;
	}

	std::string MD5::GenerateMD5(const void* dat, size_t len) {
		unsigned char out[16];
		md5bin(dat, len, out);
		std::string res = "";
		for (size_t i = 0; i < 16; ++i) {
			res.push_back(hb2hex(out[i] >> 4));
			res.push_back(hb2hex(out[i]));
		}
		return res;
	}

	std::string MD5::GenerateMD5(std::string dat) {
		return GenerateMD5(dat.c_str(), dat.length());
	}

	std::string MD5::GenerateMD5Sum6(const void* dat, size_t len) {
		static const char* tbl = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
		unsigned char out[16];
		md5bin(dat, len, out);
		std::string res = "";
		for (size_t i = 0; i < 6; ++i) {
			res.push_back(tbl[out[i] % 62]);
		}
		return res;
	}

	std::string MD5::GenerateMD5Sum6(std::string dat) {
		return GenerateMD5Sum6(dat.c_str(), dat.length());
	}

	std::string Aes::Decrypto(const std::string &input, const std::string &key) {
		if (key.size() != 16 &&
			key.size() != 24 &&
			key.size() != 32
			) {
			return "";
		}

		/* Input data to encrypt */
		unsigned char aes_input[] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5 };

		/* Init vector */
		unsigned char iv[AES_BLOCK_SIZE];
		memset(iv, 0x00, AES_BLOCK_SIZE);

		/* Buffers for Encryption and Decryption */
		std::string enc_out;
		enc_out.resize(input.size());

		AES_KEY dec_key;
		/* AES-128 bit CBC Decryption */
		memset(iv, 0x00, AES_BLOCK_SIZE); // Don't forget to set iv vector again, or you can't decrypt data properly
		AES_set_decrypt_key((const unsigned char *)key.c_str(), key.size() * 8, &dec_key); // Size of key is in bits
		AES_cbc_encrypt((const unsigned char *)input.c_str(), (unsigned char *)enc_out.c_str(), enc_out.size(), &dec_key, iv, AES_DECRYPT);
		enc_out.resize(strlen(enc_out.c_str()));
		return enc_out;
	}

	std::string Aes::Crypto(const std::string &input, const std::string &key) {
		if (key.size() != 16 &&
			key.size() != 24 &&
			key.size() != 32
			) {
			return "key error";
		}

		// Set the encryption length
		size_t len = 0;
		if ((input.size() + 1) % AES_BLOCK_SIZE == 0) {
			len = input.size() + 1;
		} else {
			len = ((input.size() + 1) / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;
		}

		// Set the input string
		unsigned char* input_string = (unsigned char*)calloc(len, sizeof(unsigned char));
		if (input_string == NULL) {
			fprintf(stderr, "Unable to allocate memory for input_string\n");
			return "";
		}
		strncpy((char*)input_string, input.c_str(), input.size());

		/* Init vector */
		unsigned char iv[AES_BLOCK_SIZE];
		memset(iv, 0x00, AES_BLOCK_SIZE);

		/* Buffers for Encryption and Decryption */
		std::string enc_out;
		enc_out.resize(len);

		/* AES-128 bit CBC Encryption */
		AES_KEY enc_key;
		AES_set_encrypt_key((const unsigned char *)key.c_str(), key.size() * 8, &enc_key);
		AES_cbc_encrypt((const unsigned char *)input.c_str(), (unsigned char *)enc_out.c_str(), input.size(), &enc_key, iv, AES_ENCRYPT);

		delete input_string;
		return enc_out;
	}

	std::string Aes::CryptoHex(const std::string &input, const std::string &key) {
		return utils::String::BinToHexString(Crypto(input, key));
	}

	std::string Aes::HexDecrypto(const std::string &input, const std::string &key) {
		return Decrypto(utils::String::HexStringToBin(input), key);
	}

	int AesCtr::InitCtr(struct ctr_state *state, const unsigned char iv[16]) {
		state->num = 0;
		memset(state->ecount, 0, AES_BLOCK_SIZE);
		memcpy(state->ivec, iv, 16);

		return 0;
	}
	// encrypt twice  == decrypt

	void AesCtr::Encrypt(unsigned char *indata, unsigned char *outdata, int bytes_read) {

		int i = 0;
		int mod_len = 0;

		AES_set_encrypt_key((const unsigned char *)ckey_.c_str(), ckey_.size() * 8, &key);

		if (bytes_read < BYTES_SIZE) {
			struct ctr_state state;
			InitCtr(&state, iv_);
			AES_ctr128_encrypt(indata, outdata, bytes_read, &key, state.ivec, state.ecount, &state.num);
			return;
		}
		// loop block size  = [ BYTES_SIZE ]
		for (i = BYTES_SIZE; i <= bytes_read; i += BYTES_SIZE) {
			struct ctr_state state;
			InitCtr(&state, iv_);
			AES_ctr128_encrypt(indata, outdata, BYTES_SIZE, &key, state.ivec, state.ecount, &state.num);
			indata += BYTES_SIZE;
			outdata += BYTES_SIZE;
		}

		mod_len = bytes_read % BYTES_SIZE;
		if (mod_len != 0) {
			struct ctr_state state;
			InitCtr(&state, iv_);
			AES_ctr128_encrypt(indata, outdata, mod_len, &key, state.ivec, state.ecount, &state.num);
		}
	}

	void AesCtr::Encrypt(const std::string &instr, std::string &outstr) {

		int i = 0;
		int mod_len = 0;
		unsigned char *indata;
		unsigned char *outdata;

		indata = (unsigned char *)instr.c_str();
		outstr.resize(instr.size());
		outdata = (unsigned char *)outstr.c_str();

		AES_set_encrypt_key((const unsigned char *)ckey_.c_str(), ckey_.size() * 8, &key);

		if (instr.size() < BYTES_SIZE) {
			struct ctr_state state;
			InitCtr(&state, iv_);
			AES_ctr128_encrypt(indata, outdata, instr.size(), &key, state.ivec, state.ecount, &state.num);
			return;
		}
		// loop block size  = [ BYTES_SIZE ]
		for (i = BYTES_SIZE; i <= instr.size(); i += BYTES_SIZE) {
			struct ctr_state state;
			InitCtr(&state, iv_);
			AES_ctr128_encrypt(indata, outdata, BYTES_SIZE, &key, state.ivec, state.ecount, &state.num);
			indata += BYTES_SIZE;
			outdata += BYTES_SIZE;
		}

		mod_len = instr.size() % BYTES_SIZE;
		if (mod_len != 0) {
			struct ctr_state state;
			InitCtr(&state, iv_);
			AES_ctr128_encrypt(indata, outdata, mod_len, &key, state.ivec, state.ecount, &state.num);
		}
	}

	bool AesCtr::IsValid() {
		return key_valid_;
	}

// 	AesCtr::AesCtr() {
// 		unsigned char temp_iv[8] = { 0x66, 0x61, 0x63, 0x65, 0x73, 0x65, 0x61, 0x00 };
// 		memcpy(iv_, temp_iv, 8);
// 
// 		unsigned char temp_ckey[32] = {
// 			0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
// 			0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12 }; // 32bytes = AES256, 16bytes = AES128
// 		memcpy(ckey_, temp_ckey, 32);
// 	}

	AesCtr::AesCtr(unsigned char* iv, const std::string &ckey) {
		memcpy(iv_, iv, 16);
		ckey_ = ckey;

		key_valid_ = ckey.size() == 16 ||
			ckey.size() == 24 ||
			ckey.size() == 32;
	}

	AesCtr::~AesCtr() {}
}
