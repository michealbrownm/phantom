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

#ifndef UTILS_STRING_UTIL_H_
#define UTILS_STRING_UTIL_H_

#include <string.h>
#include "common.h"
#include "stdio.h"

#define IS_BLANK(c) ((c) == ' ' || (c) == '\t')  
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')  
#define IS_ALPHA(c) ( ((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') )  
#define IS_HEX_DIGIT(c) (((c) >= 'A' && (c) <= 'F') || ((c) >= 'a' && (c) <= 'f'))  

namespace utils {

	typedef std::vector<std::string> StringVector;
	typedef std::list<std::string> StringList;
	typedef std::map<std::string, std::string> StringMap;

	class String {
	public:
		static const int kMaxStringLen = 1024 * 1024;

		static int IsNumber(const std::string& str) {
			int base = 10;
			const char * ptr;
			int type = 0;

			if (!str.length()) return 0;

			ptr = str.c_str();

			/* Skip blank */
			while (IS_BLANK(*ptr)) {
				ptr++;
			}

			/* Skip sign */
			if (*ptr == '-' || *ptr == '+') {
				ptr++;
			}

			/* First char should be digit or dot*/
			if (IS_DIGIT(*ptr) || ptr[0] == '.') {

				if (ptr[0] != '.') {
					/* Handle hex numbers */
					if (ptr[0] == '0' && ptr[1] && (ptr[1] == 'x' || ptr[1] == 'X')) {
						type = 2;
						base = 16;
						ptr += 2;
					}

					/* Skip any leading 0s */
					while (*ptr == '0') {
						ptr++;
					}

					/* Skip digit */
					while (IS_DIGIT(*ptr) || (base == 16 && IS_HEX_DIGIT(*ptr))) {
						ptr++;
					}
				}

				/* Handle dot */
				if (base == 10 && *ptr && ptr[0] == '.') {
					type = 3;
					ptr++;
				}

				/* Skip digit */
				while (type == 3 && base == 10 && IS_DIGIT(*ptr)) {
					ptr++;
				}

				/* If it ends with 0, it is a number */
				if (*ptr == 0)
					return (type > 0) ? type : 1;
				else
					type = 0;
			}
			return type;
		}

		/// @brief Convert to int type
		static int Stoi(const std::string &str) {
			if (0 == str.length()) {
				return 0;
			}
			return atoi(str.c_str());
		}
		
		/// @brief Convert to unsigned int
		static unsigned int Stoui(const std::string &str) {
			if (0 == str.length()) {
				return 0;
			}
			size_t used_digits = 0;
			uint32_t value = 0;
			for (int i = 0; i < (int)str.size(); i++) {
				char item_value = str[i];
				if (item_value >= '0' && item_value <= '9') {
					value *= 10;
					value += (uint32_t)(item_value - '0');
					used_digits++;
				}
				else if (!IsSpace(item_value) || used_digits > 0) {
					break;
				}
			}

			return value;
		}

		/// @brief Convert to int64 type
		static int64_t Stoi64(const std::string &str) {
			if (0 == str.length()) {
				return 0;
			}
			int64_t v = 0;
#ifdef WIN32
			v = _atoi64(str.c_str());
#else
			v = atoll(str.c_str());
#endif
			return v;
		}

		/// @brief Convert to uint64 type
		static int64_t Stoui64(const std::string &str) {
			if (0 == str.length()) {
				return 0L;
			}

			size_t used_digits = 0;
			uint64_t value = 0;
			for (int i = 0; i < (int)str.size(); i++) {
				char item_value = str[i];
				if (item_value >= '0' && item_value <= '9') {
					value *= 10;
					value += (uint64_t)(item_value - '0');
					used_digits++;
				}
				else if (!IsSpace(item_value) || used_digits > 0) {
					break;
				}
			}

			return value;
		}

		/// @brief Convert to long
		static long Stol(const std::string &str) {
			if (0 == str.length()) {
				return 0L;
			} 
			return atol(str.c_str());
		}

