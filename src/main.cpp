#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <sstream>

static const wchar_t* GAME_EXE = L"GTA5.exe";
static HWND g_overlay = nullptr;
static HWND g_game = nullptr;
static bool g_menu = true;
static bool g_god = false, g_neverWanted = false, g_infiniteAmmo = false;
static int g_tab = 0;
static DWORD g_gamePid = 0;
static const COLORREF KEY_COLOR = RGB(255, 0, 255);

DWORD FindGamePid() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{ sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, GAME_EXE) == 0) { pid = pe.th32ProcessID; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

BOOL CALLBACK FindGameWindow(HWND hwnd, LPARAM param) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == g_gamePid && IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == nullptr) {
        g_game = hwnd;
        return FALSE;
    }
    return TRUE;
}

void DrawTextLine(HDC dc, int x, int y, const wchar_t* text, COLORREF color, int size = 16, bool bold = false) {
    HFONT font = CreateFontW(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT old = (HFONT)SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    TextOutW(dc, x, y, text, (int)wcslen(text));
    SelectObject(dc, old);
    DeleteObject(font);
}
void Fill(HDC dc, RECT r, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c); FillRect(dc, &r, b); DeleteObject(b);
}
void DrawToggle(HDC dc, int y, const wchar_t* label, bool value) {
    RECT r{ 28, y, 352, y + 42 };
    Fill(dc, r, RGB(17, 29, 49));
    DrawTextLine(dc, 42, y + 11, label, RGB(235, 241, 250), 15, true);
    RECT pill{ 291, y + 10, 337, y + 32 };
    Fill(dc, pill, value ? RGB(28, 125, 235) : RGB(62, 73, 91));
    DrawTextLine(dc, value ? 302 : 307, y + 11, value ? L"ON" : L"OFF", RGB(255,255,255), 11, true);
}
void Paint() {
    if (!g_overlay) return;
    RECT wr; GetClientRect(g_overlay, &wr);
    HDC dc = GetDC(g_overlay);
    Fill(dc, wr, KEY_COLOR);
    if (g_menu) {
        RECT panel{ 0, 0, 390, 510 };
        Fill(dc, panel, RGB(7, 13, 24));
        RECT top{ 0, 0, 390, 86 }; Fill(dc, top, RGB(8, 31, 65));
        RECT accent{ 0, 84, 390, 88 }; Fill(dc, accent, RGB(35, 133, 255));
        DrawTextLine(dc, 25, 18, L"TOKENIX", RGB(255,255,255), 27, true);
        DrawTextLine(dc, 27, 53, L"STANDALONE  /  GTA V LEGACY", RGB(134,190,255), 11, true);
        DrawTextLine(dc, 28, 105, g_tab == 0 ? L"PLAYER" : (g_tab == 1 ? L"WEAPONS" : L"VEHICLES"), RGB(90,170,255), 13, true);
        if (g_tab == 0) {
            DrawToggle(dc, 137, L"God Mode", g_god);
            DrawToggle(dc, 187, L"Never Wanted", g_neverWanted);
            DrawTextLine(dc, 29, 254, L"Player actions will be connected", RGB(155,170,190), 13);
            DrawTextLine(dc, 29, 274, L"to the native game layer next.", RGB(155,170,190), 13);
        } else if (g_tab == 1) {
            DrawToggle(dc, 137, L"Infinite Ammo", g_infiniteAmmo);
            DrawTextLine(dc, 29, 204, L"Weapon controls are not yet wired", RGB(155,170,190), 13);
            DrawTextLine(dc, 29, 224, L"to GTA's gameplay functions.", RGB(155,170,190), 13);
        } else {
            DrawTextLine(dc, 29, 145, L"Vehicle spawning is not yet wired", RGB(155,170,190), 13);
            DrawTextLine(dc, 29, 165, L"to GTA's gameplay functions.", RGB(155,170,190), 13);
        }
        RECT nav{ 18, 440, 372, 484 }; Fill(dc, nav, RGB(14, 25, 42));
        DrawTextLine(dc, 35, 453, L"F1  PLAYER", RGB(220,230,245), 12, true);
        DrawTextLine(dc, 145, 453, L"F2  WEAPONS", RGB(220,230,245), 12, true);
        DrawTextLine(dc, 278, 453, L"F3  VEHICLES", RGB(220,230,245), 12, true);
        DrawTextLine(dc, 28, 490, L"F5 HIDE  |  ESC CLOSE", RGB(112,132,158), 10, true);
    }
    ReleaseDC(g_overlay, dc);
}
void SyncToGame() {
    g_gamePid = FindGamePid();
    g_game = nullptr;
    if (g_gamePid) EnumWindows(FindGameWindow, 0);
    if (!g_game) { ShowWindow(g_overlay, SW_HIDE); return; }
    RECT r; GetWindowRect(g_game, &r);
    SetWindowPos(g_overlay, HWND_TOPMOST, r.left, r.top, r.right-r.left, r.bottom-r.top, SWP_NOACTIVATE);
    ShowWindow(g_overlay, g_menu ? SW_SHOWNOACTIVATE : SW_HIDE);
    Paint();
}
LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: { PAINTSTRUCT ps; BeginPaint(h,&ps); EndPaint(h,&ps); Paint(); return 0; }
    case WM_HOTKEY:
        if (w == 1) { g_menu = !g_menu; SyncToGame(); }
        if (w == 2) { g_tab = 0; Paint(); }
        if (w == 3) { g_tab = 1; Paint(); }
        if (w == 4) { g_tab = 2; Paint(); }
        return 0;
    case WM_LBUTTONDOWN: {
        int x = LOWORD(l), y = HIWORD(l);
        if (g_tab == 0 && x >= 20 && x <= 360) {
            if (y >= 137 && y <= 179) g_god = !g_god;
            if (y >= 187 && y <= 229) g_neverWanted = !g_neverWanted;
        }
        if (g_tab == 1 && y >= 137 && y <= 179) g_infiniteAmmo = !g_infiniteAmmo;
        Paint(); return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h,m,w,l);
}
int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int) {
    WNDCLASSW wc{}; wc.lpfnWndProc = WndProc; wc.hInstance = inst;
    wc.lpszClassName = L"TOKENIX_NATIVE_OVERLAY"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    g_overlay = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        wc.lpszClassName, L"TOKENIX", WS_POPUP, 0,0,390,510, nullptr,nullptr,inst,nullptr);
    SetLayeredWindowAttributes(g_overlay, KEY_COLOR, 0, LWA_COLORKEY);
    RegisterHotKey(g_overlay, 1, 0, VK_F5);
    RegisterHotKey(g_overlay, 2, 0, VK_F1);
    RegisterHotKey(g_overlay, 3, 0, VK_F2);
    RegisterHotKey(g_overlay, 4, 0, VK_F3);
    SetTimer(g_overlay, 1, 500, nullptr);
    MSG msg{};
    while (GetMessageW(&msg,nullptr,0,0) > 0) {
        if (msg.message == WM_TIMER) SyncToGame();
        TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    UnregisterHotKey(g_overlay,1); UnregisterHotKey(g_overlay,2);
    UnregisterHotKey(g_overlay,3); UnregisterHotKey(g_overlay,4);
    return 0;
}
