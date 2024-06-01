#pragma once
#include "CThread.h"
#include <map>
#include "CSafeQueue.h"
#include "MSWSock.h"

enum EOperator{
    ENone,
    EAccept,
    ERecv,
    ESend,
    EError
};

class CServer;
class CClient;
typedef std::shared_ptr<CClient> PCLIENT;

class COverlapped {
public:
    OVERLAPPED m_overlapped; //overlapped对象
    DWORD m_operator;  //申请代号
    std::vector<char> m_buffer; //缓冲区
    ThreadWorker m_worker; //对应的处理函数
    CServer* m_server; //服务器对象
    PCLIENT m_client; //对应的客户端
    WSABUF m_wsabuffer; //用于WSASend, WSARecv 没用
    virtual ~COverlapped() {  //虚析构函数
        m_buffer.clear();
    }
};

template<EOperator> class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

template<EOperator> class RecvOverlapped;
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;

template<EOperator> class SendOverlapped;
typedef SendOverlapped<ESend> SENDOVERLAPPED;

//******************封装client**********************

class CClient: public ThreadFuncBase {
public:
    CClient();

    ~CClient() {
        closesocket(m_sock);
    }

    void SetOverlapped(PCLIENT& ptr);

    CClient& operator+() {

   }

    operator SOCKET();

    operator PVOID();

    operator LPOVERLAPPED();

    operator LPDWORD();

    LPWSABUF RecvWSABuffer();

    LPWSABUF SendWSABuffer();

    sockaddr_in* GetLocalAddr() { return &m_laddr; }
    sockaddr_in* GetRemoteAddr() { return &m_raddr; }
    size_t GetBufferSize() const { return m_buffer.size(); }
    DWORD& flags() { return m_flags; }

    bool Recv();

    int Send(void* buffer, size_t nSize);
    int SendData(std::vector<char>& data); //用于作为callback传给队列类

private:
    SOCKET m_sock;
    DWORD m_received;
    DWORD m_flags;

    std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
    std::shared_ptr<RECVOVERLAPPED> m_recv;
    std::shared_ptr<SENDOVERLAPPED> m_send;
    std::vector<char> m_buffer;
    size_t m_used; //已经使用的缓冲区的大小
    sockaddr_in m_laddr; //地址
    sockaddr_in m_raddr;
    bool m_isbusy;
    //SendQueue<std::vector<char>> m_vecSend; //发送数据的队列
};


template<EOperator>
class AcceptOverlapped : public COverlapped, ThreadFuncBase
{
public:
    AcceptOverlapped();
    int AcceptWorker();
    //PCLIENT m_client; //指向它所属的client，但是这样出现相互依赖：对client类进行前向声明：提供给编译器一个类的存在的信息
};

template<EOperator>
class RecvOverlapped : public COverlapped, ThreadFuncBase
{
public:
    RecvOverlapped();

    int RecvWorker() { //Accept动作
        int ret = m_client->Recv(); //数据并不能直接被overlapped传送，还在网卡的缓冲区中，要进行recv
        return ret;
    }
};


template<EOperator>
class SendOverlapped : public COverlapped, ThreadFuncBase
{
public:
    SendOverlapped();
    //    : m_operator(ESend), m_worker(this, &SendOverlapped::SendWorker) {
    //    memset(&m_overlapped, 0, sizeof(m_overlapped));
    //    m_buffer.resize(1024 * 256);
    //}
    int SendWorker() { //Send动作
        return -1;
    }
};
typedef SendOverlapped<ESend> SENDOVERLAPPED;

template<EOperator>
class ErrorOverlapped : public COverlapped, ThreadFuncBase
{
public:
    ErrorOverlapped() : m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024);
    }
    int ErrorWorker() { //Error动作
        return -1;
    }
};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;

class CServer :
    public ThreadFuncBase
{
public:
    CServer(const std::string& ip = "0.0.0.0", short port = 9527) :m_pool(10) {
        //初始化IOCP
        m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
        m_sock = INVALID_SOCKET;
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(port);
        m_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    }
    ~CServer() {}
    void BindSocket(SOCKET m_sock) {
        CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
    }
    bool StartServer() {
        CreateSocket();
        if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return false;
        }
        if (listen(m_sock, 5) == -1) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return false;
        }
        m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4); //创建一个iocp
        if (m_hIOCP == NULL) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }
        CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0); //绑定socket和iocp
        m_pool.Invoke(); //启动线程池
        m_pool.DispathchWorker(ThreadWorker(this, (FUNCTYPE)&CServer::threadIocp));
        if (!NewAccept()) return false; //发送给iocp一个accept请求， 完成后在threadIocp中得到通知

        return true;
    }

    bool NewAccept() {
        PCLIENT pClient(new CClient()); //创建的client
        pClient->SetOverlapped(pClient);
        m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
        if (!AcceptEx(m_sock,
                *pClient,
                *pClient,
                0,
                sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
                *pClient,
                *pClient))
        {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }
        return true;
    }

private:
    void CreateSocket() {
        m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        int opt = 1;
        setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

     }

    int threadIocp();

public:
    CThreadPool m_pool; //线程池
    HANDLE m_hIOCP; //完成端口
    SOCKET m_sock;  //服务端的socket
    sockaddr_in m_addr;  //server监听的地址
    std::map<SOCKET, std::shared_ptr<CClient>> m_client; //每一个客户端连接都有一个对应的信息
    SafeQueue<CClient> m_lstClient;
};

