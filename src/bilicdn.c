#include <windows.h>
#include <winhttp.h>
#include "resource.h"
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma function(memcpy, memset)
void *memcpy(void *d, const void *s, size_t n) { char *cd = (char *)d; const char *cs = (const char *)s; size_t i; for (i = 0; i < n; i++) cd[i] = cs[i]; return d; }
void *memset(void *d, int c, size_t n) { char *cd = (char *)d; size_t i; for (i = 0; i < n; i++) cd[i] = (char)c; return d; }

// ── wide string utils ──────────────────────────────────
static int w_len(const WCHAR *s) { const WCHAR *p = s; while (*p) p++; return (int)(p - s); }
static void w_copy(WCHAR *d, const WCHAR *s) { while (*s) *d++ = *s++; *d = 0; }
static void w_ncopy(WCHAR *d, const WCHAR *s, int n) { int i; for (i = 0; i < n - 1 && s[i]; i++) d[i] = s[i]; d[i] = 0; }
static int w_atoi(const WCHAR *s) { int n = 0; while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; } return n; }
static const WCHAR *w_find(const WCHAR *h, const WCHAR *n) { int hl = w_len(h), nl = w_len(n); if (nl == 0) return h; int i, j; for (i = 0; i <= hl - nl; i++) { for (j = 0; j < nl && h[i + j] == n[j]; j++); if (j == nl) return h + i; } return NULL; }

static void a2w(const char *a, WCHAR *w, int max) { MultiByteToWideChar(CP_UTF8, 0, a, -1, w, max); }

