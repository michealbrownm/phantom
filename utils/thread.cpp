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

#ifdef OS_LINUX
#include <sys/prctl.h>
#elif defined OS_MAC
#include <semaphore.h>
#endif

#include "strings.h"
#include "thread.h"

#ifdef WIN32
const HANDLE utils::Thread::INVALID_HANDLE = NULL;
#else
const pthread_t utils::Thread::INVALID_HANDLE = (pthread_t)-1;
#endif

#ifdef WIN32
DWORD WINAPI utils::Thread::threadProc(LPVOID param)
#else
void *utils::Thread::threadProc(void *param)
#endif
{
	Thread *this_thread = reinterpret_cast<Thread *>(param);

	this_thread->Run();
	this_thread->thread_id_ = 0;

#ifdef WIN32
	CloseHandle(this_thread->handle_);
#endif // WIN32

	this_thread->handle_ = INVALID_HANDLE;
	this_thread->running_ = false;
#ifdef WIN32
	_endthreadex(0);
	return 0;
#else
	return NULL;
#endif
}

bool utils::Thread::Start(std::string name) {
	name_ = name;
	if (running_) {
		return false;
	}

	bool result = false;
	enabled_ = true;
	running_ = true;
	int ret = 0;
#ifdef WIN32
	handle_ = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadProc, (LPVOID)this, 0, (LPDWORD)&thread_id_);
	result = (NULL != handle_);

#elif defined OS_LINUX
	pthread_attr_t object_attr;
	pthread_attr_init(&object_attr);
	pthread_attr_setdetachstate(&object_attr, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&handle_, &object_attr, threadProc, (void *)this);
	result = (0 == ret);
	thread_id_ = handle_;
	pthread_attr_destroy(&object_attr);
#elif defined OS_MAC
	pthread_attr_t object_attr;
	pthread_attr_init(&object_attr);
	pthread_attr_setdetachstate(&object_attr, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&handle_, &object_attr, threadProc, (void *)this);
	result = (0 == ret);
	thread_id_ = (size_t)handle_;
	pthread_attr_destroy(&object_attr);
#endif
	if (!result) {
		// restore _beginthread or pthread_create's error
		utils::set_error_code(ret);

		handle_ = Thread::INVALID_HANDLE;
		enabled_ = false;
		running_ = false;
	}
	return result;
}


bool utils::Thread::Stop() {
	if (!IsObjectValid()) {
		return false;
	}

	enabled_ = false;
	return true;
}


bool utils::Thread::Terminate() {
	if (!IsObjectValid()) {
		return false;
	}

	bool result = true;
#ifdef WIN32
	if (0 == ::TerminateThread(handle_, 0)) {
		result = false;
	}
#elif defined OS_LINUX
	if (0 != pthread_cancel(thread_id_)) {
		result = false;
	}
#elif defined OS_MAC
	if (0 != pthread_cancel((pthread_t)thread_id_)) {
		result = false;
	}
#endif

	enabled_ = false;
	return result;
}

bool utils::Thread::JoinWithStop() {
	if (!IsObjectValid()) {
		return true;
	}

	enabled_ = false;
	while (running_) {
		utils::Sleep(10);
	}

	return true;
}

void utils::Thread::Run() {
	assert(target_ != NULL);

	SetCurrentThreadName(name_);

	target_->Run(this);
}

bool utils::Thread::SetCurrentThreadName(std::string name) {
#ifdef WIN32
	//not supported
	return true;
#elif defined OS_LINUX
	return 0 == prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
#elif defined OS_MAC
	pthread_setname_np(name.c_str());
    return true;
#endif //WIN32
}

size_t utils::Thread::current_thread_id() {
#ifdef WIN32
	return (size_t)::GetCurrentThreadId();
#else
	return (size_t)pthread_self();
#endif
}

utils::Mutex::Mutex()
	: thread_id_(0) {
#ifdef WIN32
	InitializeCriticalSection(&mutex_);
#else
	pthread_mutexattr_t mattr;
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex_, &mattr);
	pthread_mutexattr_destroy(&mattr);
#endif
}

utils::Mutex::~Mutex() {
#ifdef WIN32
	DeleteCriticalSection(&mutex_);
#else
	pthread_mutex_destroy(&mutex_);
#endif
}

void utils::Mutex::Lock() {
#ifdef WIN32
	EnterCriticalSection(&mutex_);
#ifdef _DEBUG
	thread_id_ = static_cast<uint32_t>(GetCurrentThreadId());
#endif
#else
	pthread_mutex_lock(&mutex_);
#ifdef _DEBUG
	thread_id_ = static_cast<uint32_t>(pthread_self());
#endif
#endif
}

void utils::Mutex::Unlock() {
#ifdef _DEBUG
	thread_id_ = 0;
#endif
#ifdef WIN32
	LeaveCriticalSection(&mutex_);
#else
	pthread_mutex_unlock(&mutex_);
#endif
}

utils::ReadWriteLock::ReadWriteLock()
	: _reads(0) {}

utils::ReadWriteLock::~ReadWriteLock() {}

void utils::ReadWriteLock::ReadLock() {
	_enterLock.Lock();
	AtomicInc(&_reads);
	_enterLock.Unlock();
}

void utils::ReadWriteLock::ReadUnlock() {
	AtomicDec(&_reads);
}

void utils::ReadWriteLock::WriteLock() {
	_enterLock.Lock();
	while (_reads > 0) {
		Sleep(0);
	}
}

