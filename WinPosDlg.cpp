
// WinPosDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "framework.h"
#include "WinPos.h"
#include "WinPosDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/*----------------------------------------------------------------------------------------------*/
#define IS_NO_MOVE (65536)							// アプリ配置関数で位置が変わらないことを示す


/***********************************************************************************************
 ***********************************************************************************************
 *
 *		ディスプレイ情報を得る
 *
 ***********************************************************************************************
 ************************************************************************************************/

/*----------------------------------------------------------------------------------------------*/

/*------ ディスプレイ情報 */
#define FULLSCREEN_MAX (256)						// フルスクリーンの解像度の組み合わせの最大数
#define DISP_MAX	   (16)							// ディスプレイの最大数

/* ディスプレイ設定 */
typedef struct _DISP_MODE {
	DWORD dmPelsWidth;								// 幅
	DWORD dmPelsHeight;								// 高さ
	DWORD dmDisplayFrequency;						// 周波数（Hz）
} DISP_MODE;

/* ディスプレイデバイス１つの情報 */
typedef struct _DISP_1 {
	HMONITOR	   hMonitor;							// モニタのハンドル  （GetDispInfoHalf() で設定）
	RECT		   rect;								// モニタの座標の矩形（GetDispInfoHalf() で設定）
	RECT		   workRect;							// 作業領域の座標の矩形（GetDispInfoHalf() で設定）
	DISPLAY_DEVICE dispDevice;							// ディスプレイデバイスの情報（主に文字列）
	DISP_MODE	   dispMode;							// ディスプレイ設定
	int			   fullscreenNum;						// フルスクリーンでの、ディスプレイ設定の個数
	DISP_MODE	   fullscreenArray[FULLSCREEN_MAX];		// フルスクリーンでの、ディスプレイ設定の配列
} DISP_1;

/* ディスプレイデバイス全体の情報 */
typedef struct _DISP_INFO {
	int		 dispNum;								// ディスプレイデバイスの個数（0 も有り得る）
	DISP_1	 dispArray[DISP_MAX];					// ディスプレイデバイス情報の配列
	HMONITOR hMonitorSaveArray[DISP_MAX];			// 各ディスプレイのモニターのハンドルを保存しておくバッファ

	int		 monitorID;								// ウィンドウが対応するディスプレイモニタの番号
} DISP_INFO;


/*------ ディスプレイデバイスの情報 */
static DISP_INFO dispInfo;

/*----------------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------------
 * 解説：	ディスプレイデバイスの以下の情報を得る
 *			dispInfo.dispNum
 *			各 DISP_1.hMonitor
 *			各 DISP_1.rect
 *
 * 戻り値：	なし
 *----------------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------------*/
BOOL CALLBACK MonitorEnumProc(
	HMONITOR hMonitor,				// ディスプレイモニタのハンドル
	HDC		 hdcMonitor,			// モニタに適したデバイスコンテキストのハンドル
	LPRECT	 lprcMonitor,			// モニタ上の交差部分を表す長方形領域へのポインタ
	LPARAM	 dwData					// EnumDisplayMonitors から渡されたデータ
	)
{
	DISP_1		*pDispArray;
	MONITORINFO monitorInfo;

	if (DISP_MAX <= dispInfo.dispNum) {
		return false;
	}

	pDispArray			 = &dispInfo.dispArray[dispInfo.dispNum];
	pDispArray->hMonitor = hMonitor;					// モニタのハンドルを保存
	pDispArray->rect	 = *lprcMonitor;				// RECT を保存

	monitorInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMonitor, &monitorInfo);
	pDispArray->workRect = monitorInfo.rcWork;			// 作業領域の RECT を保存

#ifdef DEBUG_DISPLAY
//	PRINTF( "display %d: (%d, %d)-(%d, %d)",
//			dispInfo.dispNum,
//			lprcMonitor->left, lprcMonitor->top, lprcMonitor->right, lprcMonitor->bottom );
#endif
	dispInfo.dispNum++;
	return true;
}
/*----------------------------------------------------------------------------------------------*/
static void GetDispInfoHalf(void)
{
	dispInfo.dispNum = 0;
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
}

/*----------------------------------------------------------------------------------------------
 * 解説：	ディスプレイデバイスの残りの情報を得る
 *
 * 戻り値：	なし
 *----------------------------------------------------------------------------------------------*/
