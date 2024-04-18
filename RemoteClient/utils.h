#pragma once
#include <Windows.h>
#include <string>
#include <atlimage.h>


class Cutils //纯功能性的项目
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
    { //这一段就是单纯的数据类型转换，单独封装
		//存入m_image缓冲中
		BYTE* pData = (BYTE*)strBuffer.c_str();  //server发送图片是放到一个包中
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);  // 分配内存
		if (hMem == NULL) {
			TRACE("内存不足");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL;  // 定义一个流对象指针
		// 创建一个基于内存的流对象
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (hRet == S_OK) {
			ULONG length = 0;
			// 向流对象中写入数据
			pStream->Write(pData, strBuffer.size(), &length);
			LARGE_INTEGER bg = { 0 }; // 定义一个大整数对象，用于指定流中的位置
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);// 将流中的位置设置为开头
			if ((HBITMAP)image != NULL)
				image.Destroy();

			image.Load(pStream); // 从流中加载图像到 m_image 对象中
		}
		return hRet;
    }
};

