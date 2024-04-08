#include "pch.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"

CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point)
{
	//CPoint cur = point;
	CRect clientRect;
	ScreenToClient(&point); //全局坐标到客户区域坐标
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
	CClientSocket* pclient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pclient->Send(pack);
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{

	CDialog::OnLButtonDown(nFlags, point);
}



void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	//CPoint remote = UserPoint2RemoteScreenPoint(point);
	////封装
	//MouseEvent event;
	//event.ptXY = remote;
	//event.nButton = 0; //左键
	//event.nAction = 2; //双击
	//CClientSocket* pclient = CClientSocket::getInstance();
	//CPacket pack(5, (BYTE*)&event, sizeof(event));
	//pclient->Send(pack);

	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 2; //右键
	event.nAction = 2; //双击
	CClientSocket* pclient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pclient->Send(pack);

	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 0; //左键
	event.nAction = 3; //双击 TODO：服务端要做对应的修改
	CClientSocket* pclient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pclient->Send(pack);

	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 0; //左键
	event.nAction = 4; //弹起
	CClientSocket* pclient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pclient->Send(pack);

	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 0; //移动不考虑鼠标键
	event.nAction = 1; //移动
	CClientSocket* pclient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pclient->Send(pack);
	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	CPoint point;
	GetCursorPos(&point);
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MouseEvent event;
	event.ptXY = remote;
	event.nButton = 0; //左键
	event.nAction = 3; //按下
	CClientSocket* pclient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pclient->Send(pack);
}