static void GetDispInfo(void)
{
	DISP_1 *pDispArray;
	int	   dispCheckIndex;				// 存在を確認するディスプレイモニタの番号
	int	   dispNum;
	bool   res;

	pDispArray	   = dispInfo.dispArray;
	dispCheckIndex = 0;
	dispNum		   = 0;

	while (1) {
		DEVMODE	  devMode;
		DISP_MODE *pFullscreenArrayTop;
		DISP_MODE *pFullscreenArray;
		int		  fullscreenCheckIndex;
		int		  fullscreenNum;

		memset(&pDispArray->dispDevice, 0, sizeof(DISPLAY_DEVICE));
		pDispArray->dispDevice.cb = sizeof(DISPLAY_DEVICE);			// cb メンバを DISPLAY_DEVICE のサイズで初期化

		/* DISPLAY_DEVICE を得る */
		res = EnumDisplayDevices(nullptr, dispCheckIndex, &pDispArray->dispDevice, 0);
		if (!res) {
			break;
		}
		dispCheckIndex++;

		if (!(pDispArray->dispDevice.StateFlags & DISPLAY_DEVICE_ACTIVE)) {		// アクティブか？
			continue;
		}

		/* 現在のディスプレイ設定（DEVMODE）を得る */
		memset(&devMode, 0, sizeof(DEVMODE));
		devMode.dmSize		  = sizeof(DEVMODE);			// dmDize メンバを DEVMODE のサイズで初期化
		devMode.dmDriverExtra = 0;							// デバイス固有の情報のバイト数
		EnumDisplaySettingsEx(pDispArray->dispDevice.DeviceName, ENUM_CURRENT_SETTINGS, &devMode, 0);

		pDispArray->dispMode.dmPelsWidth		= devMode.dmPelsWidth;
		pDispArray->dispMode.dmPelsHeight		= devMode.dmPelsHeight;
		pDispArray->dispMode.dmDisplayFrequency = devMode.dmDisplayFrequency;

#ifdef DEBUG_DISPLAY
		PRINTF("\n-------- DISPLAY %d: %s %s --------",
			   dispNum, pDispArray->dispDevice.DeviceName, pDispArray->dispDevice.DeviceString);
#endif

		/* 以下、フルスクリーンでの、ディスプレイ設定 DISP_MODE を保存 */
		pFullscreenArrayTop	 = \
			pFullscreenArray = pDispArray->fullscreenArray;
		fullscreenCheckIndex = 0;
		fullscreenNum		 = 0;

		while (1) {
			bool isSame;
			int	 j;

			/* DEVMODE を得る */
			memset(&devMode, 0, sizeof(DEVMODE));
			devMode.dmSize		  = sizeof(DEVMODE);		// dmDize メンバを DEVMODE のサイズで初期化
			devMode.dmDriverExtra = 0;						// デバイス固有の情報のバイト数
			res					  = EnumDisplaySettingsEx(pDispArray->dispDevice.DeviceName, fullscreenCheckIndex, &devMode, 0);
			if (!res) {
				break;
			}
			fullscreenCheckIndex++;

			/* 既に同じ DEVMODE の情報があった場合は省く */
			isSame = false;
			for (j = 0; j < fullscreenNum; j++) {
				if (devMode.dmPelsWidth        == pFullscreenArrayTop[j].dmPelsWidth &&
					devMode.dmPelsHeight       == pFullscreenArrayTop[j].dmPelsHeight &&
					devMode.dmDisplayFrequency == pFullscreenArrayTop[j].dmDisplayFrequency) {
					isSame = true;
					break;
				}
			}
			if (isSame) {
				continue;
			}

			/* DISP_MODE を設定 */
			pFullscreenArray->dmPelsWidth		 = devMode.dmPelsWidth;
			pFullscreenArray->dmPelsHeight		 = devMode.dmPelsHeight;
			pFullscreenArray->dmDisplayFrequency = devMode.dmDisplayFrequency;

#ifdef DEBUG_DISPLAY
			PRINTF("  %dx%d %dHz",
				   pFullscreenArray->dmPelsWidth,
				   pFullscreenArray->dmPelsHeight,
				   pFullscreenArray->dmDisplayFrequency);
#endif
			pFullscreenArray++;
			fullscreenNum++;
			if (FULLSCREEN_MAX <= fullscreenNum) {
				break;
			}
		}

		pDispArray->fullscreenNum = fullscreenNum;

		pDispArray++;
		dispNum++;
		if (DISP_MAX <= dispNum) {
			break;
		}
	}

#ifdef DEBUG_DISPLAY
	PRINTF("MONITOR NUM: %d\n", dispNum);
#endif

#if 0
	/* ２種類の関数で得られるディスプレイの数が異なる？（エラー） */
	if (dispInfo.dispNum != dispNum) {
		PRINTF_MSG("Number of DisplayMonitor is failed.");
	}
#endif
	dispInfo.dispNum = dispNum;
}

