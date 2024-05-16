#pragma once

class Cutils
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

    static bool IsAdmin() {
        HANDLE hToken = NULL;
        //handle��һ��voidָ�룬һ���������û��㣬һ��������os���û���handle���ε������ݵĽṹ
        //�����������͵�����������ʹ�ù���ʱҪ�þ�������ýӿڣ���֤�Լ��ļ���ϸ��
        //Q:���ʵ����handle���ö���
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            //���󼰴���
            ShowError();
            return false;
        }
        //�õ���token�������ж�
        TOKEN_ELEVATION eve;
        DWORD len = 0;
        if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) {
            ShowError();
            return false;
        }
        return eve.TokenIsElevated;
        if (len == sizeof(eve)) {
            //�鿴�Ƿ��Ѿ��ɹ���Ȩ
            CloseHandle(hToken);

        }
        else {
            //��������ֵ����ʾ����
            printf("length of tokeninfomation is %d\r\n", len);
            return false;
        }
    }

    static bool RunAsAdmin() {
        //Winרҵ�棺���ز����飺����Administrator�˺ţ���ֹ������ֻ�ܵ�¼������̨
        STARTUPINFO si = { 0 };
        PROCESS_INFORMATION pi = { 0 };
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
        BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath,
            CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi); //��¼�ķ�ʽ��������

        if (!ret) {
            ShowError();
            MessageBox(NULL, _T("��������ʧ��"), _T("�������"), 0);
            ::exit(0);
        }
        WaitForSingleObject(pi.hProcess, INFINITE); //�ȴ�handle
        CloseHandle(pi.hProcess); //����������ɺ�ر�
        CloseHandle(pi.hThread);
        return true;
    }

    static void ShowError()
    {
        LPWSTR lpMessageBuf = NULL;
        //strerror(errno); //��׼C��Ĵ���
        //Ӧ�ԷǱ�׼��Ĵ���
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&lpMessageBuf, 0, NULL
        );
        OutputDebugString(lpMessageBuf);
        MessageBox(NULL, lpMessageBuf, _T("��������"), 0);
        LocalFree(lpMessageBuf); //�ͷ�ϵͳ������ڴ�
    }

    static BOOL WriteStartupDir(const CString& strPath) {

        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
        BOOL ret = CopyFile(sPath, strPath, FALSE); //FALSE���ļ����ھ�ֱ�Ӹ���
        return ret;
    }

    static bool WriteRegisterTable(const CString& strPath) {
        /*
            ͨ���޸�ע�����ʵ�ֿ�������
        */
        CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
        char sPath[MAX_PATH] = "";
        char sSys[MAX_PATH] = "";
        std::string strExe = "\\RemoteControl.exe "; //ע�����Ҫ�ӿո�, �ļ�����Ҫ�пո�

        GetCurrentDirectoryA(MAX_PATH, sPath);
        GetSystemDirectoryA(sSys, sizeof(sSys));
        std::string strCmd = "mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe;
        int ret = system(strCmd.c_str()); //ִ���������һ�������� 1.�������⣺ִ�в��ɹ���������ֵ����ȷ�� 

        TRACE("ret = %d\r\n", ret);
        //����ע���
        HKEY hKey = NULL;
        ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);//KEY_WOW64_64KEY�Ǳ�Ҫ��
        if (ret != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            MessageBox(NULL, _T("�����Զ���������ʧ��!\r\n��������ʧ��"),
                _T("����"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
        /*
            �п���ִ�д�����sys32�У���������ȴ��sysWOW64�У����º����������
        */
        //CString strPath = CString(_T("%SystemRoot%\\system32\\Remote Control.exe")); //�������Ҳ���
        //CString strPath = CString(_T("%SystemRoot%\\SysWOW64\\RemoteControl.exe"));
        RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
        if (ret != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            MessageBox(NULL, _T("�����Զ���������ʧ��!\r\n��������ʧ��"),
                _T("����"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
        RegCloseKey(hKey);
        return true;
    }

    static bool Init() //���ڴ�MFC��������Ŀ�ĳ�ʼ��(ͨ��)
    {
        HMODULE hModule = ::GetModuleHandle(nullptr);
        if (hModule == nullptr) {
            wprintf(L"����: GetModuleHandle ʧ��\n");
            return false;
        }
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
            wprintf(L"����: MFC ��ʼ��ʧ��\n");
            return false;
        }
        return true;
    }
};

