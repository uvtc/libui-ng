// 6 april 2015
#include "uipriv_windows.hpp"

// this is a helper function that takes the logic of determining window classes and puts it all in one place
// there are a number of places where we need to know what window class an arbitrary handle has
// theoretically we could use the class atom to avoid a _wcsicmp()
// however, raymond chen advises against this - http://blogs.msdn.com/b/oldnewthing/archive/2004/10/11/240744.aspx (and we're not in control of the Tab class, before you say anything)
// usage: windowClassOf(hwnd, L"class 1", L"class 2", ..., NULL)
int windowClassOf(HWND hwnd, ...)
{
// MSDN says 256 is the maximum length of a class name; add a few characters just to be safe (because it doesn't say whether this includes the terminating null character)
#define maxClassName 260
	WCHAR classname[maxClassName + 1];
	va_list ap;
	WCHAR *curname;
	int i;

	if (GetClassNameW(hwnd, classname, maxClassName) == 0) {
		logLastError(L"error getting name of window class");
		// assume no match on error, just to be safe
		return -1;
	}
	va_start(ap, hwnd);
	i = 0;
	for (;;) {
		curname = va_arg(ap, WCHAR *);
		if (curname == NULL)
			break;
		if (_wcsicmp(classname, curname) == 0) {
			va_end(ap);
			return i;
		}
		i++;
	}
	// no match
	va_end(ap);
	return -1;
}

// wrapper around MapWindowRect() that handles the complex error handling
void mapWindowRect(HWND from, HWND to, RECT *r)
{
	RECT prevr;
	DWORD le;

	prevr = *r;
	SetLastError(0);
	if (MapWindowRect(from, to, r) == 0) {
		le = GetLastError();
		SetLastError(le);		// just to be safe
		if (le != 0) {
			logLastError(L"error calling MapWindowRect()");
			// restore original rect on error, just in case
			*r = prevr;
		}
	}
}

DWORD getStyle(HWND hwnd)
{
	return (DWORD) GetWindowLongPtrW(hwnd, GWL_STYLE);
}

void setStyle(HWND hwnd, DWORD style)
{
	SetWindowLongPtrW(hwnd, GWL_STYLE, (LONG_PTR) style);
}

DWORD getExStyle(HWND hwnd)
{
	return (DWORD) GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
}

void setExStyle(HWND hwnd, DWORD exstyle)
{
	SetWindowLongPtrW(hwnd, GWL_EXSTYLE, (LONG_PTR) exstyle);
}

void uiWindowsEnsureDestroyWindow(HWND hwnd)
{
	if (DestroyWindow(hwnd) == 0)
		logLastError(L"error destroying window");
}

// TODO allow passing NULL to indicate no parent
// this would allow for custom containers
void uiWindowsEnsureSetParent(HWND hwnd, HWND parent)
{
	if (SetParent(hwnd, parent) == 0)
		logLastError(L"error setting window parent");
}

void uiWindowsEnsureAssignControlIDZOrder(HWND hwnd, LONG_PTR controlID, HWND insertAfter)
{
	SetWindowLongPtrW(hwnd, GWLP_ID, controlID);
	setWindowInsertAfter(hwnd, insertAfter);
}

// see http://blogs.msdn.com/b/oldnewthing/archive/2003/09/11/54885.aspx and http://blogs.msdn.com/b/oldnewthing/archive/2003/09/13/54917.aspx
void clientSizeToWindowSize(HWND hwnd, intmax_t *width, intmax_t *height, BOOL hasMenubar)
{
	RECT window;

	window.left = 0;
	window.top = 0;
	window.right = *width;
	window.bottom = *height;
	if (AdjustWindowRectEx(&window, getStyle(hwnd), hasMenubar, getExStyle(hwnd)) == 0) {
		logLastError(L"error getting adjusted window rect");
		// on error, don't give up; the window will be smaller but whatever
		window.left = 0;
		window.top = 0;
		window.right = *width;
		window.bottom = *height;
	}
	if (hasMenubar) {
		RECT temp;

		temp = window;
		temp.bottom = 0x7FFF;		// infinite height
		SendMessageW(hwnd, WM_NCCALCSIZE, (WPARAM) FALSE, (LPARAM) (&temp));
		window.bottom += temp.top;
	}
	*width = window.right - window.left;
	*height = window.bottom - window.top;
}

HWND parentOf(HWND child)
{
	return GetAncestor(child, GA_PARENT);
}

HWND parentToplevel(HWND child)
{
	return GetAncestor(child, GA_ROOT);
}

void uiWindowsEnsureMoveWindowDuringResize(HWND hwnd, intmax_t x, intmax_t y, intmax_t width, intmax_t height)
{
	RECT r;

	r.left = x;
	r.top = y;
	r.right = x + width;
	r.bottom = y + height;
	if (SetWindowPos(hwnd, NULL, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOZORDER) == 0)
		logLastError(L"error moving window");
}

void setWindowInsertAfter(HWND hwnd, HWND insertAfter)
{
	if (SetWindowPos(hwnd, insertAfter, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE) == 0)
		logLastError(L"error reordering window");
}