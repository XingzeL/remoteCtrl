#pragma once
#include "pch.h"
#include "framework.h"

//#define _CRT_SECURE_NO_WARNINGS
#pragma pack(push)
#pragma pack(1)
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

	//������ع�
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear(); //�����Ͳ������forѭ��
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

	CPacket(const BYTE* pData, size_t& nSize) { //�����nSize��һ�����ã��ܹ����ؽ���˶�������
		//���ڽ������ݵĹ��캯��
		size_t i;
		for (i = 0; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2; //���ֻ�а�ͷ����û�ж�����������Խ���return�ķ�֧
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) { //�еİ�û�����ݣ�����һ�����Ҳ���Խ��н���
			//û�а�ͷ������ֻ�а�ͷ(���ݲ�ȫ)
			nSize = 0;
			return;
		}
		//˵���а�ͷ�Һ���������
		nLength = *(DWORD*)(pData + i); i += 4; //ȡ�����ĳ���
		if (nLength + i > nSize) {
			//˵����û����ȫ���յ����ͷ��أ�����ʧ��
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
		//����У��
		WORD sum = 0;
		for (int j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i; //head2 length4 data...  i����ʵ�ʴӻ��������˵�size
			return;
		}
		//����ʧ�ܵ����
		nSize = 0;
		return;
	}

	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) { //������������
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}

	int Size() { //�����ݵĴ�С
		return nLength + 6;
	}

	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		//������
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)(pData) = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

	~CPacket() {}
public:
	WORD sHead; //��ͷ�ⲿ��Ҫʹ�ã� �̶�ΪFEFF
	DWORD nLength;
	WORD sCmd; //Զ������
	std::string strData; //������
	WORD sSum; //��У��
	std::string strOut; //������������
};
#pragma pack(pop)