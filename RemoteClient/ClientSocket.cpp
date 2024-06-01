#include "pch.h"
#include "ClientSocket.h"

//CServerSocket server; //声明的全局变量,会在主函数之前进行初始化 
//这样的好处是在main函数之前是单线程，不会有竞争的风险产生

CClientSocket* CClientSocket::m_instance = NULL; //置空，全局成员需要显示初始化
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pclient = CClientSocket::getInstance(); //也是全局的指针，调用了共有的访问方法，让单例对象被初始化并赋给指针

std::string GetErrorInfo(int wsaErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);

	return ret;
}

bool CClientSocket::InitSocket()
{
	//修改的bug：因为每次送消息都会关闭连接，client不能像server一样socket只初始化一次
	// 每次Inite都要初始化m_sock

	//10.6.10：网多问题: 在发送接收的线程中会调用InitSocket,CloseSocket到创建socket之间
	if (m_sock != INVALID_SOCKET) CloseSocket(); //这里关系到CloseSocket到创建socket之间
	m_sock = socket(PF_INET, SOCK_STREAM, 0); //选择协议族：IPV4， stream：TCP协议
	//TODO: 进行socket的校验
	if (m_sock == -1) return false;
	sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	//serv_adr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//TRACE("addr: %08X nIP %08X\r\n", serv_adr.sin_addr.s_addr, nIp);
	serv_adr.sin_addr.s_addr = htonl(m_nIP);
	serv_adr.sin_port = htons(m_nPort);
	//为什么server有多个IP: 有的时候是对内工作，比如是数据库，由内网的设备进行连接；
	// 对外的IP地址将对外的服务暴露给外面，内网的带宽可以弄得很宽(用光纤) 外网带宽成本高

	if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
		//LPCTSTR msg = "IP address not exist!";
		AfxMessageBox(_T("不存在IP地址！"));
		return false;
	}
	int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));
	if (ret == -1) {
		AfxMessageBox(_T("连接失败"));
		TRACE("连接失败：%d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
	}
	//else {
	//	AfxMessageBox(_T("连接成功"));

	//}
	return true;
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClose, WPARAM wParam)
{
	//if (m_hThread == INVALID_HANDLE_VALUE) {
	//	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	//} //第一次发现没有消息处理线程的话就创建一个
	UINT nMode = isAutoClose ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	//有消息接收线程：发送给对应的线程号对应的消息和包，目前只发WM_SEND_PACK消息，包不一样，得到的响应就不同，对应不同响应会再OnSendPacketAck中对号入座(使用switch-case)
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData,(LPARAM)hWnd);
	//发送消息给消息线程
	if (ret == false) {
		delete pData;
	}
	return ret;
}