		/// @brief Convert to float
		static float Stof(const std::string &str) {
			if (0 == str.length()) {
				return 0.0F;
			}
			return static_cast<float>(atof(str.c_str()));
		}

		/// @brief Convert to double
		static double Stod(const std::string &str) {
			if (0 == str.length()) {
				return 0.0;
			}
			return atof(str.c_str());
		}

		/// @brief Convert to bool
		static bool Stob(const std::string &str) {
			return (0 == str.length() || str == "0" || str == "false" || str == "FALSE") ? false : true;
		}

		/// @brief Convert int type into String
		static std::string ToString(const int val) {
			char buf[32] = { 0 };
#ifdef WIN32
			_snprintf(buf, sizeof(buf), "%d", val);
#else
			snprintf(buf, sizeof(buf), "%d", val);
#endif
			return buf;
		}

		/// @brief Convert unsigned int into String
		static std::string ToString(const unsigned int val) {
			char buf[32] = { 0 };
#ifdef WIN32
			_snprintf(buf, sizeof(buf), "%u", val);
#else
			snprintf(buf, sizeof(buf), "%u", val);
#endif
			return buf;
		}

		/// @brief Convert long into String
		//    static std::string ToString(const long val)
		//    {
		//        char buf[32] = {0};
		//#ifdef WIN32
		//		_snprintf(buf, sizeof(buf), "%ld", val);
		//#else
		//                snprintf(buf, sizeof(buf), "%ld", val);
		//#endif
		//		return buf;
		//    }

		/// @brief Convert long long into String
		static std::string ToString(const int64_t val) {
			char buf[32] = { 0 };
#ifdef WIN32    
			_snprintf(buf, sizeof(buf), FMT_I64, val);
#else
			snprintf(buf, sizeof(buf), FMT_I64, val);
#endif
			return buf;
		}

		static std::string ToString(const uint64_t val) {
			char buf[32] = { 0 };
#ifdef WIN32    
			_snprintf(buf, sizeof(buf), FMT_U64, val);
#else
			snprintf(buf, sizeof(buf), FMT_U64, val);
#endif
			return buf;
		}

		/// @brief Convert double into String
		static std::string ToString(const double val) {
			char buf[32] = { 0 };
#ifdef WIN32    
			_snprintf(buf, sizeof(buf), "%f", val);
#else
			snprintf(buf, sizeof(buf), "%f", val);
#endif
			return buf;
		}

		/// @brief Convert bool into String
		static std::string ToString(const bool val) {
			return val ? "1" : "0";
		}

		static bool SafeStoi(const std::string &str, int& num){
			if (str.size() > 11)
				return false;

			int temNum = Stoi(str);
			std::string temStr = ToString(temNum);
			if (0 == str.compare(temStr)){
				num = temNum;
				return true;
			}
			else{
				return false;
			}
		}

		static bool SafeStoui(const std::string &str, unsigned int& num){
			if (str.size() > 10)
				return false;

			unsigned int temNum = Stoui(str);
			std::string temStr = ToString(temNum);
			if (0 == str.compare(temStr)){
				num = temNum;
				return true;
			}
			else{
				return false;
			}
		}

		static bool SafeStoi64(const std::string &str, int64_t& num){
			if (str.size() > 20)
				return false;

			int64_t temNum = Stoi64(str);
			std::string temStr = ToString(temNum);
			if (0 == str.compare(temStr)){
				num = temNum;
				return true;
			}
			else{
				return false;
			}
		}

		static bool SafeStoui64(const std::string &str, uint64_t& num){
			if (str.size() > 20)
				return false;

			uint64_t temNum = Stoui64(str);
			std::string temStr = ToString(temNum);
			if (0 == str.compare(temStr)){
				num = temNum;
				return true;
			}
			else{
				return false;
			}
		}

