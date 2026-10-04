#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")

static const wchar_t CLASS_NAME[] = L"ChronoGlassWindow";
static const UINT WM_TRAY = WM_APP + 1;
static HWND g_hwnd = nullptr;
static NOTIFYICONDATAW g_nid{};
static HFONT g_dayFont = nullptr, g_dateFont = nullptr, g_timeFont = nullptr;
static bool g_running = true;
static bool g_24h = true;
static bool g_seconds = true;

static std::wstring formatNow(const wchar_t* fmt) {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buf[128]{};
    wcsftime(buf, 128, fmt, nullptr);
    return buf;
}

static void createFonts() {
    g_dayFont = CreateFontW(42,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,
        DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
        FF_DONTCARE,L"Segoe UI");
    g_dateFont = CreateFontW(15,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
        DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
        FF_DONTCARE,L"Segoe UI");
    g_timeFont = CreateFontW(12,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
        DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
        FF_DONTCARE,L"Segoe UI");
}

static void deleteFonts() {
    if(g_dayFont) DeleteObject(g_dayFont);
    if(g_dateFont) DeleteObject(g_dateFont);
    if(g_timeFont) DeleteObject(g_timeFont);
}

static void drawCentered(HDC dc, const std::wstring& text, HFONT font, int y, COLORREF color) {
    RECT rc{}; GetClientRect(g_hwnd, &rc);
    SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, text.c_str(), -1, &rc, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
}

static std::wstring getTimeText() {
    SYSTEMTIME st{}; GetLocalTime(&st);
    int h = st.wHour;
    std::wstring suffix;
    if(!g_24h) {
        suffix = h >= 12 ? L" PM" : L" AM";
        h %= 12; if(h == 0) h = 12;
    }
    wchar_t b[64]{};
    if(g_seconds) swprintf_s(b, L"%02d:%02d:%02d", h, st.wMinute, st.wSecond);
    else swprintf_s(b, L"%02d:%02d", h, st.wMinute);
    return std::wstring(b) + suffix;
}

static void paint(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd, &ps);

    RECT rc{}; GetClientRect(hwnd, &rc);
    HBRUSH bg = CreateSolidBrush(RGB(0,0,0));
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    // The layered window uses color-key transparency: black is invisible.
    drawCentered(dc, formatNow(L"%A"), g_dayFont, 2, RGB(245,245,245));
    drawCentered(dc, formatNow(L"%b %d, %Y"), g_dateFont, 54, RGB(190,190,190));
    drawCentered(dc, getTimeText(), g_timeFont, 76, RGB(170,170,170));

    EndPaint(hwnd, &ps);
}

static BOOL CALLBACK findWorker(HWND hwnd, LPARAM lParam) {
    HWND shellView = FindWindowExW(hwnd, nullptr, L"SHELLDLL_DefView", nullptr);
    if(shellView) {
        HWND* out = reinterpret_cast<HWND*>(lParam);
        *out = FindWindowExW(nullptr, hwnd, L"WorkerW", nullptr);
        return FALSE;
    }
    return TRUE;
}

static void attachToDesktop() {
    // Ask Explorer to create the WorkerW desktop layer, then parent our
    // transparent window to it. This keeps the clock on the wallpaper layer.
    HWND progman = FindWindowW(L"Progman", nullptr);
    if(progman) {
        DWORD_PTR result = 0;
        SendMessageTimeoutW(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, &result);
    }
    HWND worker = nullptr;
    EnumWindows(findWorker, reinterpret_cast<LPARAM>(&worker));
    if(worker) SetParent(g_hwnd, worker);
}

static void setClickThrough(bool on) {
    LONG_PTR ex = GetWindowLongPtrW(g_hwnd, GWL_EXSTYLE);
    if(on) ex |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
    else ex &= ~(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
    SetWindowLongPtrW(g_hwnd, GWL_EXSTYLE, ex);
}

static void showMenu() {
    POINT p{}; GetCursorPos(&p);
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1001, g_seconds ? L"✓ Show seconds" : L"Show seconds");
    AppendMenuW(menu, MF_STRING, 1002, g_24h ? L"✓ 24-hour time" : L"24-hour time");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 1003, L"Open settings");
    AppendMenuW(menu, MF_STRING, 1004, L"Exit");
    SetForegroundWindow(g_hwnd);
    int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, p.x, p.y, 0, g_hwnd, nullptr);
    DestroyMenu(menu);

    if(cmd == 1001) g_seconds = !g_seconds;
    else if(cmd == 1002) g_24h = !g_24h;
    else if(cmd == 1004) PostMessageW(g_hwnd, WM_CLOSE, 0, 0);
    InvalidateRect(g_hwnd, nullptr, TRUE);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
        case WM_CREATE:
            SetTimer(hwnd, 1, 1000, nullptr);
            createFonts();
            attachToDesktop();
            setClickThrough(true);
            return 0;
        case WM_TIMER:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_PAINT:
            paint(hwnd);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_CONTEXTMENU:
            showMenu();
            return 0;
        case WM_LBUTTONDOWN:
            // Temporarily allow dragging with Ctrl+Alt+click.
            if((GetAsyncKeyState(VK_CONTROL)&0x8000) && (GetAsyncKeyState(VK_MENU)&0x8000)) {
                setClickThrough(false);
                ReleaseCapture();
                SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
                setClickThrough(true);
            }
            return 0;
        case WM_TRAY:
            if(lp == WM_RBUTTONUP || lp == WM_LBUTTONUP) showMenu();
            return 0;
        case WM_DESTROY:
            KillTimer(hwnd, 1);
            Shell_NotifyIconW(NIM_DELETE, &g_nid);
            deleteFonts();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void addTrayIcon(HWND hwnd) {
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAY;
    g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L"ChronoGlass");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    WNDCLASSW wc{};
    wc.hInstance = hInst;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassW(&wc);

    // Wide enough for the typographic layout; black is color-keyed transparent.
    g_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        CLASS_NAME, L"ChronoGlass",
        WS_POPUP,
        0, 0, 420, 110,
        nullptr, nullptr, hInst, nullptr);

    if(!g_hwnd) return 1;

    SetLayeredWindowAttributes(g_hwnd, RGB(0,0,0), 0, LWA_COLORKEY);
    // Place near the top-center of the primary display.
    int sw = GetSystemMetrics(SM_CXSCREEN);
    SetWindowPos(g_hwnd, HWND_BOTTOM, (sw-420)/2, 32, 420, 110, SWP_NOACTIVATE | SWP_SHOWWINDOW);

    addTrayIcon(g_hwnd);

    MSG msg{};
    while(GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
