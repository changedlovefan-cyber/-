cmake_minimum_required(VERSION 3.20)
project(MinecraftAI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# x64 のみ。統合版は 64bit プロセス
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "x64 でビルドしてください。統合版 (Minecraft.Windows.exe) は 64bit です。")
endif()

add_library(MinecraftAI SHARED
  src/dllmain.cpp
  src/screen_capture.cpp
  src/jpeg_encode.cpp
  src/ollama_client.cpp
  src/input_sim.cpp
)

# GDI+ が使えるなら GDI+ 版を使う。MinGW など GDI+ の無い環境では
# 自動的に WIC 版 (windowscodecs.dll) にフォールバックする。
include(CheckIncludeFileCXX)
check_include_file_cxx("gdiplus.h" HAVE_GDIPLUS_H)
if(HAVE_GDIPLUS_H AND MSVC)
  target_compile_definitions(MinecraftAI PRIVATE DX_WITH_GDI_PLUS)
endif()

# ソースは UTF-8 (BOM なし)。MSVC は既定で CP932 と解釈するため /utf-8 が必須。
# これが無いと日本語プロンプトが文字化けし、モデルの応答品質が落ちる。
if(MSVC)
  target_compile_options(MinecraftAI PRIVATE /utf-8 /W3 /EHsc)
  target_compile_definitions(MinecraftAI PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
  target_compile_options(MinecraftAI PRIVATE -Wall -Wextra)
endif()
target_compile_definitions(MinecraftAI PRIVATE
  WIN32_LEAN_AND_MEAN
  NOMINMAX
  NDEBUG
)

target_include_directories(MinecraftAI PRIVATE src)
target_link_libraries(MinecraftAI PRIVATE
  winhttp ole32 oleaut32 gdi32 user32
)
if(HAVE_GDIPLUS_H AND MSVC)
  target_link_libraries(MinecraftAI PRIVATE gdiplus)
endif()

# 出力名を MinecraftAI.dll に
set_target_properties(MinecraftAI PROPERTIES
  OUTPUT_NAME "MinecraftAI"
  PREFIX ""
)

# config.json を出力フォルダにコピー
add_custom_command(TARGET MinecraftAI POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/config.json
    $<TARGET_FILE_DIR:MinecraftAI>/config.json
)
