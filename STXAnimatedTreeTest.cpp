// STXAnimatedTreeTest.cpp
//


#include "stdafx.h"

#define USING_NS_VERSION			//Using Non-static version of AnimatedTree


#include "STXAnimatedTreeTest.h"
#ifdef USING_NS_VERSION
#include "STXAnimatedTreeCtrlNS.h"
#else
#include "STXAnimatedTreeCtrl.h"
#endif

#include "shlwapi.h"
#include <string>
#include "STXAnchor.h"

using namespace std;

#ifdef USING_NS_VERSION
CSTXAnimatedTreeCtrlNS *g_pWnd = nullptr;
#else 
CSTXAnimatedTreeCtrl *g_pWnd = nullptr;
#endif

CSTXAnchor *_anchor = nullptr;

//Item comparing function
int CALLBACK STXTestSortFunc( LPARAM lParamItem1, LPARAM lParamItem2, LPARAM lParamSort )
{
	if(lParamItem1 < lParamItem2)
		return -1;
	else if(lParamItem1 > lParamItem2)
		return 1;

	return 0;
}

int iFactor = 1;
int CALLBACK STXTestSortItemFunc( HSTXTREENODE hItem1, HSTXTREENODE hItem2, LPARAM lParamSort)
{
	TCHAR szBuf1[1024];
	TCHAR szBuf2[1024];
	g_pWnd->Internal_GetItemText(hItem1, szBuf1, 1024);
	g_pWnd->Internal_GetItemText(hItem2, szBuf2, 1024);
	return iFactor * _tcscmp(szBuf1, szBuf2);
}

//Custom Draw function
int CALLBACK MySTXAnimatedTreeItemDrawFunc(LPSTXTVITEMDRAW lpItemDraw)
{
	switch(lpItemDraw->dwStage)
	{
	case STXTV_STAGE_PREPAINT:
		return STXTV_CDRF_DODEFAULT;

	case STXTV_STAGE_BACKGROUND:
		return STXTV_CDRF_DODEFAULT;
	case STXTV_STAGE_IMAGE:
		{
			COLORREF clrHighlight = GetSysColor(COLOR_HIGHLIGHT);

			Gdiplus::SolidBrush bkBrush(Gdiplus::Color(static_cast<BYTE>(192 * lpItemDraw->fOpacity), GetRValue(clrHighlight), GetGValue(clrHighlight), GetBValue(clrHighlight)));
			Gdiplus::RectF rectSelection(lpItemDraw->rectItem->GetLeft(), lpItemDraw->rectItem->GetTop(), lpItemDraw->rectItem->Height, lpItemDraw->rectItem->Height);
			//rectSelection.X += iImageOccupie;
			//rectSelection.Width -= iImageOccupie;

			lpItemDraw->pGraphics->FillRectangle(&bkBrush, rectSelection);
		}
		return STXTV_CDRF_SKIPDEFAULT;

	case STXTV_STAGE_SUBIMAGE:
		return STXTV_CDRF_DODEFAULT;
	case STXTV_STAGE_TEXT:
		return STXTV_CDRF_DODEFAULT;

	case STXTV_STAGE_POSTPAINT:
		return STXTV_CDRF_DODEFAULT;
	}


	return STXTV_CDRF_DODEFAULT;
}


#define MAX_LOADSTRING 100

// Global variables:
HINSTANCE hInst;								// Current instance
TCHAR szTitle[MAX_LOADSTRING];					// Caption text
TCHAR szWindowClass[MAX_LOADSTRING];			// Class name of main window

HSTXTREENODE g_NodeParent = STXTVI_ROOT;
HSTXTREENODE g_NodeC1Parent = STXTVI_ROOT;
HSTXTREENODE g_NodeC2Parent = STXTVI_ROOT;
HSTXTREENODE g_NodeC3Parent = STXTVI_ROOT;
HSTXTREENODE g_NodeToSetImage[16];

IStream *pStreamBK = nullptr;
IStream *pStreamMail = nullptr;
IStream *pStream1 = nullptr;
IStream *pStream2 = nullptr;

int iTimerStep = 0;


// forward declarations
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
TCHAR szExePath[MAX_PATH];
TCHAR szRandom[MAX_PATH];

