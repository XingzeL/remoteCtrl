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
    OVERLAPPED m_overlapped;
    DWORD m_operator;
    std::vector<char> m_buffer; //缓冲区
    ThreadWorker m_worker; //对应的处理函数
    CServer* m_server; //服务器对象
};

template<EOperator> class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

class CClient {
public:
    CClient();

    ~CClient() {
        closesocket(m_sock);
    }

    void SetOverlapped(PCLIENT& ptr);

    operator SOCKET();

    operator PVOID();

    operator LPOVERLAPPED();

    operator LPDWORD();

    sockaddr_in* GetLocalAddr() { return &m_laddr; }
    sockaddr_in* GetRemoteAddr() { return &m_raddr;}


private:
    SOCKET m_sock;
    DWORD m_received;
    //ACCEPTOVERLAPPED m_overlapped; //这里声明了实体的AccepteOverlapped对象，而那个对象中有CClient对象，形成循环依赖
    std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
    std::vector<char> m_buffer;
    sockaddr_in m_laddr; //地址
    sockaddr_in m_raddr;
    bool m_isbusy;
};


template<EOperator>
class AcceptOverlapped : public COverlapped, ThreadFuncBase
{
public:
    AcceptOverlapped();
    int AcceptWorker();
    PCLIENT m_client;
};

template<EOperator>
class RecvOverlapped : public COverlapped, ThreadFuncBase
{
public:
    RecvOverlapped() : m_operator(ERecv), m_worker(this, &RecvOverlapped::RecvWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024 * 256);
    }
    int RecvWorker() { //Accept动作

    }
};
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;

template<EOperator>
class SendOverlapped : public COverlapped, ThreadFuncBase
{
public:
    SendOverlapped() : m_operator(ESend), m_worker(this, &SendOverlapped::SendWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024 * 256);
    }
    int SendWorker() { //Send动作

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
        if (!NewAccept()) return false;

        return false;
    }

    bool NewAccept() {
        PCLIENT pClient(new CClient());
        pClient->SetOverlapped(pClient);
        m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
        if (FALSE ==
            AcceptEx(m_sock,
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



    int threadIocp() {
        DWORD transferred = 0;
        ULONG_PTR CompletionKey = 0;
        OVERLAPPED* lpOverlapped = NULL;
        if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITY))
        {
            if (transferred > 0 && (CompletionKey != 0)) {
                COverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, COverlapped, m_overlapped);
                switch (pOverlapped->m_operator) {
                case EAccept:
                {
                    ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
                    m_pool.DispathchWorker(pOver->m_worker);
                }
                break;
                case ERecv:
                {
                    RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
                    m_pool.DispathchWorker(pOver->m_worker);
                }
                    break;
                case ESend:
                {
                    SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
                    m_pool.DispathchWorker(pOver->m_worker);
                }
                    break;
                case EError:
                {
                    ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
                    m_pool.DispathchWorker(pOver->m_worker);
                }
                    break;
                }
            }
            else {
                return -1;
            }
        }
        return 0;
    }
public:
    CThreadPool m_pool; //线程池
    HANDLE m_hIOCP; //完成端口
    SOCKET m_sock;  //服务端的socket
    sockaddr_in m_addr;  //server监听的地址
    std::map<SOCKET, std::shared_ptr<CClient>> m_client; //每一个客户端连接都有一个对应的信息
    SafeQueue<CClient> m_lstClient;
};

