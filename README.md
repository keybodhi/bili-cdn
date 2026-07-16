# BiliCDN

解析B站视频链接，获取直链CDN下载地址。原生Windows GUI工具，大小仅约8KB，无需运行时、无依赖，Windows 10/11 通用。

> **免责声明：** 本程序仅供学习交流，请勿用于非法用途。

## 功能

- **粘贴** — 从剪贴板粘贴B站链接到输入框
- **获取** — 解析输入框中的链接，显示直链CDN地址
- **一键复制** — 读取剪贴板中的B站链接，解析出CDN直链，自动复制回剪贴板

## 截图

![截图](screenshot.png)

## 使用方法

1. 在浏览器中复制B站视频链接（如 `https://www.bilibili.com/video/BV1xx411c7mD`）
2. 打开 `bilicdn.exe`
3. 点击 **粘贴** 将链接填入输入框（可选，也可以直接输入）
4. 点击 **获取** 查看直链地址，或点击 **一键复制** 一步到位
5. 复制得到的 CDN 链接，即可在播放器或下载工具中使用

## 下载

从 [GitHub Releases](../../releases) 下载最新版 `bilicdn.exe`。

## 从源码编译

**环境要求：** Visual Studio 2022 (Build Tools) + Windows 10 SDK

```bat
cd src
build.bat
```

输出：`bilicdn.exe`（UPX压缩后约8KB）

### 编译说明

- MSVC `/O1 /GS- /NODEFAULTLIB` — 无C运行时，自写入口点
- WinHTTP 实现 HTTPS API 调用，TLS 1.2
- 纯字符串扫描解析JSON，无第三方库
- Unicode 对话框（WCHAR），正确处理中文显示
- Common Controls v6 Manifest，启用Windows现代控件风格
- UPX `--best` LZMA 压缩

## License

MIT
