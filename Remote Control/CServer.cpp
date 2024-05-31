#include "pch.h"
#include "CServer.h"
#pragma warning(disable:4407)

template<EOperator op>
AcceptOverlapped<op>::AcceptOverlapped() {
    m_worker = ThreadWorker(this, (FUNCTYPE)& AcceptOverlapped<op>::AcceptWorker);
    m_operator = EAccept;
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024);
    m_server = NULL;
}

template<EOperator op>
int AcceptOverlapped<op>::AcceptWorker() { //Accept动作，执行一次
    INT lLength = 0, rLength = 0;
    if (*(LPDWORD)*m_client.get() > 0) {
        GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16,
            sizeof(sockaddr_in) + 16,
            (sockaddr**)m_client->GetLocalAddr(), &lLength,//本地地址
            (sockaddr**)m_client->GetRemoteAddr() ,&rLength//远程地址
        );

        if (!m_server->NewAccept()) //执行一次accept的请求
        {
            return -2; //accept请求发送失败
        }
    }
    return -1;
}


CClient::CClient() : m_isbusy(false), m_overlapped(new ACCEPTOVERLAPPED()) {
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    m_buffer.resize(1024);
    memset(&m_laddr, 0, sizeof(m_laddr));
    memset(&m_raddr, 0, sizeof(m_raddr));
}


void CClient::SetOverlapped(PCLIENT& ptr) {
    m_overlapped->m_client = ptr;
}

CClient::operator SOCKET() {
    return m_sock;
}

CClient::operator PVOID() {
    return &m_buffer[0]; //返回缓冲区的首地址
}

CClient::operator LPOVERLAPPED() {
    return &m_overlapped->m_overlapped;
}

CClient::operator LPDWORD() {
    return &m_received;
}