// int64 -> ascii (wsprintfA doesn't support %llu)
static int ull_to_a(unsigned long long v, char *buf) {
    char tmp[32]; int i = 0, j;
    do { tmp[i++] = '0' + (v % 10); v /= 10; } while (v);
    for (j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
    buf[i] = 0; return i;
}

// ── JSON extraction (handles \/ \n \t \\ \" escapes) ───
static int json_strval(const char *j, const char *key, char *buf, int max) {
    int kl; for (kl = 0; key[kl]; kl++);
    char key2[128]; key2[0] = '"'; int ki;
    for (ki = 0; ki < kl; ki++) key2[ki + 1] = key[ki];
    key2[++ki] = '"'; key2[++ki] = ':'; key2[++ki] = '"'; key2[++ki] = 0;
    int jl; for (jl = 0; j[jl]; jl++);
    int i, m; const char *p = 0;
    for (i = 0; i <= jl - ki; i++) {
        for (m = 0; m < ki && j[i + m] == key2[m]; m++);
        if (m == ki) { p = j + i + ki; break; }
    }
    if (!p) return 0;
    int n = 0;
    while (*p && n < max - 1) {
        if (*p == '\\' && p[1]) {
            p++;
            if (*p == '"' || *p == '\\' || *p == '/') { buf[n++] = *p; p++; }
            else if (*p == 'n') { buf[n++] = '\n'; p++; }
            else if (*p == 't') { buf[n++] = '\t'; p++; }
            else if (*p == 'u') {
                p++;
                int h = 0, k;
                for (k = 0; k < 4 && p[k]; k++) { char c = p[k]; h <<= 4; if (c >= '0' && c <= '9') h |= c - '0'; else if (c >= 'a' && c <= 'f') h |= c - 'a' + 10; else if (c >= 'A' && c <= 'F') h |= c - 'A' + 10; }
                buf[n++] = (char)h; p += k;
            }
            else { buf[n++] = *p; p++; }
        }
        else if (*p == '"') break;
        else { buf[n++] = *p; p++; }
    }
    buf[n] = 0; return n;
}

static unsigned long long json_intval(const char *j, const char *key) {
    int kl; for (kl = 0; key[kl]; kl++);
    char key2[128]; key2[0] = '"'; int ki;
    for (ki = 0; ki < kl; ki++) key2[ki + 1] = key[ki];
    key2[++ki] = '"'; key2[++ki] = ':'; key2[++ki] = 0;
    int jl; for (jl = 0; j[jl]; jl++);
    int i, m; const char *p = 0;
    for (i = 0; i <= jl - ki; i++) {
        for (m = 0; m < ki && j[i + m] == key2[m]; m++);
        if (m == ki) { p = j + i + ki; break; }
    }
    if (!p) return 0;
    while (*p == ' ' || *p == '\t') p++;
    unsigned long long n = 0; while (*p >= '0' && *p <= '9') { n = n * 10 + (*p - '0'); p++; } return n;
}

// ── WinHTTP ────────────────────────────────────────────
static int http_get(const WCHAR *host, const WCHAR *path, char *body, int maxbody, int *errCode) {
    *errCode = 0;
    HINTERNET hs = WinHttpOpen(L"Mozilla/5.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hs) { *errCode = GetLastError(); return 0; }
    HINTERNET hc = WinHttpConnect(hs, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hc) { *errCode = GetLastError(); WinHttpCloseHandle(hs); return 0; }
    HINTERNET hr = WinHttpOpenRequest(hc, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hr) { *errCode = GetLastError(); WinHttpCloseHandle(hc); WinHttpCloseHandle(hs); return 0; }
    if (!WinHttpSendRequest(hr, L"Referer: https://www.bilibili.com\r\n", -1, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) { *errCode = GetLastError(); WinHttpCloseHandle(hr); WinHttpCloseHandle(hc); WinHttpCloseHandle(hs); return 0; }
    if (!WinHttpReceiveResponse(hr, NULL)) { *errCode = GetLastError(); WinHttpCloseHandle(hr); WinHttpCloseHandle(hc); WinHttpCloseHandle(hs); return 0; }
    int total = 0; DWORD avail, read;
    while (WinHttpQueryDataAvailable(hr, &avail) && avail > 0) {
        DWORD tr = avail; if (total + (int)tr >= maxbody - 1) tr = maxbody - 1 - total;
        if (!WinHttpReadData(hr, body + total, tr, &read)) break; total += read; if (total >= maxbody - 1) break;
    }
    body[total] = 0; WinHttpCloseHandle(hr); WinHttpCloseHandle(hc); WinHttpCloseHandle(hs); return total;
}

// ── Bilibili resolver (wide strings) ───────────────────
static int resolve_bili(const WCHAR *input, WCHAR *result, int maxres) {
    static char vbuf[8192], pbuff[8192], durl[2048];
    static WCHAR tmp[512];

    const WCHAR *bv = w_find(input, L"BV");
    if (!bv) { w_copy(result, L"未识别到BV号"); return w_len(result); }
    WCHAR bvid[16]; int blen = 0;
    for (const WCHAR *p = bv; *p && blen < 15; p++) { WCHAR c = *p; if (c == '/' || c == '?' || c == '&' || c == '#' || c == ' ') break; bvid[blen++] = c; }
    bvid[blen] = 0;
    int pn = 1; const WCHAR *pp = w_find(input, L"p="); if (pp) pn = w_atoi(pp + 2); if (pn < 1) pn = 1;

    // view API
    vbuf[0] = 0;
    wsprintfW(tmp, L"/x/web-interface/view?bvid=%s", bvid);
    int err; int vlen = http_get(L"api.bilibili.com", tmp, vbuf, 8192, &err);
    if (vlen == 0) { wsprintfW(result, L"view接口失败 err=%d", err); return w_len(result); }

    unsigned long long code = json_intval(vbuf, "code");
    if (code != 0) { wsprintfW(result, L"view返回code=%d", (int)code); return w_len(result); }

    unsigned long long aid = json_intval(vbuf, "aid");
    if (aid == 0) { w_copy(result, L"视频不存在(aid=0)"); return w_len(result); }

    // find pages array and specific page
    const char *pages = 0; int i;
    for (i = 0; vbuf[i]; i++) { if (vbuf[i] == '"' && vbuf[i+1] == 'p' && vbuf[i+2] == 'a' && vbuf[i+3] == 'g' && vbuf[i+4] == 'e' && vbuf[i+5] == 's' && vbuf[i+6] == '"' && vbuf[i+7] == ':' && vbuf[i+8] == '[') { pages = vbuf + i; break; } }
    if (!pages) { w_copy(result, L"无分P数据"); return w_len(result); }

    unsigned long long ncid = 0; const char *cp = pages;
    for (i = 1; i <= pn; i++) {
        for (; *cp; cp++) { if (*cp == '"' && cp[1] == 'c' && cp[2] == 'i' && cp[3] == 'd' && cp[4] == '"' && cp[5] == ':') { cp += 6; break; } }
        ncid = 0; while (*cp >= '0' && *cp <= '9') { ncid = ncid * 10 + (*cp - '0'); cp++; }
    }
    if (ncid == 0) { w_copy(result, L"未找到分P的cid"); return w_len(result); }
    unsigned long long cid = ncid;

    // playurl API
    pbuff[0] = 0;
    char haid[32], hcid[32]; ull_to_a(aid, haid); ull_to_a(cid, hcid);
    char url_buf[512]; url_buf[0] = 0;
    char *up = url_buf;
    char *pfx = "/x/player/playurl?avid="; while (*pfx) *up++ = *pfx++;
    char *pa = haid; while (*pa) *up++ = *pa++;
    pfx = "&cid="; while (*pfx) *up++ = *pfx++;
    pa = hcid; while (*pa) *up++ = *pa++;
    pfx = "&qn=64&platform=html5&otype=json&high_quality=1"; while (*pfx) *up++ = *pfx++;
    *up = 0;
    a2w(url_buf, tmp, 512);
    int plen = http_get(L"api.bilibili.com", tmp, pbuff, 8192, &err);
    if (plen == 0) { wsprintfW(result, L"playurl接口失败 err=%d", err); return w_len(result); }

    unsigned long long pcode = json_intval(pbuff, "code");
    if (pcode != 0) { wsprintfW(result, L"playurl返回code=%d", (int)pcode); return w_len(result); }

    if (!json_strval(pbuff, "url", durl, 2048)) {
        static WCHAR debug[300]; pbuff[200] = 0; a2w(pbuff, debug, 300);
        wsprintfW(result, L"未找到url.响应头200字节:\r\n%s", debug);
        return w_len(result);
    }

    a2w(durl, result, maxres);
    return w_len(result);
}

// ── clipboard ───────────────────────────────────────────
static WCHAR *read_clipboard(HWND h, WCHAR *buf, int max) {
    if (!OpenClipboard(h)) return 0; HANDLE d = GetClipboardData(CF_UNICODETEXT);
    if (!d) { CloseClipboard(); return 0; } WCHAR *w = (WCHAR *)GlobalLock(d);
    if (!w) { CloseClipboard(); return 0; } w_ncopy(buf, w, max); GlobalUnlock(d); CloseClipboard(); return buf;
}
static void paste_to_edit(HWND h) { WCHAR buf[2048]; if (read_clipboard(h, buf, 2048)) SetDlgItemTextW(h, IDC_URL, buf); }
static void copy_to_clip(HWND h, const WCHAR *s) {
    if (!OpenClipboard(h)) return; EmptyClipboard(); int n = w_len(s) + 1;
    HGLOBAL m = GlobalAlloc(GMEM_MOVEABLE, n * sizeof(WCHAR));
    if (m) { WCHAR *w = (WCHAR *)GlobalLock(m); if (w) { w_copy(w, s); GlobalUnlock(m); } SetClipboardData(CF_UNICODETEXT, m); }
    CloseClipboard();
}

// ── dialog ─────────────────────────────────────────────
static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_COMMAND:
        if (LOWORD(wp) == IDC_PASTE) { paste_to_edit(hDlg); }
        if (LOWORD(wp) == IDC_COPY) {
            WCHAR result[4096]; result[0] = 0;
            GetDlgItemTextW(hDlg, IDC_RESULT, result, 4096);
            if (result[0]) copy_to_clip(hDlg, result);
        }
        if (LOWORD(wp) == IDC_PARSE) {
            WCHAR input[2048], result[4096]; input[0] = 0; result[0] = 0;
            GetDlgItemTextW(hDlg, IDC_URL, input, 2048);
            int r = resolve_bili(input, result, 4096);
            SetDlgItemTextW(hDlg, IDC_RESULT, result);
        }
        if (LOWORD(wp) == IDC_QUICK) {
            WCHAR input[2048], result[4096]; input[0] = 0; result[0] = 0;
            read_clipboard(hDlg, input, 2048);
            SetDlgItemTextW(hDlg, IDC_URL, input);
            int r = resolve_bili(input, result, 4096);
            SetDlgItemTextW(hDlg, IDC_RESULT, result);
            if (r > 0 && result[0] == 'h' && result[1] == 't' && result[2] == 't' && result[3] == 'p')
                copy_to_clip(hDlg, result);
        }
        break;
    case WM_CTLCOLORSTATIC:
        if ((HWND)lp == GetDlgItem(hDlg, IDC_TIP)) {
            SetTextColor((HDC)wp, RGB(0x88, 0x88, 0x88));
            SetBkMode((HDC)wp, TRANSPARENT);
            return (INT_PTR)GetStockObject(NULL_BRUSH);
        }
        SetBkMode((HDC)wp, TRANSPARENT);
        return (INT_PTR)GetStockObject(NULL_BRUSH);
    case WM_CTLCOLORDLG:
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    case WM_CTLCOLOREDIT:
        SetBkColor((HDC)wp, RGB(255, 255, 255));
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    case WM_CTLCOLORBTN:
        SetBkColor((HDC)wp, RGB(255, 255, 255));
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    case WM_CLOSE: EndDialog(hDlg, 0); break;
    case WM_INITDIALOG: {
        HICON icon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_APPLICATION));
        if (icon) { SendMessageW(hDlg, WM_SETICON, ICON_BIG, (LPARAM)icon); SendMessageW(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)icon); }
        // tip label: smaller font
        HFONT hTipFont = CreateFontW(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
        if (hTipFont) { SendMessageW(GetDlgItem(hDlg, IDC_TIP), WM_SETFONT, (WPARAM)hTipFont, TRUE); }
        // Fluent: rounded corners (Win11, silent fallback on Win10)
        HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
        if (dwm) {
            typedef HRESULT (WINAPI *DSA_t)(HWND, DWORD, LPCVOID, DWORD);
            DSA_t pDSA = (DSA_t)GetProcAddress(dwm, "DwmSetWindowAttribute");
            if (pDSA) { int v = 2; pDSA(hDlg, 33, &v, 4); }
            FreeLibrary(dwm);
        }
        break;
    }
    }
    return 0;
}

void __stdcall Entry(void) {
    HMODULE h = GetModuleHandleW(NULL);
    DialogBoxParamW(h, MAKEINTRESOURCEW(IDD_MAIN), NULL, DlgProc, 0);
    ExitProcess(0);
}