		/// @brief Format String
		static std::string &Format(std::string &str, const char *format, ...) {
			va_list ap;

			int nTimes = 1;
			while (nTimes < 256) {
				int nMalloc = kMaxStringLen* nTimes;
				char *buf = (char *)malloc(nMalloc);
				if (buf) {
					va_start(ap, format);
					int nCopy = vsnprintf(buf, nMalloc, format, ap);
					va_end(ap);
					if ((-1 < nCopy) && (nCopy < nMalloc)) {
						str = buf;

						free(buf);
						break;
					}
					free(buf);
					nTimes = nTimes * 2;
				}
				else abort();
			}
			return str;
		}

		static std::vector< std::string >  split(const std::string& s, const std::string& delim) {
			size_t last = 0;
			size_t index = s.find_first_of(delim, last);
			std::vector< std::string > ret;
			while (index != std::string::npos) {
				ret.push_back(s.substr(last, index - last));
				last = index + 1;
				index = s.find_first_of(delim, last);
			}
			if (index - last > 0) {
				ret.push_back(s.substr(last, index - last));
			}
			return ret;
		}

		//Such as string a=1;b=2;c=3 delim is ; ope is = 
		static std::map<std::string, std::string> ParseAttribute(const std::string &s, const std::string &delim, const std::string &ope) {
			std::map<std::string, std::string> ret;
			std::vector< std::string > split_string = split(s, delim);
			for (size_t i = 0; i < split_string.size(); i++) {
				std::vector< std::string > item = split(split_string[i], ope);
				if (item.size() > 1) {
					ret.insert(std::make_pair(item[0], item[1]));
				}
				else {
					ret.insert(std::make_pair(item[0], ""));
				}
			}
			return ret;
		}

		/// @brief Format String
		static std::string Format(const char *format, ...) {
			va_list ap;

			std::string str;
			int32_t nTimes = 1;
			while (nTimes < 256) {
				int nMalloc = kMaxStringLen*nTimes;
				char *buf = (char *)malloc(nMalloc);
				if (buf) {
					va_start(ap, format);
					int nCopy = vsnprintf(buf, nMalloc, format, ap);
					va_end(ap);
					if ((-1 < nCopy) && (nCopy < nMalloc)) {
						str = buf;
						free(buf);
						break;
					}
					nTimes = nTimes * 2;
					free(buf);
				}
				else abort();
			}

			return str;
		}

		static std::string AppendFormat(const std::string &org, const char *format, ...) {
			va_list ap;

			std::string str = org;
			int nTimes = 1;
			while (nTimes < 256) {

				int nMalloc = kMaxStringLen*nTimes;

				char *buf = (char *)malloc(nMalloc);
				if (!buf)
					abort();

				va_start(ap, format);
				int nCopy = vsnprintf(buf, nMalloc, format, ap);
				va_end(ap);
				if (-1 < nCopy && nCopy < nMalloc) {
					str += buf;
					free(buf);
					break;
				}
				free(buf);
				nTimes = nTimes * 2;
			}

			return str;
		}

		static bool IsSpace(const char nValue) {
			//
			// Isspace will cause assert failure if value < 0
			//
			return (nValue > 0) && isspace(nValue);
		}

		/// @brief Remove spaces, line breaks, and tabs on the left
		static std::string TrimLeft(std::string &str) {
			if (str.size() == 0) return str;

			size_t nLeftPos = 0, nRightPos = str.size() - 1;
			while (nLeftPos < str.size() && IsSpace(str[nLeftPos])) nLeftPos++;

			if (nLeftPos > nRightPos) {
				str.clear();
			}
			else {
				std::string strTrim = str.substr(nLeftPos, nRightPos - nLeftPos + 1);
				str = strTrim;
			}

			return str;
		}

		/// @brief Remove spaces, line breaks, and tabs to the right
		static std::string TrimRight(std::string str) {
			if (str.size() == 0) return str;

			size_t nLeftPos = 0, nRightPos = str.size() - 1;
			while (nRightPos >= nLeftPos && IsSpace(str[nRightPos])) {
				if (nRightPos == 0) break;
				nRightPos--;
			}

			if (nLeftPos > nRightPos) {
				str.clear();
			}
			else {
				std::string strTrim = str.substr(nLeftPos, nRightPos - nLeftPos + 1);
				str = strTrim;
			}

			return str;
		}