/*----------------------------------------------------------------------------------------------
 * 解説：	指定のディスプレイ解像度があるか？をチェックする
 *
 * 戻り値：	true or false
 *----------------------------------------------------------------------------------------------*/
static bool IsDispMode(
	int checkDispID,					// チェックするディスプレイ番号
	int checkWidth,						// チェックする画面解像度の幅
	int checkHeight,					// チェックする画面解像度の高さ
	int checkFrequency					// チェックする周波数
	)
{
	DISP_1	  *pDispArray;
	DISP_MODE *pFullscreenArray;
	int		  i;

	if (dispInfo.dispNum <= 0) {			// そもそもディスプレイがない
		return false;
	}
	if (dispInfo.dispNum <= checkDispID) {	// ディスプレイ番号が範囲外
		return false;
	}

	pDispArray		 = &dispInfo.dispArray[checkDispID];
	pFullscreenArray = pDispArray->fullscreenArray;
	for (i = pDispArray->fullscreenNum; 0 < i; i--, pFullscreenArray++) {

		if (checkWidth     == pFullscreenArray->dmPelsWidth &&
			checkHeight    == pFullscreenArray->dmPelsHeight &&
			checkFrequency == pFullscreenArray->dmDisplayFrequency) {
			return true;
		}
	}
	return false;
}

/*----------------------------------------------------------------------------------------------
 * 解説：	現在のウィンドウの位置（pMain->windowRect）からディスプレイの番号を得る
 *
 * 戻り値：	ディスプレイの番号
 *----------------------------------------------------------------------------------------------*/
static int GetDispID(
	RECT *pRect							// Window の RECT
	)
{
	DISP_1 *pDispArray;
	int	   inCount;						// ディスプレイの矩形に入った回数
	int	   inDisplayID;
	int	   i;

	if (dispInfo.dispNum <= 0) {		// ディスプレイがない
		return 0;
	}

	inCount		= 0;
	inDisplayID = 0;

	pDispArray = dispInfo.dispArray;
	for (i = 0; i < dispInfo.dispNum; i++, pDispArray++) {

		if (pDispArray->rect.left < pRect->right  &&
			pDispArray->rect.top  < pRect->bottom &&
			pRect->left < pDispArray->rect.right  &&
			pRect->top  < pDispArray->rect.bottom) {

			if (0 == inCount) {
				inDisplayID = i;
			}
			inCount++;
		}
	}

	if (0 != inCount) {		//------ ディスプレイのどれかにウィンドウがある
#ifdef  DEBUG_DISPLAY
		PRINTF("Set Display ID: %d", inDisplayID);
#endif
		return inDisplayID;

	} else {				//------ その他
		return 0;
	}
}

/*----------------------------------------------------------------------------------------------
 * 解説：	ディスプレイ設定関連の初期設定
 *
 * 戻り値：	なし
 *----------------------------------------------------------------------------------------------*/
static void InitDispInfo(void)
{
	int i;

	memset(&dispInfo, 0, sizeof(dispInfo));
	GetDispInfoHalf();
	GetDispInfo();

	/* dispInfo.hMonitorsavebuf[] にモニタのハンドルを保存 */
	for (i = 0; i < dispInfo.dispNum; i++) {
		dispInfo.hMonitorSaveArray[i] = dispInfo.dispArray[i].hMonitor;
	}
}





/***********************************************************************************************
 ***********************************************************************************************
 *
 *		デスクトップで起動されているアプリケーションの情報を得る
 *
 ***********************************************************************************************
 ************************************************************************************************/

/*----------------------------------------------------------------------*/
#define STR_NAME_MAX (256)			/* ウィンドウ名の最大文字数 */

struct WINDOW_INFO {
	HWND hWnd;							/* ウィンドウのハンドル */
	char name[STR_NAME_MAX + 1];		/* ウィンドウ名（タイトル）*/
};

struct CTRL_WINDOW_INFO {
	int			num;			/* 現在の登録数 */
	int			memNum;			/* メモリ使用数 */
	WINDOW_INFO *pInfo;
};

static CTRL_WINDOW_INFO			  ctrlWindowInfo;
static CTRL_WINDOW_INFO    *const pCtrlWindowInfo = &ctrlWindowInfo;


