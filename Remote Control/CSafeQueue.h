#pragma once
#include "pch.h"
#include <atomic>
#include <list>
#include "CThread.h"

template<class T>
class SafeQueue
{
	/*
		利用iocp实现的线程安全队列
	*/
public:
	enum {
		SQNone,
		SQPush,
		SQPop,
		SQSize,
		SQClear,
	};

	typedef struct IocpParam {
		size_t nOperator; //操作码
		T strData; //数据
		//_beginthread_proc_type cbFunc; //从beginthread参数里拿的类型
		HANDLE hEvent; //pop操作需要* 
		IocpParam(int op, const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			strData = data;
			hEvent = hEve;
			//cbFunc = cb;
		}
		IocpParam() {
			nOperator = SQNone;
		}
	}PPARAM; //Post Parameter, 用于投递信息的结构体

	SafeQueue() {
		m_lock = false;
		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		//这里的m_hCompletionPort是一个句柄，下面的_beginthread的最后一个参数应该传入this
		HANDLE hThread = INVALID_HANDLE_VALUE;
		if (m_hCompletionPort != NULL) {
			m_hThread = (HANDLE)_beginthread(
				&SafeQueue<T>::threadEntry, 
				0, this);
		}
	}
	~SafeQueue()
	{
		if (m_lock) return; //防止反复调用析构
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL); //通知端口结束
		WaitForSingleObject(m_hThread, INFINITE); //确保线程正常结束
		if (m_hCompletionPort != NULL) { //防止关闭两次
			HANDLE hTemp = m_hCompletionPort;
			m_hCompletionPort = NULL; //防御性：先将handle置空，防止在close的过程中被使用
			CloseHandle(hTemp); //进行速度更慢的close
		}

	}

	bool PushBack(const T& data)
	{
		IocpParam* pParam = new IocpParam(SQPush, data);
		if (m_lock) {
			delete pParam;
			return false;
		} 
		//往里面发送数据
		//丢完就立马走，防止在线程中处理时数据就释放了使用new。而在pop的时候不用new，因为会等待数据
		BOOL ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam; //即便m_lock在判断之后又变成false了，前面的post会失败，在这里释放掉内存防止泄漏
		printf("push back don%d %08p\r\n", ret, (void*)pParam);
		return ret;
	}

	virtual bool PopFront(T& data) {
		//想要取值到data
		//往里面发送数据
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		//hEvent:用于等待iocp处理线程把值写进去
		IocpParam Param(SQPop, data, hEvent); //不需要像push那用new
		if (m_lock) {
			if (hEvent) CloseHandle(hEvent);
			return false;
		}
		BOOL ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}//即便m_lock在判断之后又变成false了，前面的post会失败，在这里释放掉内存防止泄漏
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0; //等待写完
		//push不需要wait，只用post成功，iocp一定会那到。这样保证了效率
		if (ret) {
			//data = Param.strData;
		}
		return ret;
	}

	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(SQSize, T(), hEvent);
		if (m_lock) {
			if (hEvent) CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM),
			(ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			Param.nOperator; //操作数也可以作为载体被处理线程修改
		}
		return -1;
	}

	bool Clear() {
		IocpParam* pParam = new IocpParam(SQClear, T()); //丢完就不管
		if (m_lock) {
			delete pParam;
			return false;
		}
		//往里面发送数据

		BOOL ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL); //注意：post也需要时间
		if (ret == false) delete pParam; //即便m_lock在判断之后又变成false了，前面的post会失败，在这里释放掉内存防止泄漏
		return ret;
	}
