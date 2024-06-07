#pragma once
#include <WinSock2.h>
#include <memory>

enum class  PTYPE { //为class，加入作用范围
	UTypeTCP = 1,
	UTypeUDP
};


class CUSockaddrIn {
public:
	CUSockaddrIn() {
		memset(&m_addr, 0, sizeof(m_addr));
		m_port = -1;
	}

	CUSockaddrIn(sockaddr_in addr) {
		memcpy(&m_addr, &addr, sizeof(addr));
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	CUSockaddrIn(UINT nIP, short nPort) {
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = htonl(nIP);
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = nPort;
	}
	CUSockaddrIn(const std::string& strIP, short nPort) {
		m_ip = strIP;
		m_port = nPort;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = inet_addr(strIP.c_str());
	}

	CUSockaddrIn(const CUSockaddrIn& addr) {
		memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
		m_ip = addr.m_ip;
		m_port = addr.m_port;
	}

	CUSockaddrIn& operator=(const CUSockaddrIn& addr) {
		if (this != &addr) {
			memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
			m_ip = addr.m_ip;
			m_port = addr.m_port;
		}
		return *this;
	}

	operator sockaddr* () const { //使得可以直接传给bind的第二个参数而不用(sockaddr*)强制转换
		return (sockaddr*)&m_addr;
	}

	operator void* ()const {
		return (void*)&m_addr;
	}

	void update() {
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	std::string GetIP() const {
		return m_ip;
	}
	short GetPort() const {
		return m_port;
	}

	int size() const { return sizeof(sockaddr_in); }
private:
	sockaddr_in m_addr;
	std::string m_ip;
	short m_port;
};

class CBuffer : public std::string
{
public:
	CBuffer(const char* str) {
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}

	CBuffer(size_t size) : std::string() {
		if (size > 0) {
			resize(size);
			memset(*this, 0, this->size()); //第三个参数需要明确指明
		}
	}

	CBuffer(void* buffer, size_t size) : std::string() {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}

	~CBuffer() {
		std::string::~basic_string();
	}

	operator char* () const { return (char*)c_str(); }
	operator const char* () const { return c_str(); }
	operator BYTE* () const { return (BYTE*)c_str(); }
	operator void* () const { return (void*)c_str(); }
	void Update(void* buffer, size_t size) {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}

};

class CUSocket
{
public:
	CUSocket(PTYPE nType = PTYPE::UTypeTCP, int nProtocol = 0) {
		m_socket = socket(PF_INET, (int)nType, nProtocol);
		m_type = nType;
		m_protocol = nProtocol;
	}

	CUSocket(const CUSocket& sock) {
		/*
			socket只是一个句柄，复制不能进行真正的复制，可以用shared_ptr管理socket,复制时复制指针
			或者创建一个同参数的socket
		*/
		m_socket = socket(PF_INET, (int)sock.m_type, m_protocol);
		m_type = sock.m_type;
		m_protocol = sock.m_protocol;
	}

	~CUSocket(){
		closesocket(m_socket);
	}

	CUSocket& operator=(const CUSocket& sock) {
		if (this != &sock) {
			m_socket = socket(PF_INET, (int)sock.m_type, m_protocol);
			m_type = sock.m_type;
			m_protocol = sock.m_protocol;
		}
		return *this;
	}

	operator SOCKET() const { return m_socket; };
	operator SOCKET() { return m_socket; };
	bool operator==(SOCKET sock) const { //有const后缀可以让常量也能用
		return m_socket == sock;
	}

	int listen(int backlog = 5) {
		if(m_type != PTYPE::UTypeTCP) return -1;
		return ::listen(m_socket, backlog);
	}

	int bind(const std::string& ip, short port) {
		return ::bind(m_socket, m_addr, m_addr.size());
	}

	int accept() {}

	int connect(const std::string& ip, short port) {

	}

	//int send(char* buffer, size_t size) {}
	int send(const CBuffer& buffer) {
		return ::send(m_socket, buffer, buffer.size(), 0);
	}

	int recv(CBuffer& buffer) {
		return ::recv(m_socket, buffer, buffer.size(), 0);
	}
	int sendto(const CBuffer& buffer, const CUSockaddrIn& to) {
		return ::sendto(m_socket, buffer, buffer.size(), 0, to, to.size());
		/*
		::sendto 显式指定了调用全局的 sendto 函数，而不依赖于当前作用域的任何可能的 sendto 函数重载。
		*/
	}

	int recvfrom(CBuffer& buffer, CUSockaddrIn& from) {
		int len = from.size();
		int ret = ::recvfrom(m_socket, buffer, buffer.size(), 0, from, &len);
		if (ret > 0) {
			from.update();
		}
		return ret;
	}

private:
	SOCKET m_socket;
	PTYPE m_type;
	int m_protocol;
	CUSockaddrIn m_addr;
};

typedef std::shared_ptr<CUSocket> CUSOCKT;