/*-----------------------------------------------------------------------
*  解説：	Window 情報の初期設定
*
*  戻り値：なし
   -----------------------------------------------------------------------*/
static void InitWindowInfo(void) {
	CTRL_WINDOW_INFO *pCtrl;

	pCtrl		  = pCtrlWindowInfo;
	pCtrl->num	  = 0;
	pCtrl->memNum = 0;
	pCtrl->pInfo  = NULL;
}
/*-----------------------------------------------------------------------
*  解説：	Window 情報の終了処理
*
*  戻り値：なし
   -----------------------------------------------------------------------*/
static void ReleaseWindowInfo(void) {
	CTRL_WINDOW_INFO *pCtrl;

	pCtrl = pCtrlWindowInfo;
	if (NULL != pCtrl->pInfo) {
		::free(pCtrl->pInfo);
	}
	pCtrl->pInfo = NULL;
}

/*-----------------------------------------------------------------------
*  解説：	Window 情報の追加
*
*  戻り値：なし
   -----------------------------------------------------------------------*/
static void AddWindowInfo(
	HWND hWnd					/* ウィンドウのハンドル */
	)
{
	CTRL_WINDOW_INFO *pCtrl;
	WINDOW_INFO		 *pInfo;

	if (FALSE == ::IsWindow(hWnd)) {			/* ハンドルが有効？ */
		return;
	}
	if (TRUE == ::IsIconic(hWnd)) {				/* ウィンドウが最小化？ */
		return;
	}
	if (TRUE == ::IsZoomed(hWnd)) {				/* ウィンドウが最大化？ */
		return;
	}
	if (NULL != ::GetParent(hWnd)) {			/* 親ウィンドウが無い？（普通は有る）*/
		return;
	}
	if (FALSE == ::IsWindowVisible(hWnd)) {		/* ウィンドウが表示されている？ */
		return;
	}


#if 0
	{
		HWND hTop;

		hTop = ::GetDesktopWindow();

		if (hTop == ::GetAncestor(hWnd, GA_PARENT)) {
			return;
		}
	}
#endif

#if 1
	{
		WINDOWINFO wInfo;

		::GetWindowInfo(hWnd, &wInfo);

		/* UWP（ストアアプリ）の判定 */
		if (wInfo.dwStyle & WS_POPUP) {							// ポップアップウィンドウ？
			if (!(wInfo.dwExStyle & WS_EX_OVERLAPPEDWINDOW)) {	// オーバーラップしたウィンドウ（通常ウィンドウ）以外？
				return;
			}
		}

	}

#endif

	pCtrl = pCtrlWindowInfo;
	pCtrl->num++;
	if (pCtrl->memNum < pCtrl->num) {
		pCtrl->memNum += 4;
		if (NULL == pCtrl->pInfo) {
			pCtrl->pInfo = (WINDOW_INFO *)::malloc(sizeof(WINDOW_INFO) * pCtrl->memNum);
		} else {
			pCtrl->pInfo = (WINDOW_INFO *)::realloc(pCtrl->pInfo,
													sizeof(WINDOW_INFO) * pCtrl->memNum);
		}
	}
	pInfo		= pCtrl->pInfo + pCtrl->num - 1;
	pInfo->hWnd = hWnd;

	/* ウィンドウのタイトルを得る */
	::GetWindowText(hWnd, (LPWSTR)pInfo->name, STR_NAME_MAX);
	if (pInfo->name[0] == '\0') {			/* 名前が空白 */
		pCtrl->num--;						/* 足してしまった WINDOW_INFO の数を減らす */
		return;
	}

//	OutputDebugString((LPCWSTR)pInfo->name);
//	OutputDebugString((LPCWSTR)"\n");
};


/*-----------------------------------------------------------------------
*  解説：	Window 情報を得る、コールバック
*
*  戻り値：なし
   -----------------------------------------------------------------------*/
static BOOL CALLBACK EnumWindowsProc(
	HWND   hWnd,		/* 親ウィンドウのハンドル */
	LPARAM lParam		/* アプリケーション定義の値 */
	)
{
	AddWindowInfo(hWnd);
	return TRUE;
}
/*-----------------------------------------------------------------------
*  解説：	Window 情報を得る
*
*  戻り値：なし
   -----------------------------------------------------------------------*/
static void GetWindows(void)
{
	ReleaseWindowInfo();
	InitWindowInfo();

	::EnumWindows(EnumWindowsProc, 0);
}





