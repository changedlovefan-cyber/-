#pragma once
#include <windows.h>
#include <string>

// Minecraft 統合版 (Bedrock / UWP) のウィンドウを探す
HWND FindMinecraftWindow();

// ウィンドウ全体を JPEG にして base64 文字列で返す。失敗したら空文字。
std::string CaptureWindowToBase64(HWND hwnd, int targetWidth);

// DLL 自身のフォルダとログファイルのフルパスを求める（printf 非依存）
void ResolveDllPaths(HMODULE hModule, std::wstring& dllDir, std::wstring& logPath);