protected:
	static void threadEntry(void* arg) {
		SafeQueue<T>* thiz = (SafeQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}

	virtual void DealParam(PPARAM* pParam) {
		switch (pParam->nOperator) {
		case SQPush:
		{
			m_lstData.push_back(pParam->strData);
			printf("delete %08p\r\n", (void*)pParam);
			delete pParam; //push操作发完就走，不等待，使用的堆上内存，需要操作完成后删除
		}
		break;

		case SQPop:
		{
			if (m_lstData.size() > 0) {
				pParam->strData = m_lstData.front(); //pop操作是同步的，传来的是栈上变量且对方在用hEvent等待，不用delete
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent); //完成操作，提醒对方停止等待
			}
		}
		break;

		case SQSize:
		{
			pParam->nOperator = m_lstData.size(); //以operator为载体传回信息
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
		}
		break;

		case SQClear:
		{
			m_lstData.clear();
			delete pParam;
		}
		break;

		default:
		{
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
		}
	}

	virtual void threadMain()
	{
		DWORD dwTransferred = 0;
		PPARAM* pParam = NULL;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = NULL;
		while (GetQueuedCompletionStatus( //此处触发了异常：读取访问权限冲突：访问了不可访问的内存区段
			m_hCompletionPort, //最终定位到成员变量这里
			&dwTransferred, //局部变量取地址是不可能访问不能访问的内存区域的
			&CompletionKey,
			&pOverlapped,
			INFINITE))
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepare to exit!\r\n");
				break;
			}
			pParam = (PPARAM*)CompletionKey; //另一边传来的数据
			DealParam(pParam);

		}
		//防御性：看看队列里还有没有
		while (GetQueuedCompletionStatus(
			m_hCompletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverlapped,
			0)) { //如果有残留数据一定不会超时，就不会进入while
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				continue;
			}
			printf("发现残留数据!\r\n");
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		HANDLE hTemp = m_hCompletionPort;
		m_hCompletionPort = NULL;
		CloseHandle(hTemp);
	}


protected:
	
	HANDLE m_hCompletionPort; //iocp端口
	HANDLE m_hThread;
	std::list<T> m_lstData;
	std::atomic<bool> m_lock; //队列正在析构中
};



template<class T>
class SendQueue
	: public SafeQueue<T> , public ThreadFuncBase{
public:
	typedef int (ThreadFuncBase::* ECALLBACK)(T& data);
	SendQueue(ThreadFuncBase* obj, ECALLBACK callback)
		:SafeQueue<T>(), m_base(obj), m_callback(callback) {
		m_thread.Start();
		m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE) & SendQueue<T>::threadTick));
	}
	

protected:
	virtual bool PopFront(T& data) { return false; };

	virtual bool PopFront() {
		typename SafeQueue<T>::IocpParam* Param = new typename SafeQueue<T>::IocpParam(SafeQueue<T>::SQPop, T());
		if (SafeQueue<T>::m_lock) {
			delete Param;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(SafeQueue<T>::m_hCompletionPort, sizeof(*Param), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			delete Param;
			return false;
		}
		return ret;
	}

	int threadTick() { //用于不断激活下一个pop处理
		if (SafeQueue<T>::m_lstData.size() > 0) {
			PopFront();
		}
		Sleep(1);
		return 0;
	}

	virtual void DealParam(typename SafeQueue<T>::PPARAM* pParam) {
		switch (pParam->nOperator) {
		case SafeQueue<T>::SQPush:
		{
			SafeQueue<T>::m_lstData.push_back(pParam->strData);
			delete pParam; //push操作发完就走，不等待，使用的堆上内存，需要操作完成后删除
		}
		break;

		case SafeQueue<T>::SQPop:
		{
			if (SafeQueue<T>::m_lstData.size() > 0) {
				pParam->strData = SafeQueue<T>::m_lstData.front(); //pop操作是同步的，传来的是栈上变量且对方在用hEvent等待，不用delete
				if ((m_base->*m_callback)(pParam->strData) == 0) {
					//此处callback传递data的引用，callback中直接剪切data，没剪切完就返回1
					SafeQueue<T>::m_lstData.pop_front();
				}
			}
			delete pParam;
		}
		break;

		case SafeQueue<T>::SQSize:
		{
			pParam->nOperator = SafeQueue<T>::m_lstData.size(); //以operator为载体传回信息
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
		}
		break;

		case SafeQueue<T>::SQClear:
		{
			SafeQueue<T>::m_lstData.clear();
			delete pParam;
		}
		break;

		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}


private:
	ThreadFuncBase* m_base;
	ECALLBACK m_callback;
	CThread m_thread;
};

typedef SendQueue<std::vector<char>>::ECALLBACK SENDCALLBACK; 
//ECALLBACK: 是参数为T& data的ThreadFuncBase