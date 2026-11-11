@echo off
REM Visual Studio 2022 が必要。x64 Release でビルドする。
REM 使い方: build.bat
setlocal
cd /d %~dp0
if exist build rmdir /s /q build
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto fail
cmake --build build --config Release
if errorlevel 1 goto fail
echo.
echo ビルド完了: build\Release\MinecraftAI.dll
echo config.json も同じフォルダにコピーされます。
echo.
echo 次の手順:
echo   1. build\Release フォルダを C:\MinecraftAI など書き込み可能な場所へコピー
echo   2. FateInjector を「管理者として実行」
echo   3. Minecraft.Windows.exe を選び、MinecraftAI.dll を inject
goto end

:fail
echo.
echo ビルドに失敗しました。Visual Studio 2022 の「C++ によるデスクトップ開発」が
echo 入っているか確認してください。

:end
pause
