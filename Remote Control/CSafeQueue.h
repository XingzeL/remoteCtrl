#pragma once
template<class T>
class SafeQueue
{
	/*
		利用iocp实现的线程安全队列
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
	HANDLE m_hCompletionPort; //iocp端口
	HANDLE m_hThread;
public:
	typedef struct IocpParam {
		int nOperator; //操作码
		T strData; //数据
		//_beginthread_proc_type cbFunc; //从beginthread参数里拿的类型
		HANDLE hEvent; //pop操作需要* 
		IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
			nOperator = op;
			strData = sData;
			//cbFunc = cb;
		}
		IocpParam() {
			nOperator = -1;
		}
	}PPARAM; //Post Parameter, 用于投递信息的结构体
	enum {
		SQPush,
		SQtPop,
		SQSize,
		SQClear,
	};
};

