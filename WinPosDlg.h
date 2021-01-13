
// WinPosDlg.h : ヘッダー ファイル
//

#pragma once


// CWinPosDlg ダイアログ
class CWinPosDlg : public CDialogEx
{
// コンストラクション
public:
CWinPosDlg(CWnd *pParent = nullptr);		// 標準コンストラクター

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
enum { IDD = IDD_WINPOS_DIALOG };
#endif

protected:
virtual void DoDataExchange(CDataExchange *pDX);		// DDX/DDV サポート


// 実装
protected:
HICON m_hIcon;

// 生成された、メッセージ割り当て関数
virtual BOOL	OnInitDialog();
afx_msg void	OnPaint();
afx_msg HCURSOR OnQueryDragIcon();
DECLARE_MESSAGE_MAP()

// 以下、変更分
public:
afx_msg void OnBnClickedRenew();
afx_msg void OnBnClickedCancel();
//afx_msg int	 OnCreate(LPCREATESTRUCT lpCreateStruct);
afx_msg void OnBnClickedButton1();
afx_msg void OnBnClickedButton2();
afx_msg void OnClose();
afx_msg void OnBnClickedButtonUp();
afx_msg void OnBnClickedButtonDown();
afx_msg void OnBnClickedButtonLeft();
afx_msg void OnBnClickedButtonRight();

private:
void SetWindowPos(int xMode, int yMode, bool isWorkRect);

};
