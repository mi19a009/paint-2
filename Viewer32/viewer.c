/*
Copyright (C) 2026 Taichi Murakami.
アプリケーション ウィンドウ プロシージャを実装します。
*/
#include "stdafx.h"
#include "viewer.h"
#include "resource.h"

/* Viewer ウィンドウ インスタンス */
typedef struct ViewerWindowExtra
{
	WINDOWEXTRA_LPTR(lpDocument);
	WINDOWEXTRA_LPTR(lpFactory);
	WINDOWEXTRA_LPTR(hAccel);
	WINDOWEXTRA_LPTR(hbmImage);
	WINDOWEXTRA_LPTR(hbrBackground);
	WINDOWEXTRA_LPTR(hDevMode);
	WINDOWEXTRA_LPTR(hDevNames);
	WINDOWEXTRA_LONG(rgbBackground);
	WINDOWEXTRA_LONG(nDocumentFrame);
} WINDOWEXTRA;

/* Viewer ウィンドウ User Data */
typedef struct ViewerWindowUserData
{
	RECT rcMargin;
	COLORREF rgbCustColors[MAXCUSTCOLORS];
	TCHAR strFile[MAX_PATH];
} USERDATA, FAR *LPUSERDATA;

STATIC_ASSERT(sizeof(WINDOWEXTRA) == VIEWERWINDOWEXTRA);
#define GWLP_DOCUMENT           offsetof(WINDOWEXTRA, lpDocument)
#define GWLP_FACTORY            offsetof(WINDOWEXTRA, lpFactory)
#define GWLP_HACCEL             offsetof(WINDOWEXTRA, hAccel)
#define GWLP_HBMIMAGE           offsetof(WINDOWEXTRA, hbmImage)
#define GWLP_HBRBACKGROUND      offsetof(WINDOWEXTRA, hbrBackground)
#define GWLP_HDEVMODE           offsetof(WINDOWEXTRA, hDevMode)
#define GWLP_HDEVNAMES          offsetof(WINDOWEXTRA, hDevNames)
#define GWL_RGBBACKGROUND       offsetof(WINDOWEXTRA, rgbBackground)
#define GWL_DOCUMENTFRAME       offsetof(WINDOWEXTRA, nDocumentFrame)

static BOOL WINAPI
OnCommand(_In_ HWND hWnd, _In_ UINT idMenu);

static BOOL WINAPI
OnCreate(_In_ HWND hWnd, _In_ CONST CREATESTRUCT FAR *lpCreate);

static void WINAPI
OnDestroy(_In_ HWND hWnd);

static void WINAPI
OnDpiChanged(_In_ HWND hWnd, _In_ UINT nDpi, _In_ CONST RECT FAR *lpRect);

static BOOL WINAPI
OnEraseBackground(_In_ HWND hWnd, _In_ HDC hDC);

static void WINAPI
OnPaint(_In_ HWND hWnd);

static BOOL WINAPI
OnShowDialog(_In_ HWND hWnd, _In_ UINT uDialog);

static BOOL WINAPI
OnTranslateAccelerator(_In_ HWND hWnd, _In_ LPMSG lpMsg);

static BOOL WINAPI
OnLoad(_In_ HWND hWnd, _In_ LPCTSTR lpDocumentName);

static void WINAPI
ClearDocument(_In_ HWND hWnd);

static HACCEL WINAPI
InitAccelerators(_In_ HWND hWnd, _In_opt_ HINSTANCE hInstance);

static LPUSERDATA WINAPI
InitUserData(_In_ HWND hWnd);

static void WINAPI
SetDocumentName(_In_ HWND hWnd, _In_opt_ LPCTSTR lpName);

static BOOL WINAPI
ShowAboutDialog(_In_ HWND hWnd);

static BOOL WINAPI
ShowColorDialog(_In_ HWND hWnd);

static BOOL WINAPI
ShowOpenFileDialog(_In_ HWND hWnd);

