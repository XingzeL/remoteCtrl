#pragma once
#include <Windows.h>
#include <string>
#include <atlimage.h>


class Cutils //�������Ե���Ŀ
{
public:
    static void Dump(BYTE* pData, size_t nSize) {
        std::string strOut;
        for (size_t i = 0; i < nSize; i++) {
            char buf[8] = "";
            if (i > 0 && (i % 16 == 0)) strOut += "\n";
            snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF);
            strOut += buf;
        }
        strOut += "\n";
        OutputDebugStringA(strOut.c_str());
    }

    static int Bytes2Image(CImage& image, const std::string& strBuffer)
    { //��һ�ξ��ǵ�������������ת����������װ
		//����m_image������
		BYTE* pData = (BYTE*)strBuffer.c_str();  //server����ͼƬ�Ƿŵ�һ������
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);  // �����ڴ�
		if (hMem == NULL) {
			TRACE("�ڴ治��");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL;  // ����һ��������ָ��
		// ����һ�������ڴ��������
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (hRet == S_OK) {
			ULONG length = 0;
			// ����������д������
			pStream->Write(pData, strBuffer.size(), &length);
			LARGE_INTEGER bg = { 0 }; // ����һ����������������ָ�����е�λ��
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);// �����е�λ������Ϊ��ͷ
			if ((HBITMAP)image != NULL)
				image.Destroy();

			image.Load(pStream); // �����м���ͼ�� m_image ������
		}
		return hRet;
    }
};