LPCTSTR GetRandomString()
{
	_itot_s( rand(), szRandom, MAX_PATH, 10);
	return szRandom;
}

IStream* LoadImageFromResource(HMODULE hModule, LPCWSTR lpName, LPCWSTR lpType)
{
	HRSRC hRC = FindResource(hModule, lpName, lpType);
	if(hRC == nullptr)
		return nullptr;

	HGLOBAL hPkg = LoadResource(hModule, hRC);
	if(hPkg == nullptr)
		return nullptr;

	DWORD dwSize = SizeofResource(hModule, hRC);
	LPVOID pData = LockResource(hPkg);

	HGLOBAL hImage = GlobalAlloc(GMEM_FIXED, dwSize);
	LPVOID pImageBuf = GlobalLock(hImage);
	memcpy(pImageBuf, pData, dwSize);
	GlobalUnlock(hImage);

	UnlockResource(hPkg);

	IStream *pStream = nullptr;
	CreateStreamOnHGlobal(hImage, TRUE, &pStream);

	return pStream;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;
	
	//////////////////////////////////////////////////////////////////////////

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_STXANIMATEDTREETEST, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	GetModuleFileName(nullptr, szExePath, MAX_PATH);
	LPTSTR pszName = PathFindFileName(szExePath);
	pszName[0] = 0;

	CoInitialize(nullptr);
	ULONG_PTR m_gdiplusToken = 0;
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;  
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);

	pStreamBK = LoadImageFromResource(nullptr, MAKEINTRESOURCE(IDB_PNG_BK), _T("PNG"));
	pStreamMail = LoadImageFromResource(nullptr, MAKEINTRESOURCE(IDB_PNG_MAIL), _T("PNG"));
	pStream1 = LoadImageFromResource(nullptr, MAKEINTRESOURCE(IDB_PNG1), _T("PNG"));
	pStream2 = LoadImageFromResource(nullptr, MAKEINTRESOURCE(IDB_PNG2), _T("PNG"));

	// Application initialization
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	// Use shared cached image to reduce memory overhead
	g_pWnd->CacheImage(_T("ItemImage1"), pStream1);
	g_pWnd->CacheImage(_T("ItemImage2"), pStream2);

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_STXANIMATEDTREETEST));

	// Message loop
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	delete g_pWnd;

	if(pStreamBK) pStreamBK->Release();
	if(pStreamMail) pStreamMail->Release();
	if(pStream1) pStream1->Release();
	if(pStream2) pStream2->Release();

	Gdiplus::GdiplusShutdown(m_gdiplusToken);
	CoUninitialize();

	return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_STXANIMATEDTREETEST));
	wcex.hCursor		= LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	//wcex.hbrBackground	= nullptr;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_STXANIMATEDTREETEST);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Save to global variable

#ifdef USING_NS_VERSION
   CSTXAnimatedTreeCtrlNS::RegisterAnimatedTreeCtrlClass();
#else
   CSTXAnimatedTreeCtrl::RegisterAnimatedTreeCtrlClass();
#endif


   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
	   CW_USEDEFAULT, 0, 640, 600, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
	   return FALSE;
   }

#ifdef USING_NS_VERSION
   g_pWnd = new CSTXAnimatedTreeCtrlNS();
#else
   g_pWnd = new CSTXAnimatedTreeCtrl();
