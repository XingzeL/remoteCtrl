#pragma once
#include "CThread.h"
#include <map>

class CClient {

};

enum EOperator{
    ENone,
    EAccept,
    ERecv,
    ESend,
    EError
};

class COverlapped {
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator;
    std::vector<char> m_buffer; //������
    ThreadWorker m_worker; //��Ӧ�Ĵ�����
    COverlapped() {
        m_operator = ENone;
        //memset(&m_overlapped, 0, sizeof(m_overlapped));
        //memset(m_buffer, 0, sizeof(m_buffer));
    }
};

template<EOperator>
class AcceptOverlapped : public COverlapped, ThreadFuncBase
{
public:
    AcceptOverlapped() : m_operator(EAccept), m_worker(this,&AcceptOverlapped::AcceptWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlpped));
        m_buffer.resize(1024);
    }
    int AcceptWorker() { //Accept����

    }
};
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

template<EOperator>
class RecvOverlapped : public COverlapped, ThreadFuncBase
{
public:
    RecvOverlapped() : m_operator(ERecv), m_worker(this, &RecvOverlapped::RecvWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlpped));
        m_buffer.resize(1024 * 256);
    }
    int RecvWorker() { //Accept����

    }
};
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;

template<EOperator>
class SendOverlapped : public COverlapped, ThreadFuncBase
{
public:
    SendOverlapped() : m_operator(ESend), m_worker(this, &SendOverlapped::SendWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlpped));
        m_buffer.resize(1024 * 256);
    }
    int SendWorker() { //Send����

    }
};
typedef SendOverlapped<ESend> SENDOVERLAPPED;

template<EOperator>
class ErrorOverlapped : public COverlapped, ThreadFuncBase
{
public:
    ErrorOverlapped() : m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlpped));
        m_buffer.resize(1024);
    }
    int ErrorWorker() { //Error����

    }
};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;

class CServer :
    public ThreadFuncBase
{
public:
    CServer(const std::string& ip = "0.0.0.0", short port = 9527) :m_pool(10) {
        //��ʼ��IOCP
        m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
        m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

        int opt = 1;
        setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        if (bind(m_sock, (sockaddr*)&addr, sizeof(addr)) == -1) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return;
        }
        if (listen(m_sock, 5) == -1) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return;
        }
        m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
        if (m_hIOCP == NULL) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return;
        }
        CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
        m_pool.DispathchWorker(ThreadWorker(this, (FUNCTYPE)&CServer::threadIocp));
        m_pool.DispathchWorker(ThreadWorker(this, (FUNCTYPE)&CServer::threadIocp));
        m_pool.DispathchWorker(ThreadWorker(this, (FUNCTYPE)&CServer::threadIocp));
        m_pool.DispathchWorker(ThreadWorker(this, (FUNCTYPE)&CServer::threadIocp)); //GetQueuedCompletionStatus���Զ���̴߳���
    }
    ~CServer() {}
private:
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
private:
    CThreadPool m_pool;
    HANDLE m_hIOCP; //��ɶ˿�
    SOCKET m_sock;
    std::map<SOCKET, std::shared_ptr<CClient>> m_client; //ÿһ���ͻ������Ӷ���һ����Ӧ����Ϣ
};

