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

#ifndef SM3_H
#define SM3_H

#include "crypto.h"

namespace utils {

	class Sm3 : public Hash {
		typedef struct {
			unsigned long total[2];     /*!< number of bytes processed 8 */
			unsigned long state[8];     /*!< intermediate digest state  */
			unsigned char buffer[64];   /*!< data block being processed */

			unsigned char ipad[64];     /*!< HMAC: inner padding        */
			unsigned char opad[64];     /*!< HMAC: outer padding        */

		}
		sm3_context;
		sm3_context ctx_;
	public:
		Sm3();
		~Sm3();

		void Update(const std::string &input);
		void Update(const void *buffer, size_t len);
		std::string Final();

		static std::string CryptoBase58(const std::string &input) {
			return utils::Base58::Encode(Crypto(input));
		}

		static std::string Crypto(const std::string &input);
		static void Crypto(unsigned char* str, int len, unsigned char *buf);
		static void Crypto(const std::string &input, std::string &str);

	private:

		static void sm3_starts(sm3_context *ctx);
		/**
		* \brief          SM3 process buffer
		*
		* \param ctx      SM3 context
		* \param input    buffer holding the  data
		* \param ilen     length of the input data
		*/
		static void sm3_update(sm3_context *ctx, unsigned char *input, int ilen);
		/**
		* \brief          SM3 final digest
		*
		* \param ctx      SM3 context
		*/
		static void sm3_finish(sm3_context *ctx, unsigned char output[32]);

		/**
		* \brief          Output = SM3( input buffer )
		*
		* \param input    buffer holding the  data
		* \param ilen     length of the input data
		* \param output   SM3 checksum result
		*/
		static void sm3(unsigned char *input, int ilen,
			unsigned char output[32]);

		static void sm3_process(sm3_context *ctx, unsigned char data[64]);

		/**
		* \brief          Output = SM3( file contents )
		*
		* \param path     input file name
		* \param output   SM3 checksum result
		*
		* \return         0 if successful, 1 if fopen failed,
		*                 or 2 if fread failed
		*/
		static int sm3_file(char *path, unsigned char output[32]);

		/**
		* \brief          SM3 HMAC context setup
		*
		* \param ctx      HMAC context to be initialized
		* \param key      HMAC secret key
		* \param keylen   length of the HMAC key
		*/
		static void sm3_hmac_starts(sm3_context *ctx, unsigned char *key, int keylen);
		/**
		* \brief          SM3 HMAC process buffer
		*
		* \param ctx      HMAC context
		* \param input    buffer holding the  data
		* \param ilen     length of the input data
		*/
		static void sm3_hmac_update(sm3_context *ctx, unsigned char *input, int ilen);

		/**
		* \brief          SM3 HMAC final digest
		*
		* \param ctx      HMAC context
		* \param output   SM3 HMAC checksum result
		*/
		static void sm3_hmac_finish(sm3_context *ctx, unsigned char output[32]);

		/**
		* \brief          Output = HMAC-SM3( hmac key, input buffer )
		*
		* \param key      HMAC secret key
		* \param keylen   length of the HMAC key
		* \param input    buffer holding the  data
		* \param ilen     length of the input data
		* \param output   HMAC-SM3 result
		*/
		static void sm3_hmac(unsigned char *key, int keylen,
			unsigned char *input, int ilen,
			unsigned char output[32]);
	};
}

#endif
