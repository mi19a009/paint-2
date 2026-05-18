/*
Copyright (C) 2026 Taichi Murakami.
アプリケーションのメイン エントリ ポイントを実装します。
*/
#include "stdafx.h"
#include "viewer.h"
#include "resource.h"
#define ERRORMSGBOXFLAGS        (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM)

static HWND WINAPI
CreateViewerWindow(_In_opt_ HINSTANCE hInstance);

static BOOL WINAPI
Initialize(_In_opt_ HINSTANCE hInstance);

static BOOL WINAPI
Load(_In_ HWND hWnd, _In_ LPCWSTR lpCmdLine);

static BOOL WINAPI
Register(_In_opt_ HINSTANCE hInstance);

static BOOL WINAPI
RegisterViewerClass(_In_opt_ HINSTANCE hInstance);

static const BLENDFUNCTION BLEND =
{ AC_SRC_OVER, 0, MAXBYTE, AC_SRC_ALPHA };

/*
アプリケーションのメイン エントリ ポイントです。
*/
EXTERN_C int APIENTRY
wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInst, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	HWND hWnd;
	MSG msg;
	int nExitCode = -1;

	if (Initialize(hInstance))
	{
		if (Register(hInstance))
		{
			hWnd = CreateViewerWindow(hInstance);

			if (hWnd)
			{
				ShowWindow(hWnd, nCmdShow);
				UpdateWindow(hWnd);

				if (*lpCmdLine)
				{
					Load(hWnd, lpCmdLine);
				}
				while (GetMessage(&msg, NULL, 0, 0) > 0)
				{
					if (!SendMessage(hWnd, VWM_TRANSLATEACCELERATOR, 0, (LPARAM)&msg))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}

				nExitCode = (int)msg.wParam;
			}
		}

		CoUninitialize();
	}

	return nExitCode;
}

/*
Imaging Bitmap から Compatible Bitmap を作成します。
*/
EXTERN_C HRESULT WINAPI
CreateDocumentBitmap(_In_ HDC hDC, _In_ IWICBitmap *pBitmap, _Out_ HBITMAP *phBitmap)
{
	IWICBitmapLock *pLock;
	WICInProcPointer pData, pLine;
	HBITMAP hBitmap;
	BITMAPINFOHEADER info;
	HRESULT hResult;
	UINT nWidth, nHeight, cbStride, nLine;
	hBitmap = NULL;
	hResult = pBitmap->lpVtbl->Lock(pBitmap, NULL, WICBitmapLockRead, &pLock);

	if (SUCCEEDED(hResult))
	{
		ZeroMemory(&info, sizeof info);
		hResult = pLock->lpVtbl->GetDataPointer(pLock, &info.biSizeImage, &pData);

		if (SUCCEEDED(hResult))
		{
			hResult = pLock->lpVtbl->GetSize(pLock, &nWidth, &nHeight);

			if (SUCCEEDED(hResult))
			{
				hResult = pLock->lpVtbl->GetStride(pLock, &cbStride);

				if (SUCCEEDED(hResult))
				{
					hBitmap = CreateCompatibleBitmap(hDC, nWidth, nHeight);

					if (hBitmap)
					{
						info.biSize = sizeof info;
						info.biWidth = nWidth;
						info.biHeight = nHeight;
						info.biPlanes = 1;
						info.biBitCount = 32;

						for (nLine = 0; nLine < nHeight; nLine++)
						{
							pLine = pData + (cbStride * (nHeight - nLine - 1));
							SetDIBits(hDC, hBitmap, nLine, 1, pLine, (LPBITMAPINFO)&info, 0);
						}
					}
					else
					{
						hResult = HRESULT_FROM_WIN32(GetLastError());
					}
				}
			}
		}

		pLock->lpVtbl->Release(pLock);
	}

	*phBitmap = hBitmap;
	return hResult;
}