/***********************************************************************************************
 ***********************************************************************************************
 *
 *		ダイアログ本体
 *
 ***********************************************************************************************
 ************************************************************************************************/


// CWinPosDlg ダイアログ

CWinPosDlg::CWinPosDlg(CWnd *pParent /*=nullptr*/)
	: CDialogEx(IDD_WINPOS_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWinPosDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CWinPosDlg, CDialogEx)

ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
/* 以下、追加分 */
ON_BN_CLICKED(IDRenew, &CWinPosDlg::OnBnClickedRenew)
ON_WM_CREATE()
ON_BN_CLICKED(IDC_BUTTON1, &CWinPosDlg::OnBnClickedButton1)
ON_BN_CLICKED(IDC_BUTTON2, &CWinPosDlg::OnBnClickedButton2)
ON_WM_CLOSE()
ON_BN_CLICKED(IDC_BUTTON_UP, &CWinPosDlg::OnBnClickedButtonUp)
ON_BN_CLICKED(IDC_BUTTON_DOWN, &CWinPosDlg::OnBnClickedButtonDown)
ON_BN_CLICKED(IDC_BUTTON_LEFT, &CWinPosDlg::OnBnClickedButtonLeft)
ON_BN_CLICKED(IDC_BUTTON_RIGHT, &CWinPosDlg::OnBnClickedButtonRight)

END_MESSAGE_MAP()


// CWinPosDlg メッセージ ハンドラー

BOOL CWinPosDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。
	InitWindowInfo();
	InitDispInfo();
	OnBnClickedRenew();

	return TRUE;	// フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CWinPosDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);	// 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int	  cxIcon = GetSystemMetrics(SM_CXICON);
		int	  cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CWinPosDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


/*-----------------------------------------------------------------------
 * 「更新」ボタンが押された
 *
 *	注意：イニシャライズ時にも呼ばれる。
 */
void CWinPosDlg::OnBnClickedRenew()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。

	/* 全ウィンドウの情報を得る */
	GetWindows();

	/* リストボックスの更新 */
	{
		CTRL_WINDOW_INFO *pCtrl;
		WINDOW_INFO		 *pInfo;
		CListBox		 *pList;
		int				 i;

		pList = (CListBox *)GetDlgItem(IDC_LIST1);
		pList->ResetContent();				/* 全項目削除 */

		pCtrl = pCtrlWindowInfo;
		pInfo = pCtrl->pInfo;
		for (i = 0; i < pCtrl->num; i++, pInfo++) {
/*			pList->AddString((LPCTSTR)pInfo->name);		なんか並びがおかしくなる */
			pList->InsertString(i, (LPCTSTR)pInfo->name);
		}
	}
}


/*-----------------------------------------------------------------------
 * ウィンドウの配置
 */