/* 队列机制时候的SendPacket
bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClose)
{
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
		//if (InitSocket() == false) return false;
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this); //无效的时候启动线程,这个线程用于发送和接收响应包
		TRACE("start thread\r\n"); //检验是否线程的创建只有一次
	}

	m_lock.lock();
	auto pr = m_mapAck.insert(std::pair<HANDLE,
		std::list<CPacket>&>(pack.hEvent, lstPacks)); //一个事务开启，加入对应的时间handle和响应包列表
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClose));
	TRACE("cmd:%d event %08X\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
	//给任务handle写入是否自动关闭的信息

	m_lstSend.push_back(pack); //加入到发送列表中
	m_lock.unlock();
	//有线程进行发送和接收，发送后阻塞等待回应的包，解包成功后就会响应对应的Event
	WaitForSingleObject(pack.hEvent, INFINITE); //等待响应
	std::map<HANDLE, std::list<CPacket>&>::iterator it;
	it = m_mapAck.find(pack.hEvent);

	if (it != m_mapAck.end()) {
		std::list<CPacket>::iterator i;
		//10.6.8的最后冯老师把这个for删掉了，但是导致没有画面显示
		//for (i = it->second.begin(); i != it->second.end(); i++) { 
			//这个for循环没有的话画面就不会显示，SendCommandPacket就会返回-1因为lstPack的大小是0
			// 在m_mapAck中的list改成&前进行的手动插入
		//	lstPacks.push_back(*i);
		//}
		m_mapAck.erase(it);
		return true;
	}
	return false;
}
*/
CClientSocket::CClientSocket(const CClientSocket& ss)
{
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAutoClose = ss.m_bAutoClose;
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
	for (; it != ss.m_mapFunc.end(); it++) {
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}

CClientSocket::CClientSocket():
	m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true),
	m_hThread(INVALID_HANDLE_VALUE)
{
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误"), MB_OK | MB_ICONERROR);
		exit(0); //结束进程
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL); //默认，自动重置，初始值为未处理，名字为""
	
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {
		TRACE("网络消息处理线程启动失败!\r\n");
	}
	CloseHandle(m_eventInvoke); //之后就不需要这个event了
	//m_sock = socket(PF_INET, SOCK_STREAM, 0); //选择协议族：IPV4， stream：TCP协议
	m_buffer.resize(BUFFERSIZE);
	memset(m_buffer.data(), 0, BUFFERSIZE);
	struct {
		UINT message;
		MSGFUNC func;
	}funcs[] = {
		{WM_SEND_PACK, &CClientSocket::SendPack},
		{0, NULL},
	};
	for (int i = 0; funcs[i].message != 0; ++i) {
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false)
			TRACE("插入失败，消息值： %d  函数值： %08X  序号：%d\r\n", funcs[i].message, funcs[i].func, i);
	}

}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	//thiz->threadFunc();
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam/*缓冲区的长度*/)
//消息处理函数的应答也是消息，会将内容放到wParam和lParam中
{//需要定义一个消息的数据结构(数据长度，模式autoclose等)， 回调消息的数据结构(需要知道：窗口的HANDLE， MESSAGE)
	PACKET_DATA data = *(PACKET_DATA*)wParam; //把内容复制到本地来，防止另一个线程的局部变量被释放
	//俩线程之间发送数据，如果发送的是一个局部变量的话可能会在这个线程使用之前就释放了
	delete (PACKET_DATA*)wParam; //另一个线程传来的包是new出来的，接收到后，复制内容，当场释放，这个在响应函数中也应当使用
	HWND hWnd = (HWND)lParam;
	//以下是防止server关闭了socket，但是这边还有一两个包没接收的情况：current包
	size_t nTemp = data.strData.size();
	CPacket current((BYTE*)data.strData.c_str(), nTemp);
	if (InitSocket() == true) {

		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0); //将Packet的内容发送出去
		if (ret > 0) {
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFERSIZE); //使用resize的好处：用的堆的内存，又可以自动析构，比new更好
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock, pBuffer + index, BUFFERSIZE - index, 0);
				if (length > 0 || (index > 0)) { //length: 收到的数据； index: 缓冲区中的数据
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0) {
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						//为了跨线程传输包，并且保证生命周期，传给另一个线程的包应该是new出来的
						if (data.nMode & CSM_AUTOCLOSE) {
							
							CloseSocket();
							return;
						}
						index -= nLen;
						//memmove(pBuffer, pBuffer + index, nLen);
						memmove(pBuffer, pBuffer + nLen, index);
					}
				}
				else {
					TRACE("recv failed length %d, index %d\r\n", length, index);
					//对方关闭了socket或者网络设备异常
					CloseSocket();

					::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(current.sCmd, NULL, 0), 1); //响应 通知结束
				}
			}
		}
		else {
			CloseSocket();
			//网络终止处理
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1); //告诉业务线程结束了
		}
	}
	else {
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2); //免得一直等
	}
	
	//TODO:
}
//10.6.8问题：客户端按照长连接进行，而server每处理完一个东西就关闭socket，导致再次发送时候就失败

void CClientSocket::threadFunc2() {
	//设置event为已处理
	SetEvent(m_eventInvoke); 
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}