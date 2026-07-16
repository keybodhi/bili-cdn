# BiliCDN

Resolve Bilibili video page URLs to direct CDN download links. A tiny (~8KB) native Windows GUI tool — no runtime, no dependencies, works on any Windows 10/11 machine.

> **Disclaimer:** This tool is for educational and personal use only. Do not use it for illegal purposes, copyright infringement, or unauthorized downloading of copyrighted content.

## Features

- **Paste** — paste a Bilibili URL from clipboard into the input field
- **Get** — resolve the input URL to a direct CDN link (displayed in the result area)
- **One-click Copy** — read clipboard, resolve to CDN URL, and copy it back — all in one click
- **Tiny** — ~8KB executable, compressed with UPX
- **Zero dependencies** — pure Win32 API, no CRT, no runtime required

## Screenshot

```
┌──────────────────────────────── BiliCDN ────────────────────────────────┐
│                                                                          │
│  B站链接: [________________________________________]  [粘贴] [一键复制] │
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │ https://upos-sz-mirrorcos.bilivideo.com/upgcxcode/...              │ │
│  │                                                                     │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

## How It Works

1. Extracts the `BV` id and optional `p` (page) parameter from the video URL
2. Calls [Bilibili View API](https://api.bilibili.com/x/web-interface/view) to get `aid` and `cid`
3. Calls [Bilibili Playurl API](https://api.bilibili.com/x/player/playurl) to get the direct CDN URL
4. Returns the direct media link (`.mp4` / `.flv`)

## Download

Get the latest executable from [GitHub Releases](../../releases).

## Build from Source

**Requirements:** Visual Studio 2022 (Build Tools) with Windows 10 SDK

```bat
cd src
build.bat
```

Output: `bilicdn.exe` (~8KB compressed with UPX)

### Build Details

- Compiled with MSVC `/O1 /GS- /NODEFAULTLIB` — no C runtime, custom entry point
- WinHTTP for HTTPS API calls with TLS 1.2
- All JSON parsing via string scanning (no JSON library)
- Unicode dialog (WCHAR) for proper CJK text handling
- Common Controls v6 manifest for modern Windows styling
- UPX `--best` LZMA compression

## License

MIT