static BOOL WINAPI
ShowPageSetupDialog(_In_ HWND hWnd, _In_ BOOL bReturnDefault);

static BOOL WINAPI
ShowPrintDialog(_In_ HWND hWnd);

static BOOL WINAPI
UpdateBackground(_In_ HWND hWnd);

static BOOL WINAPI
UpdateCaption(_In_ HWND hWnd);

static BOOL WINAPI
UpdateDocument(_In_ HWND hWnd);

static BOOL WINAPI
UpdateImage(_In_ HWND hWnd);

static void WINAPI
UpdateImageMenuItems(_In_ HWND hWnd);

/*
Viewer ウィンドウ プロシージャです。
*/
EXTERN_C LRESULT CALLBACK
ViewerWindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (OnCommand(hWnd, LOWORD(wParam))) return 0;
		else break;
	case WM_CREATE:
		if (OnCreate(hWnd, (LPCREATESTRUCT)lParam)) break;
		else return -1;
	case WM_DESTROY:
		OnDestroy(hWnd);
		break;
	case WM_DPICHANGED:
		OnDpiChanged(hWnd, LOWORD(wParam), (LPRECT)lParam);
		return 0;
	case WM_ERASEBKGND:
		if (OnEraseBackground(hWnd, (HDC)wParam)) return TRUE;
		else break;
	case WM_PAINT:
		OnPaint(hWnd);
		return 0;
	case VWM_LOAD:
		return OnLoad(hWnd, (LPCTSTR)lParam);
	case VWM_SHOWDIALOG:
		return OnShowDialog(hWnd, (UINT)wParam);
	case VWM_TRANSLATEACCELERATOR:
		return OnTranslateAccelerator(hWnd, (LPMSG)lParam);
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
指定したメニュー項目を実行します。
メッセージを処理した場合は TRUE を返します。
*/
static BOOL WINAPI
OnCommand(_In_ HWND hWnd, _In_ UINT idMenu)
{
	UINT uMsg;
	WPARAM wParam;
	wParam = 0;

	switch (idMenu)
	{
	case IDM_ABOUT:
		uMsg = VWM_SHOWDIALOG;
		wParam = VWD_ABOUT;
		break;
	case IDM_BACKGROUND:
		uMsg = VWM_SHOWDIALOG;
		wParam = VWD_COLOR;
		break;
	case IDM_EXIT:
		uMsg = WM_CLOSE;
		break;
	case IDM_OPEN:
		uMsg = VWM_SHOWDIALOG;
		wParam = VWD_OPEN;
		break;
	case IDM_PAGESETUP:
		uMsg = VWM_SHOWDIALOG;
		wParam = VWD_PAGESETUP;
		break;
	case IDM_PRINT:
		uMsg = VWM_SHOWDIALOG;
		wParam = VWD_PRINT;
		break;
	default:
		uMsg = 0;
		break;
	}

	return uMsg && PostMessage(hWnd, uMsg, wParam, 0);
}

/*
子ウィンドウを作成します。
メッセージを処理した場合は TRUE を返します。
*/
static BOOL WINAPI
OnCreate(_In_ HWND hWnd, _In_ CONST CREATESTRUCT FAR *lpCreate)
{
	UpdateImageMenuItems(hWnd);
	return InitAccelerators(hWnd, lpCreate->hInstance) && InitUserData(hWnd);
}

