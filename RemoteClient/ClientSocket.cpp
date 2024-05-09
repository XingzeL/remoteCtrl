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

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
}

void CClientSocket::threadFunc() //开一个线程来接收数据
{
	std::string strBuffer;
	strBuffer.resize(BUFFERSIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0; //index:一共读取了多少字节
	InitSocket();
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) {
			
			TRACE("lstSend size:%d\r\n", m_lstSend.size());
			//说明有数据要发送
			m_lock.lock();
			CPacket& head = m_lstSend.front();
			m_lock.unlock();
			if (Send(head) == false) {
				TRACE("发送失败\r\n");

				continue;
			}
			std::map<HANDLE, std::list<CPacket>&>::iterator it;
			it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end()) {
				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
				auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(head.hEvent, std::list<CPacket>()));


				do {
					//int length = recv(m_sock, pBuffer, BUFFERSIZE - index, 0); //这里导致bug
					int length = recv(m_sock, pBuffer + index, BUFFERSIZE - index, 0);
					TRACE("recv %d %d\r\n", length, index); //recv为0时
					if (length > 0 || (index > 0)) { //读取成功
						//解包
						index += length;
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size); //解包，size是引用
						//监控的时候发送一个命令，回来一个pack，但是有多个TCP包；
						//传文件的时候发送一个命令，回来多个pack，如果接收到一次就pop_front，后续的包就拿不到对应的handle，需要解决
						if (size > 0) {
							//TODO：通知对应的事件
							pack.hEvent = head.hEvent; //pack是接收到的包，将hEvent和发送的包的event匹配，然后激活event
							it->second.push_back(pack); //pr是个pair，第二个元素是bool，这里将响应包放到了队列引用中
							/*SetEvent(head.hEvent); */
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it0->second) {
								SetEvent(head.hEvent);
								break;
							}
						}
					}

					else if (length <= 0 && index <= 0) {
						CloseSocket();
						SetEvent(head.hEvent);
						if (it0 != m_mapAutoClosed.end()) {
							m_mapAutoClosed.erase(it0);
							//TRACE("SetEvent %d %d \r\n", head.sCmd, it0->second);
						}
						else {
							TRACE("异常情况，没有对应的pair\r\n");
						}
						//m_mapAutoClosed.erase(it0);  //需要将AutoClosed的状态清楚
						break; //在AutoClose为false的时候需要进行break，不然会一直卡在里面循环
					}

				} while (it0->second == false); //如果不是自动关闭的话就会一直接收
				//index > 0 : 防止在不关闭的连接时出现recv=0时陷入死循环
			}
			m_lock.lock();
			m_lstSend.pop_front();
			m_mapAutoClosed.erase(head.hEvent);
			m_lock.unlock();
			if (InitSocket() == false) {
				InitSocket();
			}
		}
		//队列长度小于等于0的时候需要休眠一下
		Sleep(1);
	}
	CloseSocket();
}
void CClientSocket::SendPack(UINT nMsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam/*缓冲区的长度*/)
//消息处理函数的应答也是消息，会将内容放到wParam和lParam中
{//需要定义一个消息的数据结构(数据长度，模式autoclose等)， 回调消息的数据结构(需要知道：窗口的HANDLE， MESSAGE)
	if (InitSocket() == true) {
		int ret = send(m_sock, (char*)wParam, (int)lParam, 0);
		if (ret > 0) {

		}
		else {
			CloseSocket();
			//网络终止处理
		}
	}
	else {

	}
	if (m_sock == -1) return;
	
	//TODO:
}
//10.6.8问题：客户端按照长连接进行，而server每处理完一个东西就关闭socket，导致再次发送时候就失败

void CClientSocket::threadFunc2() {
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}