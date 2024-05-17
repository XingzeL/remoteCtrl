#pragma once
template<class T>
class SafeQueue
{
	/*
		����iocpʵ�ֵ��̰߳�ȫ����
	*/
	SafeQueue();
	~SafeQueue();
	void PushBack(const T& data);
	void PopFront(T& data);
	size_t Size();
	void Clear();
private:
	static void threadEntry(void* arg);
	void threadMain();
private:
	std::initializer_list<T> m_lstData;
	HANDLE m_hCompletionPort; //iocp�˿�
	HANDLE m_hThread;
public:
	typedef struct IocpParam {
		int nOperator; //������
		T strData; //����
		//_beginthread_proc_type cbFunc; //��beginthread�������õ�����
		HANDLE hEvent; //pop������Ҫ* 
		IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
			nOperator = op;
			strData = sData;
			//cbFunc = cb;
		}
		IocpParam() {
			nOperator = -1;
		}
	}PPARAM; //Post Parameter, ����Ͷ����Ϣ�Ľṹ��
	enum {
		SQPush,
		SQtPop,
		SQSize,
		SQClear,
	};
};

