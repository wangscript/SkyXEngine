

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <windowsx.h>

#include "resource.h"
#include <commctrl.h>
#include <combaseapi.h>

#include <core/sxcore.h>
#include <gcore/sxgcore.h>
#include <render/sxrender.h>
#include <input/sxinput.h>

#include "terrax.h"

// Global Variables:
HINSTANCE hInst;								// current instance
HWND g_hWndMain = NULL;
HWND g_hTopRightWnd = NULL;
HWND g_hTopLeftWnd = NULL;
HWND g_hBottomRightWnd = NULL;
HWND g_hBottomLeftWnd = NULL;
HWND g_hStatusWnd = NULL;

HWND g_hABArrowButton = NULL;
HWND g_hABCameraButton = NULL;

X_VIEWPORT_LAYOUT g_xviewportLayout = XVIEW_2X2;
BOOL g_isXResizeable = TRUE;
BOOL g_isYResizeable = TRUE;

BOOL g_is3DRotating = FALSE;
BOOL g_is3DPanning = FALSE;

extern HACCEL g_hAccelTable;

extern float g_fTopRightScale;
extern float g_fBottomLeftScale;
extern float g_fBottomRightScale;

HMENU g_hMenu = NULL;

ICamera *g_pTopRightCamera = NULL;
ICamera *g_pBottomLeftCamera = NULL;
ICamera *g_pBottomRightCamera = NULL;

X_2D_VIEW g_x2DTopRightView = X2D_TOP;
X_2D_VIEW g_x2DBottomLeftView = X2D_SIDE;
X_2D_VIEW g_x2DBottomRightView = X2D_FRONT;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RenderWndProc(HWND, UINT, WPARAM, LPARAM);
void DisplayContextMenu(HWND hwnd, POINT pt, int iMenu, int iSubmenu, int iCheckItem = -1);
void XInitViewportLayout(X_VIEWPORT_LAYOUT layout);


ATOM XRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	HBRUSH	hBrush = NULL;

	// Resetting the structure members before use
	memset(&wcex, 0, sizeof(WNDCLASSEX));

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SPLITTER_WND));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);

	hBrush = CreateSolidBrush(RGB(240, 240, 240));


	wcex.hbrBackground = (HBRUSH)hBrush;
	//	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_SPLITTER_WND);
	wcex.lpszClassName = MAIN_WINDOW_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if(!RegisterClassEx(&wcex))
	{
		return(FALSE);
	}

	// Intilaizing the class for the viewport window
	wcex.lpfnWndProc = RenderWndProc;
	hBrush = CreateSolidBrush(RGB(255, 255, 255));
	wcex.hbrBackground = (HBRUSH)hBrush;
	wcex.lpszClassName = RENDER_WINDOW_CLASS;

	if(!RegisterClassEx(&wcex))
	{
		return(FALSE);
	}
	
	DeleteObject(hBrush);

	return(TRUE);
}

BOOL XInitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	// Varible used to postion the window, center to the desktop
	UINT nx_size = GetSystemMetrics(SM_CXSCREEN);
	UINT ny_size = GetSystemMetrics(SM_CYSCREEN);

	UINT nx_pos = (nx_size - WINDOW_WIDTH) / 2;
	UINT ny_pos = (ny_size - WINDOW_HEIGHT) / 2;

	g_hWndMain = CreateWindowA(MAIN_WINDOW_CLASS, "TerraX", WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_OVERLAPPEDWINDOW, nx_pos, ny_pos, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, g_hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1)), hInstance, NULL);

	if(!g_hWndMain)
	{
		UINT ret_val;

		ret_val = GetLastError();
		return FALSE;
	}

	ShowWindow(g_hWndMain, nCmdShow);
	UpdateWindow(g_hWndMain);

	g_hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	return TRUE;
}



LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rect;
	PAINTSTRUCT ps;
	HDC hdc;
	static HCURSOR	hcSizeEW = NULL;
	static HCURSOR	hcSizeNS = NULL;
	static HCURSOR	hcSizeNESW = NULL;

	

	static BOOL xSizing;
	static BOOL ySizing;


	static float fCoeffWidth = 0.0f;
	static float fCoeffHeight = 0.0f;
	static int iLeftWidth = 0;
	static int iTopHeight = 0;
	
	IMSG msg;
	msg.lParam = lParam;
	msg.wParam = wParam;
	msg.message = message;

	SSInput_AddMsg(msg);

	switch(message)
	{
	case WM_CREATE:
		hcSizeEW = LoadCursor(NULL, IDC_SIZEWE);
		hcSizeNS = LoadCursor(NULL, IDC_SIZENS);
		hcSizeNESW = LoadCursor(NULL, IDC_SIZEALL);

	//	g_hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU1));
	//	SetMenu(g_hWndMain, g_hMenu); 
	//	DrawMenuBar(g_hWndMain);


		GetClientRect(hWnd, &rect);

		rect.top += MARGIN_TOP;
		rect.bottom -= MARGIN_BOTTOM;
		rect.left += MARGIN_LEFT;
		rect.right -= MARGIN_RIGHT;

		iLeftWidth = (rect.right - rect.left - SPLITTER_BAR_WIDTH) / 2;
		iTopHeight = (rect.bottom - rect.top - SPLITTER_BAR_WIDTH) / 2;

		fCoeffWidth = (float)iLeftWidth / (float)(rect.right - rect.left);
		fCoeffHeight = (float)iTopHeight / (float)(rect.bottom - rect.top);

		// Creates the left window using the width and height read from the XML 
		// files
		g_hTopLeftWnd = CreateWindowExA(WS_EX_CLIENTEDGE, RENDER_WINDOW_CLASS, "", WS_CHILD | WS_VISIBLE | SS_SUNKEN, rect.left, rect.top, iLeftWidth, iTopHeight, hWnd, NULL, hInst, NULL);
		if(g_hTopLeftWnd)
		{
			ShowWindow(g_hTopLeftWnd, SW_SHOW);
			//	UpdateWindow(g_hTopLeftWnd);
		}

		g_hBottomLeftWnd = CreateWindowExA(WS_EX_CLIENTEDGE, RENDER_WINDOW_CLASS, "", WS_CHILD | WS_VISIBLE | SS_SUNKEN, rect.left, rect.top + iTopHeight + SPLITTER_BAR_WIDTH, iLeftWidth, iTopHeight, hWnd, NULL, hInst, NULL);
		if(g_hTopLeftWnd)
		{
			ShowWindow(g_hTopLeftWnd, SW_SHOW);
			//	UpdateWindow(g_hTopLeftWnd);
		}

		g_hTopRightWnd = CreateWindowExA(WS_EX_CLIENTEDGE, RENDER_WINDOW_CLASS, "", WS_CHILD | WS_VISIBLE | SS_SUNKEN, rect.left + iLeftWidth + SPLITTER_BAR_WIDTH, rect.top, iLeftWidth, iTopHeight, hWnd, NULL, hInst, NULL);
		if(g_hTopRightWnd)
		{
			ShowWindow(g_hTopRightWnd, SW_SHOW);
			//	UpdateWindow(g_hTopLeftWnd);
		}

		g_hBottomRightWnd = CreateWindowExA(WS_EX_CLIENTEDGE, RENDER_WINDOW_CLASS, "", WS_CHILD | WS_VISIBLE | SS_SUNKEN, rect.left + iLeftWidth + SPLITTER_BAR_WIDTH, rect.top + iTopHeight + SPLITTER_BAR_WIDTH, iLeftWidth, iTopHeight, hWnd, NULL, hInst, NULL);
		if(g_hBottomRightWnd)
		{
			ShowWindow(g_hBottomRightWnd, SW_SHOW);
			//	UpdateWindow(g_hTopLeftWnd);
		}

		g_hStatusWnd = CreateWindowEx(0, STATUSCLASSNAME, (PCTSTR)"For help, press F1", SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, (HMENU)IDC_STATUSBAR, hInst, NULL);                   // no window creation data


		g_hABArrowButton = CreateWindow("Button", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_BITMAP | BS_PUSHLIKE | BS_CHECKBOX, rect.left - MARGIN_LEFT, rect.top, MARGIN_LEFT, AB_BUTTON_HEIGHT, hWnd, (HMENU)IDC_AB_ARROW, hInst, NULL);
		{
			HBITMAP hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP30));
			SendMessage(g_hABArrowButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
		}

		g_hABCameraButton = CreateWindow("Button", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_BITMAP | BS_PUSHLIKE | BS_CHECKBOX, rect.left - MARGIN_LEFT, rect.top + AB_BUTTON_HEIGHT, MARGIN_LEFT, AB_BUTTON_HEIGHT, hWnd, (HMENU)IDC_AB_CAMERA, hInst, NULL);
		{
			HBITMAP hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP31));
			SendMessage(g_hABCameraButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
		}
		return FALSE;
		break;

	case WM_ENTERSIZEMOVE:
		GetClientRect(hWnd, &rect);

		rect.top += MARGIN_TOP;
		rect.bottom -= MARGIN_BOTTOM;
		rect.left += MARGIN_LEFT;
		rect.right -= MARGIN_RIGHT;

		fCoeffWidth = (float)iLeftWidth / (float)(rect.right - rect.left);
		fCoeffHeight = (float)iTopHeight / (float)(rect.bottom - rect.top);
		break;

	case WM_SIZE:

		GetClientRect(hWnd, &rect);

		rect.top += MARGIN_TOP;
		rect.bottom -= MARGIN_BOTTOM;
		rect.left += MARGIN_LEFT;
		rect.right -= MARGIN_RIGHT;

		if(!(xSizing || ySizing))
		{
			RECT rcTopLeft, rcBottomRight;
			GetClientRect(g_hTopLeftWnd, &rcTopLeft);
			GetClientRect(g_hBottomRightWnd, &rcBottomRight);

			UINT uLeftWidth = rcTopLeft.right - rcTopLeft.left;
			UINT uRightWidth = rcBottomRight.right - rcBottomRight.left;
			UINT uTopHeight = rcTopLeft.bottom - rcTopLeft.top;
			UINT uBottomHeight = rcBottomRight.bottom - rcBottomRight.top;

			if(fCoeffHeight && uTopHeight + uBottomHeight + SPLITTER_BAR_WIDTH + WIDTH_ADJUST * 4 != rect.bottom - rect.top)
			{
				iTopHeight = (UINT)(fCoeffHeight * (float)(rect.bottom - rect.top));
			}
			if(fCoeffWidth && uLeftWidth + uRightWidth + SPLITTER_BAR_WIDTH + WIDTH_ADJUST * 4 != rect.right - rect.left)
			{
				iLeftWidth = (UINT)(fCoeffWidth * (float)(rect.right - rect.left));
			}
		}
		MoveWindow(g_hTopLeftWnd, rect.left, rect.top, iLeftWidth, iTopHeight, FALSE);
		MoveWindow(g_hBottomLeftWnd, rect.left, rect.top + iTopHeight + SPLITTER_BAR_WIDTH, iLeftWidth, rect.bottom - rect.top - SPLITTER_BAR_WIDTH - iTopHeight, FALSE);
		MoveWindow(g_hTopRightWnd, rect.left + iLeftWidth + SPLITTER_BAR_WIDTH, rect.top, rect.right - rect.left - iLeftWidth - SPLITTER_BAR_WIDTH, iTopHeight, FALSE);
		MoveWindow(g_hBottomRightWnd, rect.left + iLeftWidth + SPLITTER_BAR_WIDTH, rect.top + iTopHeight + SPLITTER_BAR_WIDTH, rect.right - rect.left - iLeftWidth - SPLITTER_BAR_WIDTH, rect.bottom - rect.top - SPLITTER_BAR_WIDTH - iTopHeight, FALSE);

		InvalidateRect(hWnd, &rect, TRUE);

		{
			static int *r_resize = (int*)GET_PCVAR_INT("r_resize");

			if(!r_resize)
			{
				r_resize = (int*)GET_PCVAR_INT("r_resize");
			}

			*r_resize = 1;
		}

		SendMessage(g_hStatusWnd, WM_SIZE, wParam, lParam);
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_FILE_OPEN:
			//MessageBox(NULL, "Open", "", MB_OK);
			break;

		case ID_VIEW_AUTOSIZEVIEWS:

			GetClientRect(hWnd, &rect);

			rect.top += MARGIN_TOP;
			rect.bottom -= MARGIN_BOTTOM;
			rect.left += MARGIN_LEFT;
			rect.right -= MARGIN_RIGHT;

			if(g_isXResizeable)
			{
				iLeftWidth = (rect.right - rect.left - SPLITTER_BAR_WIDTH) / 2;
			}
			else
			{
				iLeftWidth = rect.right - rect.left;
			}
			if(g_isYResizeable)
			{
				iTopHeight = (rect.bottom - rect.top - SPLITTER_BAR_WIDTH) / 2;
			}
			else
			{
				iTopHeight = rect.bottom - rect.top;
			}

			fCoeffWidth = (float)iLeftWidth / (float)(rect.right - rect.left);
			fCoeffHeight = (float)iTopHeight / (float)(rect.bottom - rect.top);
			SendMessage(hWnd, WM_SIZE, 0, 0);
			break;

		case ID_VEWPORTCONFIGURATION_1X2:
			XInitViewportLayout(XVIEW_1X2);
			break;
		case ID_VEWPORTCONFIGURATION_2X2:
			XInitViewportLayout(XVIEW_2X2);
			break;
		case ID_VEWPORTCONFIGURATION_2X1:
			XInitViewportLayout(XVIEW_2X1);
			break;
		case ID_VEWPORTCONFIGURATION_3DONLY:
			XInitViewportLayout(XVIEW_3DONLY);
			break;

		case IDC_AB_ARROW:
			CheckDlgButton(hWnd, IDC_AB_ARROW, TRUE);
			CheckDlgButton(hWnd, IDC_AB_CAMERA, FALSE);
			break;

		case IDC_AB_CAMERA:
			CheckDlgButton(hWnd, IDC_AB_ARROW, FALSE);
			CheckDlgButton(hWnd, IDC_AB_CAMERA, TRUE);
			break;
		}
		break;

	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_TAB:
			/*if(focus == hcolor_button)
			{
			SendMessage(focus, WM_KILLFOCUS, 0, 0);
			SendMessage(hclose_button, WM_SETFOCUS, 0, 0);

			focus = hclose_button;
			}
			else if(focus == hclose_button)
			{
			SendMessage(focus, WM_KILLFOCUS, 0, 0);
			SendMessage(hcolor_button, WM_SETFOCUS, 0, 0);


			focus = hcolor_button;
			}*/


			break;

		case VK_RETURN:

			/*if(IsWindowEnabled(focus))
			SendMessage(hWnd, WM_COMMAND, 0, (LPARAM)focus);*/

			break;
		}
		break;