void CWinPosDlg::SetWindowPos(
	int	 xMode,			// -1:画面左, 0:画面中央, 1:画面右, IS_NO_MOVE:何もしない
	int	 yMode,			// -1:画面上, 0:画面中央, 1:画面下, IS_NO_MOVE:何もしない
	bool isWorkRect		// true: タスクバーを覗いた画面の作業領域を基準とする
	)
{
	CTRL_WINDOW_INFO *pCtrl;
	WINDOW_INFO		 *pInfo;
	CListBox		 *pList;
	int				 cursol;
	HWND			 hWnd;

	pCtrl = pCtrlWindowInfo;
	pInfo = pCtrl->pInfo;

	pList  = (CListBox *)GetDlgItem(IDC_LIST1);
	cursol = pList->GetCurSel();
	if (LB_ERR == cursol || pCtrl->num <= cursol) {	/* リストの位置が範囲外 */
		return;
	}
	pInfo += cursol;

	hWnd = pInfo->hWnd;
	if (FALSE == IsWindow(hWnd)) {				/* ハンドルが有効？ */
		return;
	}

	{
		int	 topX, topY, topW, topH;
		int	 x, y, w, h;
		int	 x0, y0;
		RECT rect;								/* テンポラリの RECT */
		int	 marginTop;
		int	 marginBottom;
		int	 marginLeft;
		int	 marginRight;

		marginTop = marginBottom = marginLeft = marginRight = 0;

		/* ウィンドウの座標を得る */
#if 0
		::GetWindowRect(hWnd, &rect);
#else
		WINDOWINFO winInfo;
		winInfo.cbSize = sizeof(WINDOWINFO);
		if (false == ::GetWindowInfo(hWnd, &winInfo)) {
			return;								// 関数呼び出しの失敗
		}
		rect		 = winInfo.rcWindow;
		marginTop	 = winInfo.rcClient.top - winInfo.rcWindow.top;
		marginBottom = winInfo.rcWindow.bottom - winInfo.rcClient.bottom;
		marginLeft	 = winInfo.rcClient.left - winInfo.rcWindow.left;
		marginRight	 = winInfo.rcWindow.right - winInfo.rcClient.right;
#endif
		x = rect.left;
		y = rect.top;
		w = rect.right - rect.left;
		h = rect.bottom - rect.top;


		/* デスクトップの座標を得る */
#if 1
		{
			int dispID;

			dispID = GetDispID(&rect);

			if (isWorkRect) {
				rect = dispInfo.dispArray[dispID].workRect;
			} else {
				rect = dispInfo.dispArray[dispID].rect;
			}
			topX = rect.left;
			topY = rect.top;
			topW = rect.right - rect.left;
			topH = rect.bottom - rect.top;
		}
#else
#if 0
		HWND topHWnd;
		topHWnd = ::GetDesktopWindow();
		::GetWindowRect(topHWnd, &rect);
		topX = rect.left;
		topY = rect.top;
		topW = rect.right - rect.left;
		topH = startY - rect.top;				/* タスクバーの幅を反映 */
#else
		::SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&rect, 0);
		topX = rect.left;
		topY = rect.top;
		topW = rect.right - rect.left;
		topH = rect.bottom - rect.top;
#endif
#endif

#if 0
		/* バージョン情報の取得 */
		{
			OSVERSIONINFO OSver;
			OSver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionEx(&OSver);

			/* もし Windows 10 (6.2) 以降のバージョンで動かしたいとき */
			if ((OSver.dwMajorVersion == 6 && OSver.dwMinorVersion >= 2) || OSver.dwMajorVersion > 6) {
				topX -= 7;
				topY -= 0;
				topW += 14;
				topH += 7;
			}
		}
#endif

		/* 移動先の座標を設定 */
		if (IS_NO_MOVE != xMode) {
			if (xMode < 0) {
				x0 = topX - marginLeft + 1;
			} else if (xMode == 0) {
				x0 = topX + (topW - w) / 2;
			} else {
				x0 = topX + topW - w + marginRight - 1;
			}
		} else {
			x0 = x;
		}
		if (IS_NO_MOVE != yMode) {
			if (yMode < 0) {
#if 1
				y0 = topY;
#else
				y0 = topY - marginTop - 1;			// タイトルバーが画面上に隠れてしまう
#endif
			} else if (yMode == 0) {
				y0 = topY + (topH - h) / 2;
			} else {
				y0 = topY + topH - h + marginBottom - 1;
			}
		} else {
			y0 = y;
		}

		::MoveWindow(hWnd, x0, y0, w, h, TRUE);
	}

}

/*-----------------------------------------------------------------------
 * ウィンドウの終了時
 */
void CWinPosDlg::OnClose()
{
	// TODO: ここにメッセージ ハンドラ コードを追加するか、既定の処理を呼び出します。
	ReleaseWindowInfo();

	CDialog::OnClose();
}

/*-----------------------------------------------------------------------
 * 「＋」ボタンが押された（指定アプリを画面中央へ）
 */
void CWinPosDlg::OnBnClickedButton1()
{
	SetWindowPos(0, 0, false);
}
/*-----------------------------------------------------------------------
 * 「□」ボタンが押された（指定アプリをタスクバーを考慮した画面中央へ）
 */
void CWinPosDlg::OnBnClickedButton2()
{
	SetWindowPos(0, 0, true);
}
/*-----------------------------------------------------------------------
 * 「↑」ボタンが押された
 */
void CWinPosDlg::OnBnClickedButtonUp()
{
	SetWindowPos(IS_NO_MOVE, -1, true);
}
/*-----------------------------------------------------------------------
 * 「↓」ボタンが押された
 */
void CWinPosDlg::OnBnClickedButtonDown()
{
	SetWindowPos(IS_NO_MOVE, 1, true);
}
/*-----------------------------------------------------------------------
 * 「←」ボタンが押された
 */
void CWinPosDlg::OnBnClickedButtonLeft()
{
	SetWindowPos(-1, IS_NO_MOVE, true);
}
/*-----------------------------------------------------------------------
 * 「→」ボタンが押された
 */
void CWinPosDlg::OnBnClickedButtonRight()
{
	SetWindowPos(1, IS_NO_MOVE, true);
}
