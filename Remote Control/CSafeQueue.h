#pragma once
#include "pch.h"
#include <atomic>
#include <list>
#include "CThread.h"

template<class T>
class SafeQueue
{
	/*
		����iocpʵ�ֵ��̰߳�ȫ����
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
		size_t nOperator; //������
		T strData; //����
		//_beginthread_proc_type cbFunc; //��beginthread�������õ�����
		HANDLE hEvent; //pop������Ҫ* 
		IocpParam(int op, const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			strData = data;
			hEvent = hEve;
			//cbFunc = cb;
		}
		IocpParam() {
			nOperator = SQNone;
		}
	}PPARAM; //Post Parameter, ����Ͷ����Ϣ�Ľṹ��

	SafeQueue() {
		m_lock = false;
		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		//�����m_hCompletionPort��һ������������_beginthread�����һ������Ӧ�ô���this
		HANDLE hThread = INVALID_HANDLE_VALUE;
		if (m_hCompletionPort != NULL) {
			m_hThread = (HANDLE)_beginthread(
				&SafeQueue<T>::threadEntry, 
				0, this);
		}
	}
	~SafeQueue()
	{
		if (m_lock) return; //��ֹ������������
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL); //֪ͨ�˿ڽ���
		WaitForSingleObject(m_hThread, INFINITE); //ȷ���߳���������
		if (m_hCompletionPort != NULL) { //��ֹ�ر�����
			HANDLE hTemp = m_hCompletionPort;
			m_hCompletionPort = NULL; //�����ԣ��Ƚ�handle�ÿգ���ֹ��close�Ĺ����б�ʹ��
			CloseHandle(hTemp); //�����ٶȸ�����close
		}

	}

	bool PushBack(const T& data)
	{
		IocpParam* pParam = new IocpParam(SQPush, data);
		if (m_lock) {
			delete pParam;
			return false;
		} 
		//�����淢������
		//����������ߣ���ֹ���߳��д���ʱ���ݾ��ͷ���ʹ��new������pop��ʱ����new����Ϊ��ȴ�����
		BOOL ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam; //����m_lock���ж�֮���ֱ��false�ˣ�ǰ���post��ʧ�ܣ��������ͷŵ��ڴ��ֹй©
		printf("push back don%d %08p\r\n", ret, (void*)pParam);
		return ret;
	}

	virtual bool PopFront(T& data) {
		//��Ҫȡֵ��data
		//�����淢������
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		//hEvent:���ڵȴ�iocp�����̰߳�ֵд��ȥ
		IocpParam Param(SQPop, data, hEvent); //����Ҫ��push����new
		if (m_lock) {
			if (hEvent) CloseHandle(hEvent);
			return false;
		}
		BOOL ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}//����m_lock���ж�֮���ֱ��false�ˣ�ǰ���post��ʧ�ܣ��������ͷŵ��ڴ��ֹй©
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0; //�ȴ�д��
		//push����Ҫwait��ֻ��post�ɹ���iocpһ�����ǵ���������֤��Ч��
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
			Param.nOperator; //������Ҳ������Ϊ���屻�����߳��޸�
		}
		return -1;
	}

	bool Clear() {
		IocpParam* pParam = new IocpParam(SQClear, T()); //����Ͳ���
		if (m_lock) {
			delete pParam;
			return false;
		}
		//�����淢������

		BOOL ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL); //ע�⣺postҲ��Ҫʱ��
		if (ret == false) delete pParam; //����m_lock���ж�֮���ֱ��false�ˣ�ǰ���post��ʧ�ܣ��������ͷŵ��ڴ��ֹй©
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
			delete pParam; //push����������ߣ����ȴ���ʹ�õĶ����ڴ棬��Ҫ������ɺ�ɾ��
		}
		break;

		case SQPop:
		{
			if (m_lstData.size() > 0) {
				pParam->strData = m_lstData.front(); //pop������ͬ���ģ���������ջ�ϱ����ҶԷ�����hEvent�ȴ�������delete
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent); //��ɲ��������ѶԷ�ֹͣ�ȴ�
			}
		}
		break;

		case SQSize:
		{
			pParam->nOperator = m_lstData.size(); //��operatorΪ���崫����Ϣ
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
		while (GetQueuedCompletionStatus( //�˴��������쳣����ȡ����Ȩ�޳�ͻ�������˲��ɷ��ʵ��ڴ�����
			m_hCompletionPort, //���ն�λ����Ա��������
			&dwTransferred, //�ֲ�����ȡ��ַ�ǲ����ܷ��ʲ��ܷ��ʵ��ڴ������
			&CompletionKey,
			&pOverlapped,
			INFINITE))
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepare to exit!\r\n");
				break;
			}
			pParam = (PPARAM*)CompletionKey; //��һ�ߴ���������
			DealParam(pParam);

		}
		//�����ԣ����������ﻹ��û��
		while (GetQueuedCompletionStatus(
			m_hCompletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverlapped,
			0)) { //����в�������һ�����ᳬʱ���Ͳ������while
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				continue;
			}
			printf("���ֲ�������!\r\n");
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		HANDLE hTemp = m_hCompletionPort;
		m_hCompletionPort = NULL;
		CloseHandle(hTemp);
	}


protected:
	
	HANDLE m_hCompletionPort; //iocp�˿�
	HANDLE m_hThread;
	std::list<T> m_lstData;
	std::atomic<bool> m_lock; //��������������
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

	int threadTick() { //���ڲ��ϼ�����һ��pop����
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
			delete pParam; //push����������ߣ����ȴ���ʹ�õĶ����ڴ棬��Ҫ������ɺ�ɾ��
		}
		break;

		case SafeQueue<T>::SQPop:
		{
			if (SafeQueue<T>::m_lstData.size() > 0) {
				pParam->strData = SafeQueue<T>::m_lstData.front(); //pop������ͬ���ģ���������ջ�ϱ����ҶԷ�����hEvent�ȴ�������delete
				if ((m_base->*m_callback)(pParam->strData) == 0) {
					//�˴�callback����data�����ã�callback��ֱ�Ӽ���data��û������ͷ���1
					SafeQueue<T>::m_lstData.pop_front();
				}
			}
			delete pParam;
		}
		break;

		case SafeQueue<T>::SQSize:
		{
			pParam->nOperator = SafeQueue<T>::m_lstData.size(); //��operatorΪ���崫����Ϣ
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
//ECALLBACK: �ǲ���ΪT& data��ThreadFuncBase