/*
指定したドキュメントから任意のフレームを取得します。
*/
EXTERN_C HRESULT WINAPI
CreateDocumentFrame(_In_ IWICImagingFactory *pFactory, _In_ IWICBitmapDecoder *pDecoder, _In_ UINT iFrame, _Out_ IWICBitmap **ppBitmap)
{
	IWICBitmapFrameDecode *pFrame;
	IWICFormatConverter *pConverter;
	IWICBitmap *pBitmap;
	HRESULT hResult;
	pBitmap = NULL;
	hResult = pDecoder->lpVtbl->GetFrame(pDecoder, iFrame, &pFrame);

	if (SUCCEEDED(hResult))
	{
		hResult = pFactory->lpVtbl->CreateFormatConverter(pFactory, &pConverter);

		if (SUCCEEDED(hResult))
		{
			hResult = pConverter->lpVtbl->Initialize(pConverter, (IWICBitmapSource*)pFrame, &GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 1.0, WICBitmapPaletteTypeMedianCut);

			if (SUCCEEDED(hResult))
			{
				hResult = pFactory->lpVtbl->CreateBitmapFromSource(pFactory, (IWICBitmapSource*)pConverter, WICBitmapCacheOnLoad, &pBitmap);
			}

			pConverter->lpVtbl->Release(pConverter);
		}

		pFrame->lpVtbl->Release(pFrame);
	}

	*ppBitmap = pBitmap;
	return hResult;
}

/*
既定のフック プロシージャです。
*/
EXTERN_C UINT_PTR CALLBACK
DefHook(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	return FALSE;
}

/*
指定した画像を描画します。
画像は矩形の範囲内で拡大されます。
画像の比率は保持されます。
*/
EXTERN_C BOOL WINAPI
DrawImage(_In_ HDC hdc, _In_opt_ CONST RECT FAR *lpRect, _In_ HBITMAP hbmImage, _In_ BOOL bKeepAspect)
{
	HDC hdcImage;
	BITMAP info;
	int X, Y, nWidth, nHeight;
	FLOAT arImage;
	BOOL bResult;
	hdcImage = CreateCompatibleDC(hdc);

	if (hdcImage)
	{
		GetObject(hbmImage, sizeof info, &info);

		if (lpRect)
		{
			X = lpRect->left;
			Y = lpRect->top;
			nWidth = lpRect->right - X;
			nHeight = lpRect->bottom - Y;
		}
		else
		{
			X = 0;
			Y = 0;
			nWidth = info.bmWidth;
			nHeight = info.bmHeight;
		}
		if (bKeepAspect)
		{
			arImage = info.bmWidth / (FLOAT)info.bmHeight;

			if (arImage < (nWidth / (FLOAT)nHeight))
			{
				bResult = nWidth;
				nWidth = (int)(nHeight / arImage);
				X += (int)(0.5F * (bResult - nWidth));
			}
			else
			{
				bResult = nHeight;
				nHeight = (int)(nWidth * arImage);
				Y += (int)(0.5F * (bResult - nHeight));
			}
		}

		SelectObject(hdcImage, hbmImage);
		bResult = AlphaBlend(hdc, X, Y, nWidth, nHeight, hdcImage, 0, 0, info.bmWidth, info.bmHeight, BLEND);
		DeleteDC(hdcImage);
	}
	else
	{
		bResult = FALSE;
	}

	return bResult;
}

/*
指定したエラーを説明するメッセージ ボックスを表示します。
*/
EXTERN_C int WINAPI
ErrorMessageBox(_In_opt_ HWND hWnd, _In_ DWORD dwError, _In_opt_ HINSTANCE hInstance)
{
	LPTSTR lpBuffer;
	int nResult;
	TCHAR strTitle[CCH_TITLE];
	lpBuffer = NULL;

	if (!dwError)
	{
		dwError = GetLastError();
	}
	if (!hInstance && hWnd)
	{
		hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
	}
	if (FormatMessage(ERRORMSGBOXFLAGS, NULL, dwError, 0, (LPTSTR)&lpBuffer, 0, NULL))
	{
		LoadString(hInstance, IDS_APPTITLE, strTitle, CCH_TITLE);
		nResult = MessageBox(hWnd, lpBuffer, strTitle, MB_ICONERROR);
	}
	else
	{
		nResult = 0;
	}

	LocalFree(lpBuffer);
	return nResult;
}

