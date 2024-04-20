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

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
}

void CClientSocket::threadFunc() //开一个线程来接收数据
{
	if (InitSocket() == false) {
		return;
	}
	std::string strBuffer;
	strBuffer.resize(BUFFERSIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0; //index:一共读取了多少字节
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) {
			//说明有数据要发送
			CPacket& head = m_lstSend.front();
			if (Send(head) == false) {
				TRACE("发送失败\r\n");
				continue;
			}
			auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>>(head.hEvent, std::list<CPacket>()));
			
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
					pack.hEvent = head.hEvent;
					pr.first->second.push_back(pack); //pr是个pair，第二个元素是bool
					SetEvent(head.hEvent);
				}
			}

			else if (length == 0 && index <= 0) {
				CloseSocket();
			}
			m_lstSend.pop_front();
		}
	}
}