		/// @brief Remove spaces, line breaks, and tabs on the left and right sides
		static std::string Trim(std::string &str) {
			if (str.size() == 0) return str;

			size_t nLeftPos = 0, nRightPos = str.size() - 1;
			while (nLeftPos < str.size() && IsSpace(str[nLeftPos])) nLeftPos++;
			while (nRightPos >= nLeftPos && IsSpace(str[nRightPos])) {
				if (nRightPos == 0) break;
				nRightPos--;
			}

			if (nLeftPos > nRightPos) {
				str.clear();
			}
			else {
				std::string strTrim = str.substr(nLeftPos, nRightPos - nLeftPos + 1);
				str = strTrim;
			}

			return str;
		}

		/// @brief Determine if the string is a displayable character
		static bool CanDisplay(const std::string &str) {
			for (size_t i = 0; i < str.length(); i++) {
				if (!(str[i] >= 0x20 && str[i] <= 127)) {
					return false;
				}
			}
			return true;
		}

		/// @brief Whether it contains uppercase letters
		static bool IsContainUppercase(const std::string &str) {
			for (size_t i = 0; i < str.length(); i++) {
				if (str[i] >= 'A' && str[i] <= 'Z') {
					return true;
				}
			}
			return false;
		}

		/// @brief Whether it contains lowercase letters
		static bool IsContainLowercase(const std::string &str) {
			for (size_t i = 0; i < str.length(); i++) {
				if (str[i] >= 'a' && str[i] <= 'z') {
					return true;
				}
			}
			return false;
		}

		/// @brief Is it an integer data?
		static bool IsInteger(const std::string &str) {
			for (size_t i = 0; i < str.length(); i++) {
				if (str[i] > '9' || str[i] < '0') {
					return false;
				}
			}
			return true;

		}

		/// @brief Convert to lowercase letters
		static std::string ToLower(std::string &str) {
			for (std::string::size_type i = 0; i < str.length(); ++i)
				if (str[i] >= 'A' && str[i] <= 'Z') {
					str[i] += 0x20;
				}
			return str;
		}

		/// @brief Convert to uppercase letters
		static std::string ToUpper(std::string &str) {
			for (std::string::size_type i = 0; i < str.length(); ++i)
				if (str[i] >= 'a' && str[i] <= 'z') {
					str[i] -= 0x20;
				}
			return str;
		}

		/// @brief Whether the character is a letter
		static bool CharIsLetter(char c) {
			if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
				return true;
			}
			return false;
		}

		/// @brief Whether the two strings are equal, ignoring case
		static bool EqualsIgnoreCase(const std::string &s1, const std::string &s2) {
			if (s1.length() != s2.length()) {
				return false;
			}

			for (std::string::size_type i = 0; i < s1.length(); ++i) {
				if (s1[i] == s2[i]) {
					continue;
				}
				if (!CharIsLetter(s1[i]) || !CharIsLetter(s2[i])) {
					return false;
				}
				if (0x20 != abs(s1[i] - s2[i])) {
					return false;
				}
			}
			return true;
		}

		// @brief Whether to include substrings, ignoring case
		static bool IsContainStringIgnoreCase(const std::string &str1, const std::string &str2) {
			std::string s1(str1);
			std::string s2(str2);

			ToUpper(s1);
			ToUpper(s2);

			return std::string::npos != str1.find(str2);
		}

		/// @brief Replace the text in the string
		static std::string Replace(std::string &str, const std::string &from, const std::string &to) {
			std::string::size_type pos = 0;
			while ((pos = str.find(from, pos)) != -1) {
				str.replace(pos, from.length(), to);
				pos += to.length();
			}
			return str;
		}

