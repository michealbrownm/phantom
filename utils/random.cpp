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
#include <cstring>
#include <random>
#include <openssl/rand.h>
#if defined(_MSC_VER)
#include <Windows.h>
#endif

#include "random.h"
#include "crypto.h"

#define NUM_OS_RANDOM_BYTES 32


namespace utils {
	int64_t GetPerformanceCounter()
	{
		return std::chrono::high_resolution_clock::now().time_since_epoch().count();
	}


	void MemoryClean(void *ptr, size_t len)
	{
		std::memset(ptr, 0, len);

#if defined(_MSC_VER)
		SecureZeroMemory(ptr, len);
#else
		__asm__ __volatile__("" : : "r"(ptr) : "memory");
#endif
	}

	void RandAddSeed()
	{
		// Seed with CPU performance counter
		int64_t nCounter = GetPerformanceCounter();
		RAND_add(&nCounter, sizeof(nCounter), 1.5);
		MemoryClean((void*)&nCounter, sizeof(nCounter));
	}

	bool GetRandBytes(unsigned char* buf, int num)
	{
		if (RAND_bytes(buf, num) != 1) {
			return false;
		}
		return true;
	}

	bool GetOSRand(unsigned char *buf, int num)
	{
#if defined(WIN32)
		HCRYPTPROV hProvider;
		int ret = CryptAcquireContextW(&hProvider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
		if (!ret) {
			return false;
		}
		ret = CryptGenRandom(hProvider, num, buf);
		if (!ret) {
			return false;
		}
		CryptReleaseContext(hProvider, 0);
#else
		std::random_device rd;
		for (int i =0;i<num;i++){
			buf[i] = (uint8_t)std::uniform_int_distribution<uint16_t>(0, 255)(rd);
		}

#endif
		return true;
	}

	bool GetStrongRandBytes(std::string & out){

		unsigned char buf[64] = { 0 };
		RandAddSeed();
		if (!GetRandBytes(buf, 32))
			return false;

		if (!GetOSRand(buf + 32,32))
			return false;

		std::string input((char*)buf, 64);
		Sha256::Crypto(input, out);
		return true;
	}
}