#include "pch.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 50, NULL); 
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CRemoteClientDlg* nParent = (CRemoteClientDlg*)GetParent();
		if (nParent->isFull()) {
			//TODO:
		}
	}
	CDialog::OnTimer(nIDEvent);
}
