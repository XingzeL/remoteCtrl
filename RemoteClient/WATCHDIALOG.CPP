#include "pch.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"

CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen = false)
{
	//CPoint cur = point;
	CRect clientRect;
	if (isScreen) ScreenToClient(&point); //全局坐标到客户区域坐标
	m_picture.GetWindowRect(clientRect); 
	//远程端的坐标转换：等比缩放
	int width0 = clientRect.Width();
	int height0 = clientRect.Height();
	int width = 1920, height = 1080;

	int x = point.x * width / width0; //等比缩放
	int y = point.y * height / height0;

	return CPoint(x, y);
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 20, NULL); 
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0) {
		CRemoteClientDlg* nParent = (CRemoteClientDlg*)GetParent();
		if (nParent->isFull()) {
			//应该得到WATCH的DC，也就是显示内容的部分
			CRect rect;
			m_picture.GetWindowRect(rect);

			//nParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY);
			nParent->GetImage().StretchBlt(
				m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			nParent->GetImage().Destroy(); //对CImage的操作
			nParent->SetImageStatus(false); //已读取缓冲中的内容，可以读了
		}
	}
	CDialog::OnTimer(nIDEvent);
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

void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 0; //左键
	event.nAction = 2; //双击
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //得到父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event); 
	//是一种让步：只能发消息委托主窗口发送包，因为初始化socket的功能放在了主窗口类CRemoteClientDlg中，这是一种耦合
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	TRACE("x = %d y = %d\r\n", point.x, point.y);
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	TRACE("x = %d y = %d\r\n", point.x, point.y);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 0; //左键
	event.nAction = 2; //按下
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //得到父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);

	CDialog::OnLButtonDown(nFlags, point);
}



void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 0; //左键
	event.nAction = 3; //弹起
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //得到父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 1; //右键
	event.nAction = 1; //双击
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //得到父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	TRACE("x = %d y = %d\r\n", point.x, point.y);
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	TRACE("x = %d y = %d\r\n", point.x, point.y);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 1; //右键
	event.nAction = 2; //按下 TODO：服务端要做对应的修改
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //得到父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 1; //右键
	event.nAction = 3; //弹起
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //得到父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)  //在类视图中添加的信号处理函数，得到的point坐标就是
{
	CPoint remote = UserPoint2RemoteScreenPoint(point, false);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 8; //移动不考虑鼠标键
	event.nAction = 0; //移动

	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	//一开始没有进入这里
	CPoint point;
	GetCursorPos(&point);
	CPoint remote = UserPoint2RemoteScreenPoint(point, true); //用MFC UI中加入点击的事件处理函数时，point坐标就是全局的坐标，需要进行转换
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 0; //左键
	event.nAction = 0; //单击
	CClientSocket* pclient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pclient->Send(pack);
}