		static void Swap(std::string &ls, std::string &rs) {
			std::string temp(ls);
			ls = rs;
			rs = temp;
		}

		static int32_t Strtok(const std::string &str, char separator, StringVector &arr) {
			size_t pos = 0;
			size_t newPos = 0;

			while (std::string::npos != pos) {
				pos = str.find_first_of(separator, newPos);
				if (std::string::npos == pos) { // Complete
					if (pos > newPos) {
						arr.push_back(str.substr(newPos, pos - newPos));
					}
					break;
				}
				else {
					if (pos > newPos) {
						arr.push_back(str.substr(newPos, pos - newPos));
					}
					newPos = pos + 1;
				}
			}

			return arr.size();
		}

		static std::string BinToHexString(const std::string &value) {
			std::string result;
			result.resize(value.size() * 2);
			for (size_t i = 0; i < value.size(); i++) {
				uint8_t item = value[i];
				uint8_t high = (item >> 4);
				uint8_t low = (item & 0x0F);
				result[2 * i] = (high >= 0 && high <= 9) ? (high + '0') : (high - 10 + 'a');
				result[2 * i + 1] = (low >= 0 && low <= 9) ? (low + '0') : (low - 10 + 'a');
			}
			return result;
		}

	static std::string HexStringToBin(const std::string &hex_string, bool force_little = false){
		if (hex_string.size() % 2 != 0 || hex_string.empty() ){
			return "";
		}
		std::string result;
		result.resize(hex_string.size()/2);
		for (size_t i = 0; i < hex_string.size() - 1; i = i + 2){
			uint8_t high = 0;
            if (hex_string[i] >= '0' && hex_string[i] <= '9')
                high = (hex_string[i] - '0');
            else if (hex_string[i] >= 'a' && hex_string[i] <= 'f')
                high = (hex_string[i] - 'a' + 10);
			else if (hex_string[i] >= 'A' && hex_string[i] <= 'F'  && !force_little) {
				high = (hex_string[i] - 'A' + 10);
			}
			else {
				return "";
			}

			uint8_t low = 0;
            if (hex_string[i + 1] >= '0' && hex_string[i + 1] <= '9')
                low = (hex_string[i + 1] - '0');
            else if (hex_string[i + 1] >= 'a' && hex_string[i + 1] <= 'f')
                low = (hex_string[i + 1] - 'a' + 10);
			else  if (hex_string[i + 1] >= 'A' && hex_string[i + 1] <= 'F' && !force_little) {
                low = (hex_string[i + 1] - 'A' + 10);
			}
			else {
				return "";
			}

			int valuex = (high << 4) + low;
			//sscanf(hex_string.substr(i, 2).c_str(), "%x", &valuex);
			result.at(i/2) = (char)valuex;
		}

			return result;
		}

		static bool HexStringToBin(const std::string &hex_string, std::string &out_put) {
			if (!IsHexString(hex_string)) {
				return false;
			}
			out_put = HexStringToBin(hex_string);
			return true;
		}

		static bool IsHexString(const std::string &hex_string) {
			if (hex_string.size() % 2 != 0) {
				return false;
			}
			for (size_t i = 0; i < hex_string.size(); i++) {
				char c = hex_string.at(i);
				if (('0' <= c &&c <= '9') || ('a' <= c&& c <= 'f') || ('A' <= c&& c <= 'F')) {
					continue;
				}
				else
					return false;
			}
			return true;
		}

		static std::string Bin4ToHexString(const std::string &value) {
			std::string result;
			for (size_t i = 0; i < value.size() && i < 4; i++) {
				result = AppendFormat(result, "%02x", (uint8_t)value[i]);
			}

			return result;
		}