#endif
   g_pWnd->Create(szTitle, WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER,
	   0, 0, 320, 520, hWnd, 600);

   _anchor = new CSTXAnchor(hWnd);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   _anchor->AddItem(g_pWnd->GetSafeHwnd(), STXANCHOR_ALL);

   //g_pWnd->ModifyStyle(0, STXTVS_ALWAYS_SHOW_EXPANDER|STXTVS_NO_LINES);
   g_pWnd->ModifyStyle(0, STXTVS_HORIZONTAL_SCROLL);

   g_pWnd->Internal_SetBackgroundImage(pStreamBK);

   //g_pWnd->Internal_SetWatermarkImage(pStreamWatermark);		// A small custom image appears on the right-bottom. Use your own watermark if you want.
   //g_pWnd->SetWatermarkOpacity(0.4f);

   STXTVSORTFUNC func;
   func.lParamSort = 0;
   func.pfnSortFunc = STXTestSortFunc;
   g_pWnd->Internal_SetDefaultSortFunction(&func);

   IStream *pImgExpanded = LoadImageFromResource(nullptr, MAKEINTRESOURCE(IDB_PNG_EXPANDED), _T("PNG"));
   IStream *pImgCollapsed = LoadImageFromResource(nullptr, MAKEINTRESOURCE(IDB_PNG_COLLAPSED), _T("PNG"));
   g_pWnd->Internal_SetExpanderImage(pImgExpanded, pImgCollapsed);
   if(pImgExpanded)	pImgExpanded->Release();
   if(pImgCollapsed)	pImgCollapsed->Release();

   CreateWindow(_T("BUTTON"), _T("Add Root Node"), WS_CHILD|WS_VISIBLE, 352, 0, 240, 24, hWnd, (HMENU)1001, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("Add Child at Level 1"), WS_CHILD|WS_VISIBLE, 352, 30, 240, 24, hWnd, (HMENU)1002, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("Add Child at Level 2"), WS_CHILD|WS_VISIBLE, 352, 60, 240, 24, hWnd, (HMENU)1003, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("Add Child at Level 3"), WS_CHILD|WS_VISIBLE, 352, 90, 240, 24, hWnd, (HMENU)1004, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("Play Default Demo Again"), WS_CHILD|WS_VISIBLE, 352, 120, 240, 24, hWnd, (HMENU)1005, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("SetSelectionImage"), WS_CHILD|WS_VISIBLE, 352, 150, 240, 24, hWnd, (HMENU)1006, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("ClearSelectionImage"), WS_CHILD|WS_VISIBLE, 352, 180, 240, 24, hWnd, (HMENU)1007, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("Batch Insert"), WS_CHILD|WS_VISIBLE, 352, 210, 240, 24, hWnd, (HMENU)1008, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("Delete Selection"), WS_CHILD|WS_VISIBLE, 352, 240, 240, 24, hWnd, (HMENU)1009, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("Delete All (Clear)"), WS_CHILD|WS_VISIBLE, 352, 270, 240, 24, hWnd, (HMENU)1010, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("Sort Children of Selection"), WS_CHILD|WS_VISIBLE, 352, 300, 240, 24, hWnd, (HMENU)1011, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("SetSelection SubImage"), WS_CHILD|WS_VISIBLE, 352, 330, 240, 24, hWnd, (HMENU)1012, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("Always Show Expander"), WS_CHILD|WS_VISIBLE, 352, 360, 240, 24, hWnd, (HMENU)1013, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("Toggle Custom Draw"), WS_CHILD|WS_VISIBLE, 352, 390, 240, 24, hWnd, (HMENU)1014, nullptr, nullptr);
   CreateWindow(_T("BUTTON"), _T("Set SubText for Selected Node"), WS_CHILD|WS_VISIBLE, 352,420, 240, 24, hWnd, (HMENU)1015, nullptr, nullptr);

   for(UINT btnId = 1001; btnId <= 1015; btnId++)
		_anchor->AddItem(btnId, STXANCHOR_RIGHT|STXANCHOR_TOP);

   SetTimer(hWnd, 1, 120, nullptr);

   return TRUE;
}

