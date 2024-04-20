﻿#pragma once
#include "afxdialogex.h"


// CWatchDialog 对话框

class CWatchDialog : public CDialog
{
	DECLARE_DYNAMIC(CWatchDialog)

public:
	CWatchDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchDialog();
	bool isFull() const { //不会修改任何成员，内部改变的话报错
		return m_isFull;
	}
	void SetImageStatus(bool isFull = false) {
		m_isFull = isFull;
	}
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_WATCH };
#endif
public:
	int m_nObjWidth;
	int m_nObjHeight;
	bool m_isFull;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持



	DECLARE_MESSAGE_MAP()
public:
	CPoint UserPoint2RemoteScreenPoint(CPoint& point, bool isScree); //isScreen:是否是屏幕的坐标
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CStatic m_picture;  //显示图像的上下文
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnStnClickedWatch();
	virtual void OnOK();
	afx_msg void OnBnClickedLock();
	afx_msg void OnBnClickedUnlock();
};