/*
プロパティを破棄します。
*/
static void WINAPI
OnDestroy(_In_ HWND hWnd)
{
	LONG_PTR lpExtra;
	lpExtra = SetWindowLongPtr(hWnd, GWLP_FACTORY, 0);
	SAFE_RELEASE((LPUNKNOWN)lpExtra);
	lpExtra = SetWindowLongPtr(hWnd, GWLP_DOCUMENT, 0);
	SAFE_RELEASE((LPUNKNOWN)lpExtra);
	lpExtra = SetWindowLongPtr(hWnd, GWLP_HACCEL, 0);
	if (lpExtra) DestroyAcceleratorTable((HACCEL)lpExtra);
	lpExtra = SetWindowLongPtr(hWnd, GWLP_HBMIMAGE, 0);
	if (lpExtra) DeleteObject((HGDIOBJ)lpExtra);
	lpExtra = SetWindowLongPtr(hWnd, GWLP_HBRBACKGROUND, 0);
	if (lpExtra) DeleteObject((HGDIOBJ)lpExtra);
	lpExtra = SetWindowLongPtr(hWnd, GWLP_HDEVMODE, 0);
	if (lpExtra) GlobalFree((HGLOBAL)lpExtra);
	lpExtra = SetWindowLongPtr(hWnd, GWLP_HDEVNAMES, 0);
	if (lpExtra) GlobalFree((HGLOBAL)lpExtra);
	PostQuitMessage(0);
}

/*
現在の DPI を設定します。
*/
static void WINAPI
OnDpiChanged(_In_ HWND hWnd, _In_ UINT nDpi, _In_ CONST RECT FAR *lpRect)
{
	MoveWindowForRect(hWnd, lpRect, TRUE);
}

/*
背景を描画します。
背景を描画した場合は TRUE を返します。
*/
static BOOL WINAPI
OnEraseBackground(_In_ HWND hWnd, _In_ HDC hDC)
{
	HBRUSH hBrush;
	RECT rcClient;
	hBrush = (HBRUSH)GetWindowLongPtr(hWnd, GWLP_HBRBACKGROUND);
	return hBrush && GetClientRect(hWnd, &rcClient) && FillRect(hDC, &rcClient, hBrush);
}

/*
指定したファイルを開きます。
ファイルを開いた場合は TRUE を返します。
*/
static BOOL WINAPI
OnLoad(_In_ HWND hWnd, _In_ LPCTSTR lpDocumentName)
{
	BOOL bResult;
	ClearDocument(hWnd);
	SetDocumentName(hWnd, lpDocumentName);
	bResult = UpdateDocument(hWnd) && UpdateImage(hWnd);
	UpdateCaption(hWnd);
	UpdateImageMenuItems(hWnd);
	InvalidateRect(hWnd, NULL, TRUE);
	return bResult;
}

/*
クライアント領域を描画します。
*/
static void WINAPI
OnPaint(_In_ HWND hWnd)
{
	HDC hDC;
	HBITMAP hBitmap;
	PAINTSTRUCT paint;
	hDC = BeginPaint(hWnd, &paint);

	if (hDC)
	{
		hBitmap = (HBITMAP)GetWindowLongPtr(hWnd, GWLP_HBMIMAGE);

		if (hBitmap)
		{
			DrawImage(hDC, NULL, hBitmap, FALSE);
		}

		EndPaint(hWnd, &paint);
	}
}

/*
ダイアログ ボックスを表示します。
ダイアログ ボックスを表示した場合は TRUE を返します。
*/
static BOOL WINAPI
OnShowDialog(_In_ HWND hWnd, _In_ UINT uDialog)
{
	BOOL bResult;

	switch (uDialog)
	{
	case VWD_ABOUT:
		bResult = ShowAboutDialog(hWnd);
		break;
	case VWD_COLOR:
		bResult = ShowColorDialog(hWnd);
		break;
	case VWD_OPEN:
		bResult = ShowOpenFileDialog(hWnd);
		break;
	case VWD_PAGESETUP:
		bResult = ShowPageSetupDialog(hWnd, FALSE);
		break;
	case VWD_PRINT:
		bResult = ShowPrintDialog(hWnd);
		break;
	default:
		bResult = FALSE;
		break;
	}

	return bResult;
}

/*
キーボード アクセラレーターを処理します。
*/
static BOOL WINAPI
OnTranslateAccelerator(_In_ HWND hWnd, _In_ LPMSG lpMsg)
{
	HACCEL hAccel;
	hAccel = (HACCEL)GetWindowLongPtr(hWnd, GWLP_HACCEL);
	return hAccel && TranslateAccelerator(hWnd, hAccel, lpMsg);
}

