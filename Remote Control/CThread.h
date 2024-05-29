#pragma once
#include <atomic>
#include <vector>
class ThreadFuncBase {};
typedef int (ThreadFuncBase::* FUNCTYPE)(); //这里定义FUNCTYPE

class ThreadWorker {
public:
	ThreadWorker() :thiz(NULL), func(NULL) {}
	ThreadWorker(ThreadFuncBase* obj, FUNCTYPE f) : thiz(obj), func(f) {}
	ThreadWorker(const ThreadWorker& worker) {
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) {
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}

	int operator()() {
		if (IsValid()) {
			return (thiz->*func)();
		}
		return -1;
	}

	bool IsValid() {
		return (thiz != NULL) && (func != NULL);
	}

private:
	ThreadFuncBase* thiz;
	FUNCTYPE func;
};

class CThread
{
public:
	CThread() {
		m_hThread = NULL;
	}

	~CThread() {
		Stop();
	}

	//True ：成功
	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&CThread::ThreadEntry, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}

	bool IsValid() {
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE)) return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}

	bool Stop() {
		if (m_bStatus == false) return true;
		m_bStatus = false;
		return WaitForSingleObject(m_hThread, INFINITY) == WAIT_OBJECT_0;
	}
	
	bool UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {
		m_worker.store(worker);
	}

private:
	void ThreadWorker() {
		while (m_bStatus) {
			::ThreadWorker worker = m_worker.load();
			if (worker.IsValid()) {
				int ret = worker();
				if (ret != 0) {
					CString str;
					str.Format(_T("thread found warning code %d\r\n"), ret);
					OutputDebugString(str);
				}
				if (ret < 0) {
					m_worker.store(::ThreadWorker());//重新初始化一个默认worker
				}
			}
			else {
				Sleep(1); //空挡
			}


		}
	}

	static void ThreadEntry(void* arg) {
		CThread* thiz = (CThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}
private:
	HANDLE m_hThread;
	bool m_bStatus;
	std::atomic<::ThreadWorker> m_worker; //Q: 全局namespace的这个符号是什么作用
};

class CThreadPool
{
public:
	CThreadPool(size_t size) {
		m_threads.resize(size);
	}
	CThreadPool() {}
	~CThreadPool() {
		Stop();
		m_threads.clear();
	}
	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); ++i) {
			if (m_threads[i].Start() == false) {
				ret = false;
				break;
			}
		}
		if (ret == false) {
			for (size_t i = 0; i < m_threads.size(); i++) {
				m_threads[i].Stop();
			}
		}
		return ret;
	}
	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i].Stop();
		}
	}

	int DispathchWorker(const ThreadWorker& worker) {
		
	}

private:
	std::vector<CThread> m_threads;
};