void utils::ReadWriteLock::WriteUnlock() {
	_enterLock.Unlock();
}

utils::Semaphore::Semaphore(int32_t num) {
#ifdef _WIN32
	sem_ = ::CreateSemaphore(NULL, num, LONG_MAX, NULL);
#else
	sem_init(&sem_, 0, num);
#endif    
}

utils::Semaphore::~Semaphore() {
#ifdef _WIN32
	if (NULL != sem_) {
		if (0 != ::CloseHandle(sem_)) {
			sem_ = NULL;
		}
	}
#else
	sem_destroy(&sem_);
#endif    
}

bool utils::Semaphore::Wait(uint32_t millisecond) {
#ifdef _WIN32
	if (NULL == sem_)
		return false;

	DWORD ret = ::WaitForSingleObject(sem_, millisecond);
	if (WAIT_OBJECT_0 == ret || WAIT_ABANDONED == ret) {
		return true;
	}
	else {
		return false;
	}
#elif defined OS_LINUX
	int32_t ret = 0;

	if (kInfinite == millisecond) {
		ret = sem_wait(&sem_);
	}
	else {
		struct timespec ts = { 0, 0 };
		//TimeUtil::getAbsTimespec(&ts, millisecond);

		ts.tv_sec = millisecond / 1000;
		ts.tv_nsec = millisecond % 1000;

		ret = sem_timedwait(&sem_, &ts);
	}

	return -1 != ret;
#elif defined OS_MAC
	int32_t ret = 0;

	if (kInfinite == millisecond) {
		ret = sem_wait(&sem_);
	}
	else {
		//waring, mac 10.13 has no sem_timedwait?
		usleep(1000 * millisecond);
	}

	return true;
#endif
}

bool utils::Semaphore::Signal() {
#ifdef _WIN32
	BOOL ret = FALSE;

	if (NULL != sem_) {
		ret = ::ReleaseSemaphore(sem_, 1, NULL);
	}
	return TRUE == ret;
#else
	return -1 != sem_post(&sem_);
#endif
}

utils::ThreadTaskQueue::ThreadTaskQueue() {}

utils::ThreadTaskQueue::~ThreadTaskQueue() {}

int utils::ThreadTaskQueue::PutFront(Runnable *task) {
	int ret = 0;
	spinLock_.Lock();
	if (task) tasks_.push_front(task);
	ret = tasks_.size();
	spinLock_.Unlock();
	return ret;
}

int utils::ThreadTaskQueue::Put(Runnable *task) {
	int ret = 0;
	spinLock_.Lock();
	if (task) tasks_.push_back(task);
	ret = tasks_.size();
	spinLock_.Unlock();
	return ret;
}


int utils::ThreadTaskQueue::Size() {
	int ret = 0;
	spinLock_.Lock();
	ret = tasks_.size();
	spinLock_.Unlock();
	return ret;
};

utils::Runnable *utils::ThreadTaskQueue::Get() {
	Runnable *task = NULL;
	spinLock_.Lock();
	if (tasks_.size() > 0) {
		task = tasks_.front();
		tasks_.pop_front();
	}
	spinLock_.Unlock();
	return task;
}

utils::ThreadPool::ThreadPool() : enabled_(false) {}

utils::ThreadPool::~ThreadPool() {
	for (size_t i = 0; i < threads_.size(); i++) {
		if (threads_[i]) delete threads_[i];
	}
}

bool utils::ThreadPool::Init(const std::string &name, int threadNum) {
	name_ = name;
	enabled_ = true;
	return AddWorker(threadNum);
}

bool utils::ThreadPool::Exit() {
	enabled_ = false;
	for (size_t i = 0; i < threads_.size(); i++) {
		if (threads_[i]) threads_[i]->JoinWithStop();
	}

	return true;
}

void utils::ThreadPool::AddTask(Runnable *task) {
	tasks_.Put(task);
}

void utils::ThreadPool::JoinwWithStop() {
	enabled_ = false;
	for (ThreadVector::const_iterator it = threads_.begin(); it != threads_.end(); ++it) {
		(*it)->JoinWithStop();
	}
	threads_.clear();
}

bool utils::ThreadPool::WaitAndJoin() {

	while (tasks_.Size() > 0)
		Sleep(1);

	enabled_ = false;
	for (size_t i = 0; i < threads_.size(); i++) {
		if (threads_[i]) threads_[i]->JoinWithStop();
	}

	return true;
}

bool utils::ThreadPool::WaitTaskComplete() {
	while (tasks_.Size() > 0)
		Sleep(1);
	return true;
}

void utils::ThreadPool::Terminate() {
	for (ThreadVector::const_iterator it = threads_.begin(); it != threads_.end(); ++it) {
		(*it)->Terminate();
	}
}

bool utils::ThreadPool::AddWorker(int threadNum) {
	for (int i = 0; i < threadNum; ++i) {
		Thread *thread = new Thread(this);
		threads_.push_back(thread);
		bool ret = thread->Start(utils::String::Format("worker-%s-%d", name_.c_str(),i));
		if ( !ret ){
			return false;
		} 
	}

	return true;
}

void utils::ThreadPool::Run(Thread *this_thread) {
	while (enabled_) {
		utils::Runnable *task = tasks_.Get();
		if (task) task->Run(this_thread);
		else utils::Sleep(1);
	}
}

