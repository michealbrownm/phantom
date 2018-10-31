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


#ifndef UTILS_SINGLETION_H_
#define UTILS_SINGLETION_H_
#include<assert.h>

#ifndef NULL
#define  NULL 0
#endif

namespace utils {

	template<class T>
	class Singleton {
	private:
		static T *instance_;

	protected:
		Singleton() {}
		virtual ~Singleton() {}

	public:
		static bool InitInstance() {
			if (NULL != instance_) return false;

			instance_ = new T;
			return true;
		}

		template<class CLS_TYPE, class ARG_TYPE> static bool InitInstance(ARG_TYPE nArg) {
			if (NULL != instance_) return false;

			instance_ = new CLS_TYPE(nArg);
			return true;
		}

		static bool ExitInstance() {
			if (NULL == instance_) return false;

			delete instance_;
			instance_ = NULL;
			return true;
		}

		inline static T *GetInstance() {
			return instance_;
		}

		inline static T &Instance() {
			assert(NULL != instance_);
			return *instance_;
		}

		template<class CLS_TYPE> inline static CLS_TYPE *GetSubInstance() {
			if (NULL == instance_) {
				return NULL;
			}

			return dynamic_cast<CLS_TYPE *>(instance_);
		}

		template<class CLS_TYPE> inline static CLS_TYPE &SubInstance() {
			assert(NULL != instance_);
			return *dynamic_cast<CLS_TYPE *>(instance_);
		}
	};

	template<class T> T *utils::Singleton<T>::instance_ = NULL;

} // namespace utils

#endif // _UTILS_SINGLETION_H_