/*
	case WM_PAINT:

		hdc = BeginPaint(hWnd, &ps);

		// Painting 
		GetClientRect(hWnd, &rect);

		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);

		break;
*/
		// Case statement to handle the left mouse button down message
		// received while the mouse left button is down
	case WM_LBUTTONDOWN:
	{
		int                 xPos;
		int                 yPos;

		// Varible used to get the mouse cursor x and y co-ordinates
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);

		// Checks whether the mouse is over the splitter window
		xSizing = g_isXResizeable && (xPos > iLeftWidth + MARGIN_LEFT - SPLITTER_BAR_WIDTH && xPos < iLeftWidth + MARGIN_LEFT + SPLITTER_BAR_WIDTH);
		ySizing = g_isYResizeable && (yPos > iTopHeight + MARGIN_TOP - SPLITTER_BAR_WIDTH && yPos < iTopHeight + MARGIN_TOP + SPLITTER_BAR_WIDTH);

		if(xSizing || ySizing)
		{
			// Api to capture mouse input
			SetCapture(hWnd);
			if(xSizing && ySizing)
			{
				SetCursor(hcSizeNESW);
			}
			else if(xSizing)
			{
				SetCursor(hcSizeEW);
			}
			else if(ySizing)
			{
				SetCursor(hcSizeNS);
			}
		}
	}
	break;

	case WM_LBUTTONUP:
		if(xSizing || ySizing)
		{
			RECT    focusrect;
			HDC     hdc;

			// Releases the captured mouse input
			ReleaseCapture();
			// Get the main window dc to draw a focus rectangle
			hdc = GetDC(hWnd);
			GetClientRect(hWnd, &rect);

			rect.top += MARGIN_TOP;
			rect.bottom -= MARGIN_BOTTOM;
			rect.left += MARGIN_LEFT;
			rect.right -= MARGIN_RIGHT;

			fCoeffWidth = (float)iLeftWidth / (float)(rect.right - rect.left);
			fCoeffHeight = (float)iTopHeight / (float)(rect.bottom - rect.top);

			if(xSizing)
			{
				SetRect(&focusrect, iLeftWidth - (WIDTH_ADJUST * 2), rect.top, iLeftWidth + WIDTH_ADJUST, rect.bottom);

				// Call api to vanish the dragging rectangle 
				DrawFocusRect(hdc, &focusrect);

				xSizing = FALSE;
			}
			if(ySizing)
			{
				SetRect(&focusrect, rect.left, iTopHeight - (WIDTH_ADJUST * 2), rect.right, iTopHeight + WIDTH_ADJUST);

				// Call api to vanish the dragging rectangle 
				DrawFocusRect(hdc, &focusrect);

				ySizing = FALSE;
			}
			// Release the dc once done 
			ReleaseDC(hWnd, hdc);
			// Post a WM_SIZE message to redraw the windows
			PostMessage(hWnd, WM_SIZE, 0, 0);
		}

		break;

	case WM_MOUSEMOVE:
	{
		int   xPos;
		int   yPos;

		// Get the x and y co-ordinates of the mouse

		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);

		GetClientRect(hWnd, &rect);

		rect.top += MARGIN_TOP;
		rect.bottom -= MARGIN_BOTTOM;
		rect.left += MARGIN_LEFT;
		rect.right -= MARGIN_RIGHT;

		if(xPos < rect.left + SPLITTER_BAR_WIDTH * 3)
		{
			xPos = rect.left + SPLITTER_BAR_WIDTH * 3;
		}
		if(xPos > rect.right - SPLITTER_BAR_WIDTH * 3)
		{
			xPos = rect.right - SPLITTER_BAR_WIDTH * 3;
		}
		if(yPos < rect.top + SPLITTER_BAR_WIDTH * 3)
		{
			yPos = rect.top + SPLITTER_BAR_WIDTH * 3;
		}
		if(yPos > rect.bottom - SPLITTER_BAR_WIDTH * 3)
		{
			yPos = rect.bottom - SPLITTER_BAR_WIDTH * 3;
		}

		// Checks if the left button is pressed during dragging the splitter
		if(wParam == MK_LBUTTON)
		{
			// If the window is d`agged using the splitter, get the
			// cursors current postion and draws a focus rectangle 
			if(xSizing)
			{
				RECT    focusrect;
				HDC     hdc;

				hdc = GetDC(hWnd);


				SetRect(&focusrect, iLeftWidth + MARGIN_LEFT - (WIDTH_ADJUST * 2), rect.top, iLeftWidth + MARGIN_LEFT + WIDTH_ADJUST, rect.bottom);

				DrawFocusRect(hdc, &focusrect);

				// Get the size of the left window to increase
				iLeftWidth = xPos - MARGIN_LEFT;

				// Draws a focus rectangle
				SetRect(&focusrect, iLeftWidth + MARGIN_LEFT - (SPLITTER_BAR_WIDTH * 2), rect.top, iLeftWidth + MARGIN_LEFT + SPLITTER_BAR_WIDTH, rect.bottom);

				DrawFocusRect(hdc, &focusrect);

				ReleaseDC(hWnd, hdc);
			}

			if(ySizing)
			{
				RECT    focusrect;
				HDC     hdc;
				hdc = GetDC(hWnd);
				SetRect(&focusrect, rect.left, iTopHeight + MARGIN_TOP - (WIDTH_ADJUST * 2), rect.right, iTopHeight + MARGIN_TOP + WIDTH_ADJUST);
				DrawFocusRect(hdc, &focusrect);
				iTopHeight = yPos - MARGIN_TOP;
				SetRect(&focusrect, rect.left, iTopHeight + MARGIN_TOP - (SPLITTER_BAR_WIDTH * 2), rect.right, iTopHeight + MARGIN_TOP + SPLITTER_BAR_WIDTH);
				DrawFocusRect(hdc, &focusrect);
				ReleaseDC(hWnd, hdc);
			}
		}
		// Set the cursor image to east west direction when the mouse is over 
		// the splitter window
		BOOL x = g_isXResizeable && (xPos > iLeftWidth + MARGIN_LEFT - SPLITTER_BAR_WIDTH && xPos < iLeftWidth + MARGIN_LEFT + SPLITTER_BAR_WIDTH);
		BOOL y = g_isYResizeable && (yPos > iTopHeight + MARGIN_TOP - SPLITTER_BAR_WIDTH && yPos < iTopHeight + MARGIN_TOP + SPLITTER_BAR_WIDTH);
		if(x && y)
		{
			SetCursor(hcSizeNESW);
		}
		else if(x)
		{
			SetCursor(hcSizeEW);
		}
		else if(y)
		{
			SetCursor(hcSizeNS);
		}
	}
	break;

	case WM_MOUSEWHEEL:
	{
		POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		ScreenToClient(hWnd, &pt);

		GetClientRect(hWnd, &rect);

		rect.top += MARGIN_TOP;
		rect.bottom -= MARGIN_BOTTOM;
		rect.left += MARGIN_LEFT;
		rect.right -= MARGIN_RIGHT;
		if(pt.x < rect.left || pt.x > rect.right || pt.y < rect.top || pt.y > rect.bottom)
		{
			return(TRUE);
		}
		BOOL isLeft = (pt.x < rect.left + iLeftWidth),
			isTop = (pt.y < rect.top + iTopHeight);
		ICamera *pCamera = NULL;
		X_2D_VIEW x2dView;
		HWND hTargetWnd;
		float *pfOldScale = NULL;
		if(isLeft)
		{
			if(isTop)
			{
				// pCamera = g_pTopLeftCamera;
				pCamera = SRender_GetCamera();
				float3 vLook, vPos;
				pCamera->getLook(&vLook);
				pCamera->getPosition(&vPos);
				pCamera->setPosition(&(float3)(vPos + vLook * (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA * 0.5f));
				pCamera = NULL;
				hTargetWnd = g_hTopLeftWnd;
			}
			else
			{
				pCamera = g_pBottomLeftCamera;
				x2dView = g_x2DBottomLeftView;
				hTargetWnd = g_hBottomLeftWnd;
				pfOldScale = &g_fBottomLeftScale;
			}
		}
		else
		{
			if(isTop)
			{
				pCamera = g_pTopRightCamera;
				x2dView = g_x2DTopRightView;
				hTargetWnd = g_hTopRightWnd;
				pfOldScale = &g_fTopRightScale;
			}
			else
			{
				pCamera = g_pBottomRightCamera;
				x2dView = g_x2DBottomRightView;
				hTargetWnd = g_hBottomRightWnd;
				pfOldScale = &g_fBottomRightScale;
			}
		}
		if(pCamera)
		{
			float fNewScale = *pfOldScale - *pfOldScale * (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA * 0.1f;
			RECT rc;
			GetWindowRect(hTargetWnd, &rc);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rc, 2);
			float2 vCenter((float)(rc.left + rc.right) * 0.5f, (float)(rc.top + rc.bottom) * 0.5f);
			float2 vDelta = (float2((float)pt.x, (float)pt.y) - vCenter) * float2(1.0f, -1.0f);
			float2 vWorldPt;
			float3 vCamPos;
			pCamera->getPosition(&vCamPos);
			switch(x2dView)
			{
			case X2D_TOP:
				vWorldPt = float2(vCamPos.x, vCamPos.z);
				break;
			case X2D_FRONT:
				vWorldPt = float2(vCamPos.x, vCamPos.y);
				vDelta.x *= -1;
				break;
			case X2D_SIDE:
				vWorldPt = float2(vCamPos.z, vCamPos.y);
				vDelta.x *= -1;
				break;
			}
			vWorldPt += vDelta * (*pfOldScale - fNewScale);
			switch(x2dView)
			{
			case X2D_TOP:
				pCamera->setPosition(&float3(vWorldPt.x, vCamPos.y, vWorldPt.y));
				break;
			case X2D_FRONT:
				pCamera->setPosition(&float3(vWorldPt.x, vWorldPt.y, vCamPos.z));
				break;
			case X2D_SIDE:
				pCamera->setPosition(&float3(vCamPos.x, vWorldPt.y, vWorldPt.x));
				break;
			}
			*pfOldScale = fNewScale;
		}
		else
		{
			// 3d
		}
		break;
	}

	case WM_DESTROY:
		DestroyMenu(g_hMenu);
		DeleteObject(hcSizeEW);
		DeleteObject(hcSizeNS);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK RenderWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rect;
	static const int *r_final_image = GET_PCVAR_INT("r_final_image");
	if(!r_final_image)
	{
		r_final_image = GET_PCVAR_INT("r_final_image");
	}

	switch(message)
	{
	case WM_CREATE:

		break;

	case WM_RBUTTONUP:
		if(g_is3DPanning)
		{
			SSInput_SetEnable(false);
			ReleaseCapture();
			g_is3DPanning = FALSE;
		}
		break;

	case WM_LBUTTONUP:
	{
		if(g_is3DRotating)
		{
			g_is3DRotating = FALSE;
			SSInput_SetEnable(false);
			ReleaseCapture();
			break;
		}
		POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		GetClientRect(hWnd, &rect);
		if(pt.x > rect.left && pt.x < rect.left + 20
			&& pt.y > rect.top && pt.y < rect.top + 20)
		{
			int iActiveMenu = -1;
			X_2D_VIEW x2dView;
			if(hWnd == g_hTopLeftWnd)
			{
				switch(*r_final_image)
				{
				case DS_RT_COLOR:
					iActiveMenu = ID_3D_TEXTURED;
					break;
				case DS_RT_NORMAL:
					iActiveMenu = ID_3D_NORMALS;
					break;
				case DS_RT_PARAM:
					iActiveMenu = ID_3D_PARAMETERS;
					break;
				case DS_RT_AMBIENTDIFF:
					iActiveMenu = ID_3D_AMBIENTDIFFUSE;
					break;
				case DS_RT_SPECULAR:
					iActiveMenu = ID_3D_SPECULAR;
					break;
				case DS_RT_SCENELIGHT:
					iActiveMenu = ID_3D_LIGHTINGSCENE;
					break;
				}
			}
			else
			{
				if(hWnd == g_hTopRightWnd)
				{
					x2dView = g_x2DTopRightView;
				}
				else if(hWnd == g_hBottomLeftWnd)
				{
					x2dView = g_x2DBottomLeftView;
				}
				else if(hWnd == g_hBottomRightWnd)
				{
					x2dView = g_x2DBottomRightView;
				}
				switch(x2dView)
				{
				case X2D_TOP:
					iActiveMenu = ID_2D_TOP;
					break;
				case X2D_FRONT:
					iActiveMenu = ID_2D_FRONT;
					break;
				case X2D_SIDE:
					iActiveMenu = ID_2D_SIDE;
					break;
				}
			}
			DisplayContextMenu(hWnd, pt, IDR_MENU2, hWnd == g_hTopLeftWnd ? 0 : 1, iActiveMenu);
		}
	}
		break;

	case WM_LBUTTONDOWN:
	{
		if(hWnd != g_hTopLeftWnd || g_is3DPanning)
		{
			break;
		}

		POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		GetClientRect(hWnd, &rect);
		if(pt.x > rect.left && pt.x < rect.left + 20
			&& pt.y > rect.top && pt.y < rect.top + 20)
		{
			break;
		}
		if(Button_GetCheck(g_hABCameraButton))
		{
			g_is3DRotating = TRUE;
			SetCapture(hWnd);
			SSInput_SetEnable(true);
		}
		break;
	}

	case WM_RBUTTONDOWN:
	{
		if(hWnd != g_hTopLeftWnd || g_is3DRotating)
		{
			break;
		}
		if(Button_GetCheck(g_hABCameraButton))
		{
			g_is3DPanning = TRUE;
			SetCapture(hWnd);
			SSInput_SetEnable(true);
		}
		break;
	}

	case WM_MOUSEMOVE:
		if(!g_is3DRotating && !g_is3DPanning)
		{
			return(TRUE);
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_3D_TEXTURED:
			Core_0SetCVarInt("r_final_image", DS_RT_COLOR);
			break;
		case ID_3D_NORMALS:
			Core_0SetCVarInt("r_final_image", DS_RT_NORMAL);
			break;
		case ID_3D_PARAMETERS:
			Core_0SetCVarInt("r_final_image", DS_RT_PARAM);
			break;
		case ID_3D_AMBIENTDIFFUSE:
			Core_0SetCVarInt("r_final_image", DS_RT_AMBIENTDIFF);
			break;
		case ID_3D_SPECULAR:
			Core_0SetCVarInt("r_final_image", DS_RT_SPECULAR);
			break;
		case ID_3D_LIGHTINGSCENE:
			Core_0SetCVarInt("r_final_image", DS_RT_SCENELIGHT);
			break;

		case ID_2D_TOP:
		case ID_2D_FRONT:
		case ID_2D_SIDE:
		{
			ICamera *pTargetCam = NULL;
			X_2D_VIEW *pX2DView = NULL;
			if(hWnd == g_hTopRightWnd)
			{
				pTargetCam = g_pTopRightCamera;
				pX2DView = &g_x2DTopRightView;
			}
			else if(hWnd == g_hBottomLeftWnd)
			{
				pTargetCam = g_pBottomLeftCamera;
				pX2DView = &g_x2DBottomLeftView;
			}
			else if(hWnd == g_hBottomRightWnd)
			{
				pTargetCam = g_pBottomRightCamera;
				pX2DView = &g_x2DBottomRightView;
			}
			switch(LOWORD(wParam))
			{
			case ID_2D_TOP:
				pTargetCam->setPosition(&X2D_TOP_POS);
				pTargetCam->setOrientation(&X2D_TOP_ROT);
				*pX2DView = X2D_TOP;
				break;
			case ID_2D_FRONT:
				pTargetCam->setPosition(&X2D_FRONT_POS);
				pTargetCam->setOrientation(&X2D_FRONT_ROT);
				*pX2DView = X2D_FRONT;
				break;
			case ID_2D_SIDE:
				pTargetCam->setPosition(&X2D_SIDE_POS);
				pTargetCam->setOrientation(&X2D_SIDE_ROT);
				*pX2DView = X2D_SIDE;
				break;
			}
		}
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void DisplayContextMenu(HWND hwnd, POINT pt, int iMenu, int iSubmenu, int iCheckItem)
{
	HMENU hmenu;            // top-level menu 
	HMENU hmenuTrackPopup;  // shortcut menu 

	// Load the menu resource. 

	if((hmenu = LoadMenu(hInst, MAKEINTRESOURCE(iMenu))) == NULL)
		return;
	hmenuTrackPopup = GetSubMenu(hmenu, iSubmenu);

	MENUITEMINFOA mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = MFS_UNCHECKED;
	for(UINT i = 0, l = GetMenuItemCount(hmenuTrackPopup); i < l; ++i)
	{
		SetMenuItemInfoA(hmenuTrackPopup, i, TRUE, &mii);
	}
	if(iCheckItem)
	{
		mii.fState = MFS_CHECKED;
		SetMenuItemInfoA(hmenuTrackPopup, iCheckItem, FALSE, &mii);
	}

	ClientToScreen(hwnd, (LPPOINT)&pt);
	// Display the shortcut menu. Track the right mouse 
	// button. 

	TrackPopupMenu(hmenuTrackPopup,
		TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		pt.x, pt.y, 0, hwnd, NULL);

	// Destroy the menu. 

	DestroyMenu(hmenu);
}

void XInitViewportLayout(X_VIEWPORT_LAYOUT layout)
{
	MENUITEMINFOA mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = MFS_UNCHECKED;
	SetMenuItemInfoA(g_hMenu, ID_VEWPORTCONFIGURATION_1X2, FALSE, &mii);
	SetMenuItemInfoA(g_hMenu, ID_VEWPORTCONFIGURATION_2X2, FALSE, &mii);
	SetMenuItemInfoA(g_hMenu, ID_VEWPORTCONFIGURATION_2X1, FALSE, &mii);
	SetMenuItemInfoA(g_hMenu, ID_VEWPORTCONFIGURATION_3DONLY, FALSE, &mii);
	UINT uMenuId = 0;
	mii.fState = MFS_CHECKED;
	switch(layout)
	{
	case XVIEW_2X2:
		uMenuId = ID_VEWPORTCONFIGURATION_2X2;
		ShowWindow(g_hTopRightWnd, TRUE);
		ShowWindow(g_hBottomLeftWnd, TRUE);
		ShowWindow(g_hBottomRightWnd, TRUE);
		g_isXResizeable = TRUE;
		g_isYResizeable = TRUE;
		break;
	case XVIEW_1X2:
		uMenuId = ID_VEWPORTCONFIGURATION_1X2;
		ShowWindow(g_hTopRightWnd, TRUE);
		ShowWindow(g_hBottomLeftWnd, FALSE);
		ShowWindow(g_hBottomRightWnd, FALSE);
		g_isXResizeable = TRUE;
		g_isYResizeable = FALSE;
		break;
	case XVIEW_2X1:
		uMenuId = ID_VEWPORTCONFIGURATION_2X1;
		ShowWindow(g_hTopRightWnd, FALSE);
		ShowWindow(g_hBottomLeftWnd, TRUE);
		ShowWindow(g_hBottomRightWnd, FALSE);
		g_isXResizeable = FALSE;
		g_isYResizeable = TRUE;
		break;
	case XVIEW_3DONLY:
		uMenuId = ID_VEWPORTCONFIGURATION_3DONLY;
		ShowWindow(g_hTopRightWnd, FALSE);
		ShowWindow(g_hBottomLeftWnd, FALSE);
		ShowWindow(g_hBottomRightWnd, FALSE);
		g_isXResizeable = FALSE;
		g_isYResizeable = FALSE;
		break;
	}
	
	SendMessage(g_hWndMain, WM_COMMAND, MAKEWPARAM(ID_VIEW_AUTOSIZEVIEWS, 0), 0);

	g_xviewportLayout = layout;
	SetMenuItemInfoA(g_hMenu, uMenuId, FALSE, &mii);
}