int CALLBACK STXTestSearchCompareFunc(LPARAM lParamItem, LPARAM lParamFind)
{
	if(lParamItem == lParamFind)
		return 0;

	return -1;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_TIMER:
		iTimerStep++;
		if (iTimerStep == 1) g_pWnd->Internal_InsertItem(_T("Root 1. Animated Tree Control Demo by Huan Xia"));
		if (iTimerStep == 2)
		{
			g_NodeParent = g_pWnd->Internal_InsertItem(_T("Root 2. Windows 7/8/10/11 supported."));
			g_pWnd->Internal_SetItemFullRowBackground(g_NodeParent, TRUE);
		}
		if (iTimerStep == 3) g_NodeToSetImage[0] = g_pWnd->Internal_InsertItem(_T("Root 3. "));
		if (iTimerStep == 20)g_pWnd->Internal_InsertItem(_T("Root 0"), STXTVI_ROOT, STXTVI_FIRST);
		if (iTimerStep == 4) g_NodeToSetImage[1] = g_pWnd->Internal_InsertItem(_T("Child 2.1"), g_NodeParent);
		if (iTimerStep == 5) g_NodeC1Parent = g_pWnd->Internal_InsertItem(_T("Child 2.2"), g_NodeParent);
		if (iTimerStep == 6)
		{
			g_NodeToSetImage[2] = g_pWnd->Internal_InsertItem(_T("Child 2.3 Custom Item Background"), g_NodeParent);
			g_pWnd->Internal_SetItemBackgroundColor(g_NodeToSetImage[2], RGB(255, 128, 128), 64);
		}
		if (iTimerStep == 19) g_pWnd->Internal_InsertItem(_T("Child 2.4"), g_NodeParent);
		if (iTimerStep == 7)
		{
			g_NodeToSetImage[3] = g_NodeC2Parent = g_pWnd->Internal_InsertItem(_T("Child 2.2.1 Full Row Background"), g_NodeC1Parent);
			g_pWnd->Internal_SetItemFullRowBackground(g_NodeToSetImage[3], TRUE);
		}
		if (iTimerStep == 8) g_NodeToSetImage[7] = g_pWnd->Internal_InsertItem(_T("Child 2.2.2"), g_NodeC1Parent);
		if (iTimerStep == 9) g_NodeToSetImage[4] = g_pWnd->Internal_InsertItem(_T("Child 2.2.3"), g_NodeC1Parent);
		if (iTimerStep == 10) g_pWnd->Internal_InsertItem(_T("Child 2.2.1.1"), g_NodeC2Parent);
		if (iTimerStep == 11) g_NodeToSetImage[5] = g_NodeC3Parent = g_pWnd->Internal_InsertItem(_T("Child 2.2.1.2"), g_NodeC2Parent);
		if (iTimerStep == 12) g_NodeToSetImage[6] = g_pWnd->Internal_InsertItem(_T("Child 2.2.1.3"), g_NodeC2Parent);

		//if (iTimerStep == 13) g_pWnd->SetItemImage(g_NodeToSetImage[0], pStream1);
		//if (iTimerStep == 14) g_pWnd->SetItemImage(g_NodeToSetImage[1], pStream1);
		//if (iTimerStep == 15) g_pWnd->SetItemImage(g_NodeToSetImage[2], pStream1);
		//if (iTimerStep == 16) g_pWnd->SetItemImage(g_NodeToSetImage[3], pStream1);
		//if (iTimerStep == 17) g_pWnd->SetItemImage(g_NodeToSetImage[4], pStream1);

		// Use shared cached image to reduce memory overhead
		if (iTimerStep == 13) g_pWnd->SetItemImageKey(g_NodeToSetImage[0], _T("ItemImage1"));
		if (iTimerStep == 14) g_pWnd->SetItemImageKey(g_NodeToSetImage[1], _T("ItemImage1"));
		if (iTimerStep == 15) g_pWnd->SetItemImageKey(g_NodeToSetImage[2], _T("ItemImage1"));
		if (iTimerStep == 16) g_pWnd->SetItemImageKey(g_NodeToSetImage[3], _T("ItemImage1"));
		if (iTimerStep == 17) g_pWnd->SetItemImageKey(g_NodeToSetImage[4], _T("ItemImage1"));

		if (iTimerStep == 18) g_pWnd->Internal_InsertItem(_T("Root 4. Note: MSAA supported!"));
		//if (iTimerStep == 21) g_pWnd->SetItemImage(g_NodeToSetImage[5], pStream1);
		//if (iTimerStep == 22) g_pWnd->SetItemImage(g_NodeToSetImage[6], pStream2);
		//if (iTimerStep == 23) g_pWnd->SetItemImage(g_NodeToSetImage[7], pStream2);
		
		// Use shared cached image to reduce memory overhead
		if (iTimerStep == 21) g_pWnd->SetItemImageKey(g_NodeToSetImage[5], _T("ItemImage1"));
		if (iTimerStep == 22) g_pWnd->SetItemImageKey(g_NodeToSetImage[6], _T("ItemImage2"));
		if (iTimerStep == 23) g_pWnd->SetItemImageKey(g_NodeToSetImage[7], _T("ItemImage2"));

		if (iTimerStep == 24)
		{
			g_NodeToSetImage[8] = g_pWnd->Internal_InsertItem(_T("Contact me: huxia1124@gmail.com"), g_NodeC3Parent, STXTVI_LAST, 0, 48);
			g_pWnd->Internal_SetItemFullRowBackground(g_NodeToSetImage[8], TRUE);
		}
		if (iTimerStep == 25) g_pWnd->SetItemImage(g_NodeToSetImage[8], pStreamMail);

		if (iTimerStep >= 25)
		{
			KillTimer(hWnd, 1);
			g_pWnd->Internal_SetItemData(g_NodeToSetImage[0], 100);
			g_pWnd->Internal_ModifyItemStyle(g_NodeToSetImage[8], STXTVIS_HANDCURSOR, 0);
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		}

		if(HIWORD(wParam) == BN_CLICKED)
		{
			switch(LOWORD(wParam))
			{
			case 1001:
				g_NodeParent = g_pWnd->Internal_InsertItem(_T("C - New Root"), STXTVI_ROOT, STXTVI_SORT, 2600);
				break;
			case 1002:
				g_NodeC1Parent = g_pWnd->Internal_InsertItem(_T("C - New Child"), g_NodeParent);
				break;
			case 1003:
				g_NodeC2Parent = g_pWnd->Internal_InsertItem(_T("C - New Child^2"), g_NodeC1Parent, STXTVI_FIRST);
				break;
			case 1004:
				g_NodeC3Parent = g_pWnd->Internal_InsertItem(_T("C - New Child^3"), g_NodeC2Parent, STXTVI_FIRST);
				break;
			case 1005:		// * Do Default Demo Again
				iTimerStep = 0;
				SetTimer(hWnd, 1, 50, nullptr);
				break;
			case 1006:
				{
					HSTXTREENODE hNode = g_pWnd->GetSelectedItem();
					g_pWnd->SetItemImageKey(hNode, _T("ItemImage1"));
					g_pWnd->Internal_SetItemSubText(hNode, _T("5"));
				}
				break;
			case 1007:
				{
					HSTXTREENODE hNode = g_pWnd->GetSelectedItem();
					g_pWnd->SetItemImage(hNode, (IStream*)nullptr);
				}
				break;
			case 1008:		//Batch Insert
				{
					HSTXTREENODE hNode = g_pWnd->Internal_InsertItem(GetRandomString(), STXTVI_ROOT, STXTVI_LAST, 1000);
					g_pWnd->Internal_Expand(hNode, STXATVE_COLLAPSE);
 					HSTXTREENODE hNode2 = g_pWnd->Internal_InsertItem(GetRandomString(), STXTVI_ROOT, STXTVI_LAST, 2000);
					g_pWnd->Internal_Expand(hNode2, STXATVE_COLLAPSE);
					HSTXTREENODE hNode3 = g_pWnd->Internal_InsertItem(GetRandomString(), STXTVI_ROOT, STXTVI_LAST, 3000);
					g_pWnd->Internal_Expand(hNode3, STXATVE_COLLAPSE);

 					g_pWnd->Internal_InsertItem(GetRandomString(), hNode2, STXTVI_FIRST, 0);
 					g_pWnd->Internal_InsertItem(GetRandomString(), hNode2, STXTVI_LAST, 0);

					g_pWnd->SetItemImageKey(hNode, _T("ItemImage1"));
					g_pWnd->SetItemImageKey(hNode2, _T("ItemImage1"));
					g_pWnd->SetItemImageKey(hNode3, _T("ItemImage1"));

// 					g_pWnd->SetItemImageKey(hNode3, _T("ItemImage1"));
// 					g_pWnd->SetItemImageKey(hNode2, _T("ItemImage2"));
				}
				break;
			case 1009:	//Delete Selection
				{
					HSTXTREENODE hNode = g_pWnd->GetSelectedItem();
					g_pWnd->Internal_DeleteItem(hNode);
					g_NodeParent = STXTVI_ROOT;
					g_NodeC1Parent = STXTVI_ROOT;
					g_NodeC2Parent = STXTVI_ROOT;
					g_NodeC3Parent = STXTVI_ROOT;
					memset(g_NodeToSetImage, 0, sizeof(g_NodeToSetImage));
				}
				break;
			case 1010:
				{
					g_pWnd->Internal_DeleteItem(STXTVI_ROOT);
					g_NodeParent = STXTVI_ROOT;
					g_NodeC1Parent = STXTVI_ROOT;
					g_NodeC2Parent = STXTVI_ROOT;
					g_NodeC3Parent = STXTVI_ROOT;
					memset(g_NodeToSetImage, 0, sizeof(g_NodeToSetImage));
				}
				break;
			case 1011:		//Sort
				{
 					HSTXTREENODE hNode = g_pWnd->GetSelectedItem();
 					//g_pWnd->Internal_SetItemHeight(hNode, rand() % 30 + 16);

					if(hNode == nullptr)
						hNode = STXTVI_ROOT;
 
 					g_pWnd->Internal_SortChildrenByItem(hNode, STXTestSortItemFunc, 100);
 					iFactor = -1 * iFactor;

//  					int n = g_pWnd->Internal_FindItems(STXTVI_ROOT, STXTestSearchCompareFunc, 0, nullptr, 0);
//  					if(n > 0)
//  					{
//  						HSTXTREENODE *pOut = new HSTXTREENODE[n];
//  						g_pWnd->Internal_FindItems(STXTVI_ROOT, STXTestSearchCompareFunc, 0, pOut, n);
//  						for(int i=0;i<n;i++)
//  						{
//  							//if(!g_pWnd->Internal_ItemHasChildren(pOut[i]))
//  								g_pWnd->Internal_DeleteItem(pOut[i]);
//  						}
//  						delete []pOut;
//  					}
				}
				break;
			case 1012:
				{
					HSTXTREENODE hNode = g_pWnd->GetSelectedItem();
					IStream *pSubImage = LoadImageFromResource(nullptr, MAKEINTRESOURCE(IDB_PNG_SUB_IMAGE), _T("PNG"));
					g_pWnd->SetItemSubImage(hNode, pSubImage);
					if(pSubImage)	pSubImage->Release();
				}
				break;
			case 1013:
				{
					HSTXTREENODE hNode = g_pWnd->GetSelectedItem();
					g_pWnd->Internal_ModifyItemStyle(hNode, STXTVIS_FORCE_SHOW_EXPANDER, 0);
				}
				break;
			case 1014:
				{
					static bool bCustomDraw = false;

					bCustomDraw = !bCustomDraw;
					if (bCustomDraw)
						g_pWnd->Internal_SetItemDrawFunction(MySTXAnimatedTreeItemDrawFunc);
					else
						g_pWnd->Internal_SetItemDrawFunction(nullptr);
				}
				break;
			case 1015:
				{
				TCHAR szSubText[16];
				_stprintf_s(szSubText, _T("%d"), rand() % 100);
				HSTXTREENODE hNode = g_pWnd->GetSelectedItem();
				g_pWnd->Internal_SetItemSubText(hNode, szSubText);
				}
				break;
			}
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_ERASEBKGND:
		{
			RECT rcClient;
			::GetClientRect(hWnd, &rcClient);
			HDC hDC = (HDC)wParam;
			RECT rcControl;
			GetWindowRect(g_pWnd->GetSafeHwnd(), &rcControl);
			ScreenToClient(hWnd, ((LPPOINT)&rcControl));
			ScreenToClient(hWnd, ((LPPOINT)&rcControl) + 1);
			ExcludeClipRect(hDC, rcControl.left, rcControl.top, rcControl.right, rcControl.bottom);
			::FillRect(hDC, &rcClient, (HBRUSH)::GetStockObject(WHITE_BRUSH));
			return 0;
		}
		break;
	case WM_DESTROY:
		::DestroyWindow(g_pWnd->GetSafeHwnd());
		delete g_pWnd;
		g_pWnd = nullptr;
		delete _anchor;
		PostQuitMessage(0);
		break;
	case WM_LBUTTONDOWN:
		break;
	case WM_NOTIFY:
		{
			LPNMHDR pNMHDR = (LPNMHDR)lParam;
			switch(pNMHDR->code)
			{
			case STXATVN_ITEMDBLCLICK:
				{
					LPSTXATVNITEM pNM = reinterpret_cast<LPSTXATVNITEM>(pNMHDR);
					TCHAR szBuf[200];
					g_pWnd->Internal_GetItemText(pNM->hNode, szBuf, 200);
					MessageBox(hWnd, szBuf, _T("Double Click"), MB_OK);
				}
				break;
			}
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

