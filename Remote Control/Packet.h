#pragma once
#include "pch.h"
#include "framework.h"

//#define _CRT_SECURE_NO_WARNINGS
#pragma pack(push)
#pragma pack(1)
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

	//打包的重构
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear(); //这样就不会进入for循环
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}

	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	CPacket(const BYTE* pData, size_t& nSize) { //这里的nSize是一个引用，能够返回解包了多少数据
		//用于解析数据的构造函数
		size_t i;
		for (i = 0; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2; //如果只有包头后面没有东西的情况可以进入return的分支
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) { //有的包没有数据，就是一个命令，也可以进行解析
			//没有包头，或者只有包头(数据不全)
			nSize = 0;
			return;
		}
		//说明有包头且后面有数据
		nLength = *(DWORD*)(pData + i); i += 4; //取到包的长度
		if (nLength + i > nSize) {
			//说明包没有完全接收到，就返回，解析失败
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		//进行校验
		WORD sum = 0;
		for (int j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i; //head2 length4 data...  i就是实际从缓冲区读了的size
			return;
		}
		//解析失败的情况
		nSize = 0;
		return;
	}

	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) { //这样可以连等
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}

	int Size() { //包数据的大小
		return nLength + 6;
	}

	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		//填数据
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)(pData) = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

	~CPacket() {}
public:
	WORD sHead; //包头外部需要使用， 固定为FEFF
	DWORD nLength;
	WORD sCmd; //远控命令
	std::string strData; //包数据
	WORD sSum; //和校验
	std::string strOut; //整个包的数据
};
#pragma pack(pop)