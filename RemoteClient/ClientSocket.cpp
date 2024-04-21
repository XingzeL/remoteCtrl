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

bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClose)
{
	if (m_sock == INVALID_SOCKET) {
		//if (InitSocket() == false) return false;
		_beginthread(&CClientSocket::threadEntry, 0, this); //无效的时候启动线程
	}
	auto pr = m_mapAck.insert(std::pair<HANDLE,
		std::list<CPacket>>(pack.hEvent, lstPacks));

	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClose));
	//给任务handle写入是否自动关闭的信息

	m_lstSend.push_back(pack); //加入到发送列表中
	//有线程进行发送和接收，发送后阻塞等待回应的包，解包成功后就会响应对应的Event
	WaitForSingleObject(pack.hEvent, INFINITE); //等待响应
	std::map<HANDLE, std::list<CPacket>>::iterator it;
	it = m_mapAck.find(pack.hEvent);

	if (it != m_mapAck.end()) {
		std::list<CPacket>::iterator i;
		//10.6.8的最后冯老师把这个for删掉了，但是导致没有画面显示
		for (i = it->second.begin(); i != it->second.end(); i++) { 
			//这个for循环没有的话画面就不会显示，SendCommandPacket就会返回-1因为lstPack的大小是0
			lstPacks.push_back(*i);
		}
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
			CPacket& head = m_lstSend.front();
			if (Send(head) == false) {
				TRACE("发送失败\r\n");

				continue;
			}
			std::map<HANDLE, std::list<CPacket>>::iterator it;
			it = m_mapAck.find(head.hEvent);
			std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
			auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>>(head.hEvent, std::list<CPacket>()));
			
			do {
				int length = recv(m_sock, pBuffer, BUFFERSIZE - index, 0);
				if (length > 0 || index > 0) { //读取成功
					//解包
					index += length;
					size_t size = (size_t)index;
					CPacket pack((BYTE*)pBuffer, size); //解包，size是引用
					//监控的时候发送一个命令，回来一个pack，但是有多个TCP包；
					//传文件的时候发送一个命令，回来多个pack，如果接收到一次就pop_front，后续的包就拿不到对应的handle，需要解决
					if (size > 0) {
						//TODO：通知对应的事件
						pack.hEvent = head.hEvent; //pack是接收到的包，将hEvent和发送的包的event匹配，然后激活event
						it->second.push_back(pack); //pr是个pair，第二个元素是bool
						SetEvent(head.hEvent);
						memmove(pBuffer, pBuffer + size, index - size);
						index -= size;
						if (it0->second) {
							SetEvent(head.hEvent);
						}
					}
				}

				else if (length == 0 && index <= 0) {
					CloseSocket();
					SetEvent(head.hEvent);
				}
				
			} while (it0->second == false); //如果不是自动关闭的话就会一直接收
			m_lstSend.pop_front();
			InitSocket();
			//CloseSocket(); //连接测试的时候加上
		}
	}
	CloseSocket();
}
//10.6.8问题：客户端按照长连接进行，而server每处理完一个东西就关闭socket，导致再次发送时候就失败