/*
ドキュメントを初期化します。
*/
static void WINAPI
ClearDocument(_In_ HWND hWnd)
{
	LONG_PTR lpExtra;
	lpExtra = SetWindowLongPtr(hWnd, GWLP_DOCUMENT, 0);
	SAFE_RELEASE((LPUNKNOWN)lpExtra);
	lpExtra = SetWindowLongPtr(hWnd, GWLP_HBMIMAGE, 0);
	if (lpExtra) DeleteObject((HGDIOBJ)lpExtra);
	SetWindowLong(hWnd, GWL_DOCUMENTFRAME, 0);
}

/*
キーボード アクセラレーターを作成します。
*/
static HACCEL WINAPI
InitAccelerators(_In_ HWND hWnd, _In_opt_ HINSTANCE hInstance)
{
	HACCEL hAccel;
	hAccel = (HACCEL)GetWindowLongPtr(hWnd, GWLP_HACCEL);

	if (!hAccel)
	{
		hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_VIEWER));

		if (hAccel)
		{
			SetWindowLongPtr(hWnd, GWLP_HACCEL, (LONG_PTR)hAccel);
		}
		else
		{
			ErrorMessageBox(hWnd, 0, NULL);
		}
	}

	return hAccel;
}

/*
User Data を作成します。
*/
static LPUSERDATA WINAPI
InitUserData(_In_ HWND hWnd)
{
	LPUSERDATA lpUserData;
	lpUserData = (LPUSERDATA)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if (!lpUserData)
	{
		lpUserData = LocalAlloc(LMEM_FIXED, sizeof(USERDATA));

		if (lpUserData)
		{
			lpUserData->strFile[0] = 0;
			ZeroMemory(lpUserData->rgbCustColors, sizeof lpUserData->rgbCustColors);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpUserData);
		}
		else
		{
			ErrorMessageBox(hWnd, 0, NULL);
		}
	}

	return lpUserData;
}

/*
開いたドキュメント ファイルの名前を設定します。
*/
static void WINAPI
SetDocumentName(_In_ HWND hWnd, _In_opt_ LPCTSTR lpName)
{
	LPUSERDATA lpUserData;
	lpUserData = (LPUSERDATA)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if (lpUserData)
	{
		if (lpName)
		{
			StringCchCopy(lpUserData->strFile, ARRAYSIZE(lpUserData->strFile), lpName);
		}
		else
		{
			lpUserData->strFile[0] = 0;
		}
	}
}

/*
バージョン情報ダイアログ ボックスを表示します。
ダイアログ ボックスを表示した場合は TRUE を返します。
*/
static BOOL WINAPI
ShowAboutDialog(_In_ HWND hWnd)
{
	MSGBOXPARAMS params;
	ZeroMemory(&params, sizeof params);
	params.cbSize = sizeof params;
	params.hwndOwner = hWnd;
	params.hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
	params.lpszCaption = MAKEINTRESOURCE(IDS_APPTITLE);
	params.lpszText = MAKEINTRESOURCE(IDS_APPABOUT);
	params.dwStyle = MB_USERICON;
	params.lpszIcon = MAKEINTRESOURCE(IDR_VIEWER);
	return MessageBoxIndirect(&params);
}

/*
色ダイアログ ボックスを表示します。
ダイアログ ボックスを表示した場合は TRUE を返します。
*/
static BOOL WINAPI
ShowColorDialog(_In_ HWND hWnd)
{
	LPUSERDATA lpUserData;
	CHOOSECOLOR cc;
	BOOL bResult;
	lpUserData = (LPUSERDATA)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	bResult = FALSE;

	if (lpUserData)
	{
		ZeroMemory(&cc, sizeof cc);
		cc.lStructSize = sizeof cc;
		cc.hwndOwner = hWnd;
		cc.rgbResult = GetWindowLong(hWnd, GWL_RGBBACKGROUND);
		cc.lpCustColors = lpUserData->rgbCustColors;
		cc.Flags = CC_RGBINIT | CC_ENABLEHOOK;
		cc.lpfnHook = DefHook;

		if (ChooseColor(&cc))
		{
			SetWindowLong(hWnd, GWL_RGBBACKGROUND, cc.rgbResult);
			bResult = UpdateBackground(hWnd);
			InvalidateRect(hWnd, NULL, TRUE);
		}
	}

	return bResult;
}

