/*
Copyright (C) 2026 Taichi Murakami.
アプリケーションの機能を公開します。
*/
#pragma once
#include <windows.h>
#define SAFE_RELEASE(p)         if (p) (p)->lpVtbl->Release(p)
#define STATIC_ASSERT(X)        static_assert((X), #X)
#define WINDOWEXTRA_LPTR(X)     BYTE X[sizeof(LONG_PTR)]
#define WINDOWEXTRA_LONG(X)     BYTE X[sizeof(LONG)]
#define WINDOWEXTRA_WORD(X)     BYTE X[sizeof(WORD)]
#define MAXCUSTCOLORS           16
#define VIEWERCLASSNAME         TEXT("Viewer")
#define CCH_CAPTION             192
#define CCH_FILEFILTER          128
#define CCH_TITLE               32
#define ZOOM_DEFAULT_VALUE      100
#define ZOOM_MAXIMUM            1000
#define ZOOM_MINIMUM            1

/* ウィンドウ インスタンス */
#ifdef _WIN64
#define VIEWERWINDOWEXTRA       64
#else
#define VIEWERWINDOWEXTRA       36
#endif

/* Viewer ウィンドウ メッセージ */
enum ViewerWindowMessage
{
	VWM_TRANSLATEACCELERATOR = WM_USER,
	VWM_LOAD,
	VWM_SHOWDIALOG,
};

/* ダイアログ ボックス */
enum ViewerWindowDialog
{
	VWD_ABOUT,
	VWD_COLOR,
	VWD_OPEN,
	VWD_PAGESETUP,
	VWD_PRINT,
};

EXTERN_C HRESULT WINAPI
CreateDocumentBitmap(_In_ HDC hDC, _In_ IWICBitmap *pBitmap, _Out_ HBITMAP *phBitmap);

EXTERN_C HRESULT WINAPI
CreateDocumentFrame(_In_ IWICImagingFactory *pFactory, _In_ IWICBitmapDecoder *pDecoder, _In_ UINT iFrame, _Out_ IWICBitmap **ppBitmap);

EXTERN_C UINT_PTR CALLBACK
DefHook(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

EXTERN_C BOOL WINAPI
DrawImage(_In_ HDC hdc, _In_opt_ CONST RECT FAR *lpRect, _In_ HBITMAP hbmImage, _In_ BOOL bKeepAspect);

EXTERN_C int WINAPI
ErrorMessageBox(_In_opt_ HWND hWnd, _In_ DWORD dwError, _In_opt_ HINSTANCE hInstance);

EXTERN_C int WINAPI
LoadStringFileFilter(_In_opt_ HINSTANCE hInstance, _In_ UINT uID, _Out_writes_(cchBufferMax) LPTSTR lpBuffer, _In_ int cchBufferMax);

EXTERN_C BOOL WINAPI
MoveWindowForRect(_In_ HWND hWnd, _In_ CONST RECT FAR *lpRect, _In_ BOOL bRepaint);

EXTERN_C UINT_PTR CALLBACK
PagePaintHook(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

EXTERN_C LRESULT CALLBACK
ViewerWindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
