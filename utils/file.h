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

#ifndef UTILS_FILE_H_
#define UTILS_FILE_H_

#include "common.h"

namespace utils {

	class FileAttribute {
	public:
		bool   is_directory_;
		time_t create_time_;
		time_t modify_time_;
		time_t access_time_;
		int64_t  size_;

		FileAttribute() {
			is_directory_ = false;
			create_time_ = 0;
			modify_time_ = 0;
			access_time_ = 0;
			size_ = 0;
		}
	};

	typedef std::map<std::string, utils::FileAttribute> FileAttributes;

	class File {
	public:
		typedef enum FILE_OPEN_MODE_ {
			FILE_M_NONE = 0x00,
			FILE_M_READ = 0x01,
			FILE_M_WRITE = 0x02,
			FILE_M_APPEND = 0x04,
			FILE_M_TEXT = 0x08,
			FILE_M_BINARY = 0x10,
			FILE_M_LOCK = 0x20,
		}FILE_OPEN_MODE;

		typedef enum FILE_SEEK_MODE_ {
			FILE_S_BEGIN = 0x00,
			FILE_S_CURRENT = 0x01,
			FILE_S_END = 0x02,
		}FILE_SEEK_MODE;

		static const char  *PATH_SEPARATOR;
		static const char   PATH_CHAR;
		static const uint64_t INVALID_SIZE = (uint64_t)-1;
		static const size_t MAX_PATH_LEN = 256;

	public:
		FILE *handle_;
		int   open_mode_;
		std::string file_name_;

		File();
		~File();
		inline bool IsOpened() { return NULL != handle_; }
		bool   Open(const std::string &file_name, int nMode);
		bool   Close();
		bool   Flush();
		size_t ReadData(std::string &data, size_t max_size);
		size_t Read(void *pBuffer, size_t nChunkSize, size_t nCount);
		bool   ReadLine(std::string &strLine, size_t nMaxCount);
		size_t Write(const void *pBuffer, size_t nChunkSize, size_t nCount);

		static std::string RegularPath(const std::string &path);
		static std::string GetFileFromPath(const std::string &path);
		static bool IsAbsolute(const std::string &path);
		static std::string GetBinPath();
		static std::string GetBinDirecotry();
		static std::string GetBinHome();
		static std::string GetUpLevelPath(const std::string &path);
		static bool GetAttribue(const std::string &strFile0, FileAttribute &nAttr);
		static bool GetFileList(const std::string &strDirectory, utils::FileAttributes &nFiles, bool bFillAttr = true, size_t nMaxCount = 0);
		static bool GetFileList(const std::string &strDirectory, const std::string &strPattern, utils::FileAttributes &nFiles, bool bFillAttr = true, size_t nMaxCount = 0);
		static utils::FileAttribute GetAttribue(const std::string &strFile);
		static bool   Move(const std::string &source, const std::string &dest, bool over_write = false);
		static bool   Copy(const std::string &source, const std::string &dest, bool over_write = true);
		static bool   IsExist(const std::string &strFile);
		static bool   Delete(const std::string &strFile);
		static bool   DeleteFolder(const std::string &path);
		static bool   CreateDir(const std::string &path);
		static std::string GetExtension(const std::string &strPath);
		static std::string GetTempDirectory();

		bool   LockRange(uint64_t offset, uint64_t size, bool try_lock = false);
		bool   UnlockRange(uint64_t offset, uint64_t size);
		uint64_t  GetPosition();
		bool Seek(uint64_t offset, FILE_SEEK_MODE nMode);
	};

}
#endif