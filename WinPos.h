﻿
// WinPos.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'pch.h' をインクルードしてください"
#endif

#include "resource.h"		// メイン シンボル


// CWinPosApp:
// このクラスの実装については、WinPos.cpp を参照してください
//

class CWinPosApp : public CWinApp
{
public:
	CWinPosApp();

// オーバーライド
public:
	virtual BOOL InitInstance();

// 実装

	DECLARE_MESSAGE_MAP()
};

extern CWinPosApp theApp;