/*
ファイル フィルター用文字列を取得します。
フィルター文字列の末尾は 2 個以上の連続した NULL 文字が含まれます。
*/
EXTERN_C int WINAPI
LoadStringFileFilter(_In_opt_ HINSTANCE hInstance, _In_ UINT uID, _Out_writes_(cchBufferMax) LPTSTR lpBuffer, _In_ int cchBufferMax)
{
	int nResult, nIndex;
	nResult = LoadString(hInstance, uID, lpBuffer, cchBufferMax);

	if (cchBufferMax < 2)
	{
		lpBuffer[0] = 0;
	}
	else
	{
		for (nIndex = 0; nIndex < nResult; nIndex++)
		{
			if (lpBuffer[nIndex] == TEXT('|'))
			{
				lpBuffer[nIndex] = 0;
			}
		}

		lpBuffer[cchBufferMax - 2] = 0;
	}

	return nResult;
}

/*
指定したウィンドウを移動します。
*/
EXTERN_C BOOL WINAPI
MoveWindowForRect(_In_ HWND hWnd, _In_ CONST RECT FAR *lpRect, _In_ BOOL bRepaint)
{
	LONG X, Y, nWidth, nHeight;
	X = lpRect->left;
	Y = lpRect->top;
	nWidth = lpRect->right - X;
	nHeight = lpRect->bottom - Y;
	return MoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);
}

/*
ページ設定ダイアログ ボックスでサンプル ページを描画します。
*/
EXTERN_C UINT_PTR CALLBACK
PagePaintHook(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	UINT_PTR bResult;
	bResult = FALSE;

	switch (uMsg)
	{
	case WM_PSD_GREEKTEXTRECT:
		bResult = GetWindowLongPtr(hWnd, GWLP_USERDATA);
		bResult = bResult && DrawImage((HDC)wParam, (LPRECT)lParam, (HBITMAP)bResult, TRUE);
		break;
	case WM_PSD_PAGESETUPDLG:
		SetWindowLongPtr(hWnd, GWLP_USERDATA, ((LPPAGESETUPDLG)lParam)->lCustData);
		break;
	}

	return bResult;
}

/*
アプリケーション ウィンドウを作成します。
*/
static HWND WINAPI
CreateViewerWindow(_In_opt_ HINSTANCE hInstance)
{
	TCHAR strTitle[CCH_TITLE];
	LoadString(hInstance, IDS_APPTITLE, strTitle, CCH_TITLE);
	return CreateWindow(VIEWERCLASSNAME, strTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
}

/*
アプリケーションを初期化します。
成功した場合は TRUE を返します。
失敗した場合はメッセージ ボックスを表示します。
*/
static BOOL WINAPI
Initialize(_In_opt_ HINSTANCE hInstance)
{
	HRESULT hResult;
	BOOL bSucceeded;
	hResult = CoInitialize(NULL);
	bSucceeded = SUCCEEDED(hResult);

	if (!bSucceeded)
	{
		ErrorMessageBox(NULL, hResult, hInstance);
	}

	return bSucceeded;
}

/*
ファイルを開きます。
成功した場合は TRUE を返します。
失敗した場合はメッセージ ボックスを表示します。
*/
static BOOL WINAPI
Load(_In_ HWND hWnd, _In_ LPCWSTR lpCmdLine)
{
	LPWSTR *argv;
	int argc;
	argv = CommandLineToArgvW(lpCmdLine, &argc);

	if (argv)
	{
		SendMessage(hWnd, VWM_LOAD, 0, (LPARAM)argv[0]);
		LocalFree(argv);
		argc = TRUE;
	}
	else
	{
		ErrorMessageBox(hWnd, 0, NULL);
		argc = FALSE;
	}

	return argc;
}

/*
ウィンドウ クラスを登録します。
成功した場合は TRUE を返します。
失敗した場合はメッセージ ボックスを表示します。
*/
static BOOL WINAPI
Register(_In_opt_ HINSTANCE hInstance)
{
	BOOL bResult;
	bResult = RegisterViewerClass(hInstance);

	if (!bResult)
	{
		ErrorMessageBox(NULL, 0, hInstance);
	}

	return bResult;
}

/*
Viewer ウィンドウ クラスを登録します。
成功した場合は 0 以外を返します。
*/
static BOOL WINAPI
RegisterViewerClass(_In_opt_ HINSTANCE hInstance)
{
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof wc);
	wc.cbSize = sizeof wc;
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = ViewerWindowProc;
	wc.cbWndExtra = VIEWERWINDOWEXTRA;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_VIEWER));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_VIEWER);
	wc.lpszClassName = VIEWERCLASSNAME;
	return RegisterClassEx(&wc);
}