/*
開くダイアログ ボックスを表示します。
ダイアログ ボックスを表示した場合は TRUE を返します。
*/
static BOOL WINAPI
ShowOpenFileDialog(_In_ HWND hWnd)
{
	OPENFILENAME ofn;
	TCHAR strFilter[CCH_FILEFILTER], strFile[MAX_PATH];
	ZeroMemory(&ofn, sizeof ofn);
	ofn.lStructSize = sizeof ofn;
	ofn.hwndOwner = hWnd;
	ofn.hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
	ofn.lpstrFilter = strFilter;
	ofn.lpstrFile = strFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_ENABLEHOOK | OFN_FILEMUSTEXIST | OFN_EXPLORER;
	ofn.lpfnHook = DefHook;
	strFile[0] = 0;
	LoadStringFileFilter(ofn.hInstance, IDS_OPENFILEFILTER, strFilter, CCH_FILEFILTER);
	return GetOpenFileName(&ofn) && SendMessage(hWnd, VWM_LOAD, 0, (LPARAM)strFile);
}

/*
ページ設定ダイアログ ボックスを表示します。
RETURNDEFAULT フラグが指定され、DEVMODE または DEVNAMES が初期化されている場合、ダイアログ ボックスを表示しません。
ダイアログ ボックスを表示した場合は TRUE を返します。
*/
static BOOL WINAPI
ShowPageSetupDialog(_In_ HWND hWnd, _In_ BOOL bReturnDefault)
{
	LPUSERDATA lpUserData;
	PAGESETUPDLG dlg;
	BOOL bResult;
	lpUserData = (LPUSERDATA)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	bResult = FALSE;

	if (lpUserData)
	{
		ZeroMemory(&dlg, sizeof dlg);
		dlg.hDevMode = (HGLOBAL)GetWindowLongPtr(hWnd, GWLP_HDEVMODE);
		dlg.hDevNames = (HGLOBAL)GetWindowLongPtr(hWnd, GWLP_HDEVNAMES);

		if (bReturnDefault && (dlg.hDevMode || dlg.hDevNames))
		{
			bResult = TRUE;
		}
		else
		{
			dlg.lStructSize = sizeof dlg;
			dlg.hwndOwner = hWnd;
			dlg.Flags = PSD_ENABLEPAGESETUPHOOK | PSD_ENABLEPAGEPAINTHOOK;
			dlg.lCustData = GetWindowLongPtr(hWnd, GWLP_HBMIMAGE);
			dlg.lpfnPageSetupHook = DefHook;
			dlg.lpfnPagePaintHook = PagePaintHook;

			if (dlg.hDevMode && dlg.hDevNames)
			{
				dlg.Flags |= PSD_MARGINS;
				CopyMemory(&dlg.rtMargin, &lpUserData->rcMargin, sizeof(RECT));
			}
			if (bReturnDefault)
			{
				dlg.Flags |= PSD_RETURNDEFAULT;
			}
			if (PageSetupDlg(&dlg))
			{
				CopyMemory(&lpUserData->rcMargin, &dlg.rtMargin, sizeof(RECT));
				bResult = TRUE;
			}

			SetWindowLongPtr(hWnd, GWLP_HDEVMODE, (LONG_PTR)dlg.hDevMode);
			SetWindowLongPtr(hWnd, GWLP_HDEVNAMES, (LONG_PTR)dlg.hDevNames);
		}
	}

	return bResult;
}

