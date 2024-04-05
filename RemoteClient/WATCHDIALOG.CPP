#include "pch.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"

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
