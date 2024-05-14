#include "pch.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"
#include "ClientController.h"

CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen = false)
{
	//CPoint cur = point;
	CRect clientRect;
	if (isScreen) ScreenToClient(&point); //全局坐标到客户区域坐标
	else {  //如果不是占满
		ClientToScreen(&point); //转换为相对屏幕左上角的坐标(屏幕内的绝对坐标)
		m_picture.ScreenToClient(&point); //转换成客户区域的坐标(相对于picture控件左上角的坐标
	}
	m_picture.GetWindowRect(clientRect); 
	//远程端的坐标转换：等比缩放
	int width0 = clientRect.Width();
	int height0 = clientRect.Height();
	int width = m_nObjWidth, height = m_nObjHeight; //还是应该动态调整，根据对方的屏幕大小

	int x = point.x * width / width0; //等比缩放
	int y = point.y * height / height0;

	return CPoint(x, y);
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_isFull = false;
	//SetTimer(0, 20, NULL); //设置定时器
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent) //定时取得图像的功能函数
{

}

//鼠标事件：
//typedef struct MouseEvent {
//	MouseEvent() {
//		nAction = 0;
//		nButton = -1; //先是没有效果
//		ptXY.x = 0;
//		ptXY.y = 0;
//	}
//	WORD nAction; //点击，移动，双击
//	WORD nButton; //左键，右键，中键
//	POINT ptXY; //坐标
//}MOUSEEV, * PMOUSEEV;

LRESULT CWatchDialog::OnSendPakcetAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2)) {
		//TODO: 错误处理
	}
	else if (lParam == 1) {
		//对方关闭了套接字
	}
	else 
	{
		CPacket* pPacket = (CPacket*)wParam;
		if (pPacket != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			switch(head.sCmd) {
			case 6:
			{
				//屏幕的包，要进行显示
				Cutils::Bytes2Image(m_image, pPacket->strData);
				CRect rect;
				m_picture.GetWindowRect(rect);

				//pController->GetImage(m_image);
				if (m_nObjWidth == -1) {
					m_nObjWidth = m_image.GetWidth();
				}

				if (m_nObjHeight == -1) {
					m_nObjHeight = m_image.GetHeight();
				}
				//nParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY);
				m_image.StretchBlt(
					m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
				m_picture.InvalidateRect(NULL);
				m_image.Destroy(); //对CImage的操作
				break;
			}
			case 5: 
				TRACE("远程应答了鼠标操作");
				break;
			case 7: //锁屏
				break;
			case 8:
				break;
			default:
				break;
			}
		}
	}
	return 0;
}

void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 0; //左键
		event.nAction = 2; //双击
		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //得到父窗口
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd() /*拿到当前句柄*/, 5, true, (BYTE*)&event, sizeof(event));
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	}
//是一种让步：只能发消息委托主窗口发送包，因为初始化socket的功能放在了主窗口类CRemoteClientDlg中，这是一种耦合
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1) {
		TRACE("x = %d y = %d\r\n", point.x, point.y);
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		TRACE("x = %d y = %d\r\n", point.x, point.y);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 0; //左键
		event.nAction = 2; //按下
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //得到父窗口
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd() /*拿到当前句柄*/, 5, true, (BYTE*)&event, sizeof(event));
	}

	CDialog::OnLButtonDown(nFlags, point);
}



void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 0; //左键
		event.nAction = 3; //弹起
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //得到父窗口
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd() /*拿到当前句柄*/, 5, true, (BYTE*)&event, sizeof(event));

	}

	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 1; //右键
		event.nAction = 1; //双击
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //得到父窗口
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd() /*拿到当前句柄*/, 5, true, (BYTE*)&event, sizeof(event));

	}

	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1) {
		TRACE("x = %d y = %d\r\n", point.x, point.y);
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		TRACE("x = %d y = %d\r\n", point.x, point.y);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 1; //右键
		event.nAction = 2; //按下 TODO：服务端要做对应的修改
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //得到父窗口
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd() /*拿到当前句柄*/, 5, true, (BYTE*)&event, sizeof(event));
	}

	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 1; //右键
		event.nAction = 3; //弹起
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //得到父窗口
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd() /*拿到当前句柄*/, 5, true, (BYTE*)&event, sizeof(event));
	}

	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)  //在类视图中添加的信号处理函数，得到的point坐标就是
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		CPoint remote = UserPoint2RemoteScreenPoint(point, false);
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 8; //移动不考虑鼠标键
		event.nAction = 0; //移动
		//CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event)); //在本机上暂时注释
	}

	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	//一开始没有进入这里
	if (m_nObjHeight != -1 && m_nObjWidth != -1) {
		CPoint point;
		GetCursorPos(&point);
		CPoint remote = UserPoint2RemoteScreenPoint(point, true); //用MFC UI中加入点击的事件处理函数时，point坐标就是全局的坐标，需要进行转换
		//封装
		MouseEvent event;
		event.ptXY = remote;
		event.nButton = 0; //左键
		event.nAction = 0; //单击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd() /*拿到当前句柄*/, 5, true, (BYTE*)&event, sizeof(event));
	}

}


void CWatchDialog::OnOK()
{

}


void CWatchDialog::OnBnClickedLock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd() /*拿到当前句柄*/, 7);
}


void CWatchDialog::OnBnClickedUnlock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd() /*拿到当前句柄*/, 8);
}