/*
印刷ダイアログ ボックスを表示します。
ダイアログ ボックスを表示した場合は TRUE を返します。
*/
static BOOL WINAPI
ShowPrintDialog(_In_ HWND hWnd)
{
	HBITMAP hBitmap;
	PRINTDLG dlg;
	BOOL bResult;
	hBitmap = (HBITMAP)GetWindowLongPtr(hWnd, GWLP_HBMIMAGE);
	bResult = FALSE;

	if (hBitmap && ShowPageSetupDialog(hWnd, TRUE))
	{
		ZeroMemory(&dlg, sizeof dlg);
		dlg.lStructSize = sizeof dlg;
		dlg.hwndOwner = hWnd;
		dlg.hDevMode = (HGLOBAL)GetWindowLongPtr(hWnd, GWLP_HDEVMODE);
		dlg.hDevNames = (HGLOBAL)GetWindowLongPtr(hWnd, GWLP_HDEVNAMES);
		dlg.Flags = PD_RETURNDC | PD_ENABLEPRINTHOOK | PD_ENABLESETUPHOOK | PD_USEDEVMODECOPIESANDCOLLATE;
		dlg.nFromPage = 1;
		dlg.nToPage = 1;
		dlg.nMinPage = 1;
		dlg.nMaxPage = 1;
		dlg.nCopies = 1;
		dlg.lpfnPrintHook = DefHook;
		dlg.lpfnSetupHook = DefHook;

		if (PrintDlg(&dlg))
		{
			bResult = DrawImage(dlg.hDC, NULL, hBitmap, FALSE);
			DeleteDC(dlg.hDC);
		}

		SetWindowLongPtr(hWnd, GWLP_HDEVMODE, (LONG_PTR)dlg.hDevMode);
		SetWindowLongPtr(hWnd, GWLP_HDEVNAMES, (LONG_PTR)dlg.hDevNames);
	}

	return bResult;
}

/*
現在の背景色から背景ブラシを作成します。
失敗した場合はメッセージ ボックスを表示します。
*/
static BOOL WINAPI
UpdateBackground(_In_ HWND hWnd)
{
	HBRUSH hBrush;
	hBrush = (HBRUSH)GetWindowLongPtr(hWnd, GWLP_HBRBACKGROUND);

	if (hBrush)
	{
		DeleteObject(hBrush);
	}

	hBrush = CreateSolidBrush(GetWindowLong(hWnd, GWL_RGBBACKGROUND));

	if (!hBrush)
	{
		ErrorMessageBox(hWnd, 0, NULL);
	}

	SetWindowLongPtr(hWnd, GWLP_HBRBACKGROUND, (LONG_PTR)hBrush);
	return hBrush != NULL;
}

/*
ウィンドウ テキストを更新します。
*/
static BOOL WINAPI
UpdateCaption(_In_ HWND hWnd)
{
	LPUSERDATA lpUserData;
	LPCTSTR lpCaption;
	HINSTANCE hInstance;
	TCHAR strCaption[CCH_CAPTION], strTitle[CCH_TITLE];
	lpUserData = (LPUSERDATA)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
	LoadString(hInstance, IDS_APPTITLE, strTitle, CCH_TITLE);

	if (lpUserData && lpUserData->strFile[0])
	{
		StringCchPrintf(strCaption, CCH_CAPTION, TEXT("%s - %s"), lpUserData->strFile, strTitle);
		lpCaption = strCaption;
	}
	else
	{
		lpCaption = strTitle;
	}

	return SetWindowText(hWnd, lpCaption);
}