		static time_t ToTimestamp(const std::string &strTime) {
			std::string strNewTime = strTime;
			utils::String::Replace(strNewTime, ":", "-");
			utils::String::Replace(strNewTime, " ", "-");
			utils::String::Replace(strNewTime, "/", "-");
			utils::String::Replace(strNewTime, ".", "-");

			utils::StringVector nValues;
			if (utils::String::Strtok(strNewTime, '-', nValues) != 6) {
				return 0;
			}

			struct tm nTimeValue = { 0 };

			nTimeValue.tm_year = atoi(nValues[0].c_str()) - 1900;
			nTimeValue.tm_mon = atoi(nValues[1].c_str()) - 1;
			nTimeValue.tm_mday = atoi(nValues[2].c_str());
			nTimeValue.tm_hour = atoi(nValues[3].c_str());
			nTimeValue.tm_min = atoi(nValues[4].c_str());
			nTimeValue.tm_sec = atoi(nValues[5].c_str());

			time_t nLocalTimestamp = mktime(&nTimeValue);

			return nLocalTimestamp;
		}

		/*
		assert(FormatDecimal(123456789, 1) == "12345678.9");
		assert(FormatDecimal(123456789, 2) == "1234567.89");
		*/
		template <typename VALUE_TYPE>
		static std::string FormatDecimal(VALUE_TYPE number, size_t decimal) {
			std::string result = utils::String::ToString(number);
			if (decimal >= result.size()) result.insert(0, decimal - result.size() + 1, '0');

			std::string result_decimal;
			bool zero_not_show = true;
			for (size_t i = 0; i < result.size(); i++) {
				size_t rev_index = result.size() - i - 1;
				if (result[rev_index] != '0') zero_not_show = false;
				if (i < decimal) {
					if (result[rev_index] != '0' || !zero_not_show) result_decimal.push_back(result[rev_index]);
				}
				else if (i == decimal) {
					if (result_decimal.size() > 0) result_decimal.push_back('.');
					result_decimal.push_back(result[rev_index]);
				}
				else {
					result_decimal.push_back(result[rev_index]);
				}
			}
			std::reverse(result_decimal.begin(), result_decimal.end());
			return result_decimal;
		}

		static StringVector Strtok(const std::string &str, char separator) {
			size_t pos = 0;
			size_t newPos = 0;
			StringVector arr;

			while (std::string::npos != pos) {
				pos = str.find_first_of(separator, newPos);
				if (std::string::npos == pos) { // Complete
					if (pos > newPos) {
						arr.push_back(str.substr(newPos, pos - newPos));
					}
					break;
				}
				else {
					if (pos > newPos) {
						arr.push_back(str.substr(newPos, pos - newPos));
					}
					newPos = pos + 1;
				}
			}
			return arr;
		}


		static std::string MultiplyDecimal(std::string value, size_t decimals) {
			size_t dot_pos = value.find(".");
			if (dot_pos == std::string::npos) {
				value.insert(value.end(), decimals, '0');
			}
			else{
				size_t right = value.size() - dot_pos - 1;
				if (right <= decimals) {
					value.erase(value.find("."), 1);
					value.insert(value.end(), decimals - right, '0');
				}
				else {
					value.erase(dot_pos, 1);
					value.insert(dot_pos + decimals, 1, '.');
				}			
			}

			size_t i = 0;
			for (; i < value.size(); i++) {
				if (value[i] != '0') {
					break;
				}
			}

			if (value[i] == '.') {
				i--;
			}

			value.erase(0, i);

			return value;
		}

		static bool IsDecNumber(const std::string &value, size_t decimal) {
			if (value.empty()) {
				return false;
			}

			if (value[value.size() - 1] == '.') {
				return false;
			}

			bool ret = true;
			size_t count_zero = 0;
			bool continue_zero = true;
			size_t dot_pos = std::string::npos;
			for (size_t i = 0; i < value.size(); i++) {
				if (!IS_DIGIT(value[i]) && value[i] != '.') {
					return false;
				}

				if (continue_zero && value[i] == '0') {
					count_zero++;
				}
				else {
					continue_zero = false;
				}

				if (value[i] == '.') {
					if (dot_pos == std::string::npos) {
						dot_pos = i;
					}
					else {
						return false;
					}
				}
			}
			// 0123 or 00123, except 0 or 0.123
			bool leading_zero = (value[0] == '0') ? true : false;
			if ((leading_zero && dot_pos != 1) && value != "0") {
				return false;
			}

			if ((dot_pos != std::string::npos) && (value.size() - dot_pos - 1 > decimal)) {
				return false;
			}

			return ret;
		}