/*
現在のファイル名が示すドキュメントを取得します。
Imaging Factory を作成します。
失敗した場合はメッセージ ボックスを表示します。
*/
static BOOL WINAPI
UpdateDocument(_In_ HWND hWnd)
{
	LPUSERDATA lpUserData;
	IWICImagingFactory *pFactory;
	IWICBitmapDecoder *pDecoder;
	HRESULT hResult;
	lpUserData = (LPUSERDATA)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	pFactory = (IWICImagingFactory*)GetWindowLongPtr(hWnd, GWLP_FACTORY);
	pDecoder = (IWICBitmapDecoder*)GetWindowLongPtr(hWnd, GWLP_DOCUMENT);

	if (pDecoder)
	{
		pDecoder->lpVtbl->Release(pDecoder);
		pDecoder = NULL;
	}
	if (lpUserData)
	{
		if (pFactory)
		{
			hResult = S_OK;
		}
		else
		{
			hResult = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, &pFactory);
			SetWindowLongPtr(hWnd, GWLP_FACTORY, (LONG_PTR)pFactory);
		}
		if (SUCCEEDED(hResult))
		{
			hResult = pFactory->lpVtbl->CreateDecoderFromFilename(pFactory, lpUserData->strFile, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
		}
	}
	else
	{
		hResult = E_ILLEGAL_METHOD_CALL;
	}
	if (FAILED(hResult))
	{
		ErrorMessageBox(hWnd, hResult, NULL);
	}

	SetWindowLongPtr(hWnd, GWLP_DOCUMENT, (LONG_PTR)pDecoder);
	return pDecoder != NULL;
}

/*
現在のフレーム数が示すビットマップを取得します。
失敗した場合はメッセージ ボックスを表示します。
*/
static BOOL WINAPI
UpdateImage(_In_ HWND hWnd)
{
	IWICImagingFactory *pFactory;
	IWICBitmapDecoder *pDecoder;
	IWICBitmap *pBitmap;
	HBITMAP hBitmap;
	HDC hDC;
	HRESULT hResult;
	UINT iFrame;
	pFactory = (IWICImagingFactory*)GetWindowLongPtr(hWnd, GWLP_FACTORY);
	pDecoder = (IWICBitmapDecoder*)GetWindowLongPtr(hWnd, GWLP_DOCUMENT);
	hBitmap = (HBITMAP)GetWindowLongPtr(hWnd, GWLP_HBMIMAGE);

	if (hBitmap)
	{
		DeleteObject(hBitmap);
		hBitmap = NULL;
	}
	if (pFactory && pDecoder)
	{
		iFrame = GetWindowLong(hWnd, GWL_DOCUMENTFRAME);
		hResult = CreateDocumentFrame(pFactory, pDecoder, iFrame, &pBitmap);

		if (SUCCEEDED(hResult))
		{
			hDC = GetDC(hWnd);

			if (hDC)
			{
				hResult = CreateDocumentBitmap(hDC, pBitmap, &hBitmap);
				ReleaseDC(hWnd, hDC);
			}

			pBitmap->lpVtbl->Release(pBitmap);
		}
	}
	else
	{
		hResult = E_ILLEGAL_METHOD_CALL;
	}
	if (FAILED(hResult))
	{
		ErrorMessageBox(hWnd, hResult, NULL);
	}

	SetWindowLongPtr(hWnd, GWLP_HBMIMAGE, (LONG_PTR)hBitmap);
	return hBitmap != NULL;
}

/*
メニュー項目を有効化します。
*/
static void WINAPI
UpdateImageMenuItems(_In_ HWND hWnd)
{
	HMENU hMenu;
	MENUITEMINFO info;
	hMenu = GetMenu(hWnd);

	if (hMenu)
	{
		ZeroMemory(&info, sizeof info);
		info.cbSize = sizeof info;
		info.fMask = MIIM_STATE;

		if (GetWindowLongPtr(hWnd, GWLP_HBMIMAGE))
		{
			info.fState = MF_ENABLED;
		}
		else
		{
			info.fState = MF_DISABLED;
		}

		SetMenuItemInfo(hMenu, IDM_PRINT, FALSE, &info);
		SetMenuItemInfo(hMenu, IDM_PAGESETUP, FALSE, &info);
	}
}