		template <typename VALUE_TYPE>
		static int Strtok(const std::string &content,
			VALUE_TYPE &values,
			const std::string &key,
			int max_items = -1,
			bool ignore_empty = false,
			size_t content_len = std::string::npos) {
			int start_pos = 0;
			int find_pos = 0;
			int item_count = 0;
			int key_len = (int)key.size();
			int end_pos = (std::string::npos == content_len) ? (int)content.size() : (int)content_len;

			values.clear();

			if (key.size() == 0) return 0;
			else if (end_pos <= 0 || end_pos >= (int)content.size()) {
				end_pos = (int)content.size();
			}

			while (start_pos < (int)end_pos) {
				find_pos = (int)content.find(key.c_str(), start_pos);
				if (find_pos < 0 || find_pos >= end_pos || (max_items > 0 && item_count == max_items - 1)) {
					find_pos = end_pos;
				}

				if (find_pos < start_pos) {
					break;
				}

				if (find_pos > start_pos || !ignore_empty) {
					std::string new_value = (find_pos > start_pos) ? content.substr(start_pos, find_pos - start_pos) : std::string("");
					values.push_back(new_value);
					item_count++;
				}

				start_pos = find_pos + key_len;
			}

			return item_count;
		}

		// Parse attribute string, in format such as "a=b;c=d;e=f" => Map{"a"=>"b", "c"=>"d","e"=>"f"}
		static int ParseAttributes(const std::string &content,
			StringMap &attrs,
			const std::string &sep_key,
			const std::string &value_key,
			bool key_low_case,
			bool trim_key,
			bool trim_value) {
			int lines = 0;
			StringList parts;

			Strtok(content, parts, sep_key);

			for (StringList::iterator itr = parts.begin(); itr != parts.end(); itr++) {
				std::string &strPartValue = (*itr);
				StringList part_values;
				if (Strtok(strPartValue, part_values, value_key, 2) == 0) {
					continue;
				}

				if (key_low_case) {
					ToLower(part_values.front());
				}

				if (trim_key) {
					Trim(part_values.front());
				}

				if (trim_value && part_values.size() > 1) {
					Trim(part_values.back());
				}

				if (part_values.size() < 2) {
					attrs[part_values.front()] = "";
				}
				else {
					attrs[part_values.front()] = part_values.back();
				}

				lines++;
			}

			return lines;
		}

		static bool LessThanXored(std::string const& l, std::string const& r, std::string const& x) {
			if (l.size() != r.size() || r.size() != x.size()) {
				return false;
			}

			std::string v1, v2;
			v1.resize(l.size(), 0);
			v2.resize(l.size(), 0);

			for (size_t i = 0; i < l.size(); i++) {
				v1[i] = x[i] ^ l[i];
				v2[i] = x[i] ^ r[i];
			}

			return v1 < v2;
		}
	};

} // namespace thefox

inline std::string operator+(const char *ls, const std::string &rs) {
	std::string buf(ls);
	buf += rs;
	return buf;
}

inline bool operator==(const char *ls, const std::string &rs) {
	return !rs.compare(ls);
}

inline bool operator==(const std::string &ls, const char *rs) {
	return !ls.compare(rs);
}

inline std::string operator^(const std::string &ls, const std::string &rs) {
	if (ls.size() != rs.size()) {
		return ls;
	}

	std::string ret;
	for (size_t i = 0; i < ls.size(); i++) {
		ret.push_back(ls.at(i) ^ rs.at(i));
	}

	return ret;
}

#endif // _THEFOX_BASE_STRING_UTIL_H_
