# Minecraft 統合版 AI 自動操作 DLL (Ollama gemma4 / FateInjector 用)

マイクラ統合版 (Bedrock / `Minecraft.Windows.exe`) の画面を見て、Ollama の **gemma4** (vision 対応) が次の操作を判断し、キーボード/マウスを自動で操作する DLL です。

```
[Minecraft.Windows.exe プロセス内]
  MinecraftAI.dll
    ├─ 画面キャプチャ (PrintWindow PW_RENDERFULLCONTENT → 黒フレーム検出 → BitBlt → JPEG → base64)
    ├─ Ollama へ POST http://127.0.0.1:11434/api/chat  (model = gemma4:e4b)
    ├─ JSON Schema 強制 (format) + think:false でパース
    └─ SendInput で W/A/S/D・視点・クリックを実行
```

> **用途について**: 自分の PC・シングルプレイのワールドでの自動化実験用です。マルチプレイサーバーでチート目的に使ったり、他人のプレイを妨害しないでください。Mojang / Microsoft の利用規約に反する使い方は自己責任です。統合版は UWP のため、Windows やゲームのアップデートで inject できなくなる場合があります。

---

## 0. 先に結論: gemma4 は存在します

調べた結果（2026 年時点）:

| 項目 | 内容 |
|---|---|
| リリース | **2026-04-02**（Google DeepMind、Apache 2.0） |
| Ollama タグ | `gemma4:e2b` (約 7.2GB) / **`gemma4:e4b` (既定・約 9.6GB)** / `gemma4:12b`（2026-06-03 追加・長いコンテキスト）/ `gemma4:26b`（MoE・約 18GB）/ `gemma4:31b`（約 20GB） |
| 必要バージョン | **Ollama 0.20 以上**（0.22 以上推奨） |
| 画像入力 | 対応（`/api/chat` の `images: [base64]`） |
| JSON 強制 | 対応（`format` に **JSON Schema** を渡すと grammar 制約で必ず valid JSON） |

`ollama pull gemma4` は既定で `e4b` が入ります。本プロジェクトの既定も **`gemma4:e4b`** に変更済みです。

> 注意: `gemma4:26b-mlx`（MLX 版）は `format` のスキーマ制約を黙って無視することが Ollama の issue で報告されています。Windows では GGUF 版（`gemma4:e4b` / `:12b` など、`-mlx` が付かないタグ）を使ってください。

---

## 1. 必要なもの

- Windows 10/11 (x64) ※**x64 DLL 必須**（Bedrock は 64bit プロセス）
- マイクラ統合版 (Microsoft Store 版 Bedrock)
- [Ollama](https://ollama.com) 0.20 以上（Windows 版）
- gemma4 モデル
- Visual Studio 2022 (C++ デスクトップ開発) — ビルド用
- **Visual C++ 再頒布可能パッケージ (x64)** — FateInjector の実行に必要（`VCRUNTIME140_1.dll` が無いと起動できません）
- FateInjector — inject 用（**管理者として実行**）

## 2. Ollama の準備

```powershell
ollama pull gemma4            # = gemma4:e4b (推奨・9.6GB)
# 軽くしたい場合: ollama pull gemma4:e2b
# 精度重視:       ollama pull gemma4:12b
```

接続テスト（inject なしで、モデル一覧・バージョン・JSON 強制・vision をまとめて確認）:

```powershell
python tools/test_ollama.py
python tools/test_ollama.py --model gemma4:12b   # 別タグを試す場合
```

`=== 結果 ===` に `JSON 強制: OK` / `vision: OK` が出れば準備完了です。

## 3. ビルド

### 3-A. MinGW-w64 でビルドする（Linux / MSYS2 / WSL 可）

このリポジトリには **MinGW-w64 用のビルドスクリプト** `build_mingw.sh` があり、
Linux 上でも Windows 用 x64 DLL をクロスコンパイルできます。

```bash
# Ubuntu / Debian
sudo apt install g++-mingw-w64-x86-64
./build_mingw.sh
```

```bash
# MSYS2 (MINGW64 シェル)
pacman -S mingw-w64-x86_64-gcc
./build_mingw.sh
```

```bash
# クロスコンパイラ名を明示する場合
CXX=x86_64-w64-mingw32-g++ ./build_mingw.sh
```

出力: `dist/MinecraftAI.dll` (+ `dist/config.json`)

ポイント:

- **GDI+ を使わない構成**にしてあるので、MinGW-w64 の `gdiplus` ヘッダ／
  インポートライブラリが無い環境でもビルドできます。JPEG エンコードは
  **WIC (`windowscodecs.dll`)** を使用（Windows 7 以降の標準コンポーネント）。
- `libstdc++-6.dll` を配布しなくて済むよう `-static-libgcc -static-libstdc++` を
  既定で付けています（`--dynamic-runtime` で無効化）。
- `-static` は DLL では使えません。付けると壊れたファイルができるので注意。
- GDI+ 版を使いたい場合は `-DDX_WITH_GDI_PLUS` を付けて `-lgdiplus` をリンクすれば
  従来の実装に切り替わります（MSVC ビルドでは自動でこちら）。

### 3-B. MSVC でビルドする（Windows ネイティブ）

Visual Studio 2022 で「C++ によるデスクトップ開発」を入れた状態で:

```bat
build.bat
```

または手動で:

```powershell
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

出力: `build\Release\MinecraftAI.dll` (+ `config.json`)

### 3-C. ビルド結果の確認（任意）

DLL は x64 の PE DLL で、`MinecraftAIStart` / `MinecraftAIStop` をエクスポートします。

```bash
python3 - <<'PY'
d = open("dist/MinecraftAI.dll", "rb").read()
assert d[:2] == b"MZ", "PE ではない"
print("サイズ:", len(d), "bytes")
PY
```

inject 後は `minecraft_ai.log` の先頭に次のような行が出ます（JPEG 経路の自己診断）:

```
=== MinecraftAI.dll injected ===
Worker start. model=gemma4:e4b goal=...
JPEG encoder OK: WIC JPEG ... bytes (base64)
F8 = AI ON/OFF, F9 = emergency stop
```

## 4. FateInjector で inject する

1. マイクラ統合版を起動し、シングルプレイのワールドに入る（タイトル画面でも可）。
2. **FateInjector を管理者として実行**（右クリック → 管理者として実行）。
   - 管理者でないと UWP プロセスを `PROCESS_ALL_ACCESS` で開けず、inject が静かに失敗します。
3. Process 一覧から **`Minecraft.Windows.exe`** を選択する。
   - 一覧に出ない場合は、マイクラを起動した状態で FateInjector の Refresh を押す。
4. DLL 一覧に `dist\MinecraftAI.dll`（MinGW ビルド）または `build\Release\MinecraftAI.dll`（MSVC ビルド）を追加する。
5. **Inject** を押す。

### UWP 特有の仕組み（詰まったときに読む）

- 統合版は AppContainer で動くため、**DLL 本体に「ALL APPLICATION PACKAGES」(SID `S-1-15-2-1`) の読み取り/実行権限**が必要です。FateInjector は inject 前にこれを自動で付与します（`SetAccessControl()`）。
- 手動でやる場合: DLL を右クリック → プロパティ → セキュリティ → 編集 → 追加 → グループ名に `ALL` と入力 → 「ALL APPLICATION PACKAGES」を選び、読み取りと実行を許可。CLI なら管理者 PowerShell で:
  ```powershell
  icacls "C:\path\to\MinecraftAI.dll" /grant "*S-1-15-2-1:(RX)"
  ```
- DLL が**読み取り専用**、または `Program Files` 配下など書き込み不可の場所にあると失敗しやすいので、`C:\MinecraftAI\` のような普通のフォルダに置いてください。
- 成功すると DLL と同じフォルダに `minecraft_ai.log` が作られ、`Worker start. model=...` と出ます。

### 操作方法

| キー | 動作 |
|------|------|
| `F8` | AI ON/OFF トグル |
| `F9` | 緊急停止（全キー解除 + ループ終了） |

AI が動いている間は、マイクラのウィンドウをフォアグラウンドにしておいてください（`SendInput` はフォアグラウンドのウィンドウに届きます）。`config.json` の `goal` を変えると AI の目的が変わります。

```json
{
  "model": "gemma4:e4b",
  "goal": "木を集めて作業台を作る。障害物があれば回避する。",
  "interval_ms": 1200,
  "capture_width": 800
}
```

## 5. マイクラ側の設定（重要）

Bedrock は GDK 版への移行以降、**Raw Input が有効だと SendInput の相対マウス移動を拾いません**（視点が回らない）。

- `設定 → コントローラー → マウスの設定 → Raw Input` を **OFF**
- 併せて「入力応答の改善 (Improved Input Response)」は **ON**、V-Sync は **OFF** にすると安定しやすい
- フルスクリーンより**ウィンドウ表示（ボーダーレス）**の方がキャプチャが安定します

## 6. うまくいかないとき

- **`Minecraft window not found` が続く**: ウィンドウタイトルに「Minecraft」が含まれているか確認。最小化中はキャプチャしない仕様です。
- **`JPEG encoder NG` がログに出る**: WIC が使えていません。`windowscodecs.dll` が存在するか（Windows 7 以降なら標準）、セキュリティソフトにブロックされていないか確認。MSVC ビルドなら GDI+ 版になるのでこのエラーは出ません。
- **キャプチャが真っ黒 / AI が同じ判断を繰り返す**: GPU レンダリング面は `PrintWindow` で黒くなる場合があります。本 DLL は平均輝度で黒フレームを検出し `BitBlt`（画面上のウィンドウ矩形）に自動フォールバックしますが、ウィンドウが他ウィンドウに隠れていると正しく取れません。マイクラを最前面に。
- **`Ollama request failed`**: Ollama が起動しているか（`ollama list`）、`config.json` の `model` 名が実在するタグか、Ollama が 0.20 以上か。`tools/test_ollama.py` で切り分け。
- **JSON パースに失敗する**: `think:false` を送っていてもモデルが thinking を出す場合は、`--model gemma4:12b` など別タグを試す。MLX 版タグは使わない。
- **inject が失敗する / DLL がロードされない**: 管理者実行・x64 DLL・ACL (`S-1-15-2-1`)・VC++ 再頒布可能パッケージを確認。それでもダメな環境では inject なし版（下記）を使ってください。
- **AI が遅い / 動きがぎこちない**: 1 回の推論に vision 付きで数十秒かかることがあります。`interval_ms` を 2000 程度に、`capture_width` を 640 に下げる、`gemma4:e2b` に落とす、GPU を使う、などで改善します。
- **キーが押しっぱなしになる**: `F9` で緊急停止。`ReleaseAllKeys()` が W/A/S/D/SHIFT/CTRL/SPACE などを全て離します。

## 7. inject なし版（外部コントローラ）

UWP で inject できない環境向けに、同じ仕組みを Python で動かす版もあります。DLL より安定して動くことが多いです（ただしゲーム画面を前面に出す必要あり）。

```powershell
pip install mss pyautogui pillow requests
python tools/external_controller.py --goal "木を集めて作業台を作る" --model gemma4:e4b
```

F8 で ON/OFF、F9 で終了。`mss` はデスクトップ全体を撮るので、マイクラを前面に置いてください。

## 8. 仕組み（ざっくり）

- **キャプチャ**: `PrintWindow(hwnd, hdc, PW_RENDERFULLCONTENT)` → 黒フレーム判定（平均輝度 < 4 でサンプリング判定）→ 必要なら `BitBlt` → 縮小 → JPEG 品質 80 → base64。
  - JPEG エンコードは 2 系統。**WIC 版**（`src/jpeg_encode.cpp` の既定。MinGW でも動く）と **GDI+ 版**（`-DDX_WITH_GDI_PLUS` / MSVC ビルド）。
- **推論**: Ollama `/api/chat` に
  ```json
  {"model":"gemma4:e4b","stream":false,"think":false,
   "format":{ ...JSON Schema... },
   "options":{"temperature":0.1,"num_predict":256},
   "messages":[{"role":"user","content":"...","images":["<base64>"]}]}
  ```
  を POST。`format` にスキーマを渡すことで grammar 制約がかかり、確実にパース可能な JSON が返ります。
- **タイムアウト**: vision 推論は 10〜30 秒かかることがあるため、受信タイムアウトを 120 秒に設定。
- **アクション形式**: `{"keys":["W"],"mouse_dx":0,"mouse_dy":0,"click_left":false,"click_right":false,"duration_ms":600,"reason":"..."}` のみを返させる。
- **入力**: `SendInput` でキー/マウスをシミュレート。対応キー: W/A/S/D/SPACE/SHIFT/CTRL/E/Q/F/R/1-9/ESC/TAB/ENTER。

## 9. ファイル構成

```
minecraft-ai-dll/
├─ CMakeLists.txt / build.bat / build_mingw.sh
├─ dist/                 ← MinGW-w64 でビルド済みの DLL
├─ config.json          ← モデル名・目標・間隔を設定
├─ src/
│  ├─ dllmain.cpp       ← DllMain + ワーカースレッド (F8/F9)
│  ├─ screen_capture.*  ← ウィンドウ探索 + 黒フレーム検出 (PrintWindow → BitBlt)
│  ├─ jpeg_encode.*     ← 縮小 + JPEG + base64 (WIC / GDI+ 切替)
│  ├─ ollama_client.*   ← Ollama 通信 (JSON Schema 強制) + パース
│  ├─ input_sim.*       ← SendInput 実行
│  ├─ config.h / base64.h
├─ tools/
│  ├─ test_ollama.py    ← Ollama 接続/モデル/vision テスト
│  └─ external_controller.py ← inject なし版
└─ README.md
```

## 10. 参考にした情報源

- gemma4 の Ollama タグ・必要バージョン（gemma4all.com / verdent.ai / avenchat.com のガイド）
- Ollama vision API: `/api/chat` の `images` は base64 必須（Ollama 公式 Capabilities ドキュメント）
- Ollama 構造化出力: `format` に JSON Schema → grammar 制約（Ollama issue #16776 で MLX 未対応を確認）
- FateInjector: 管理者実行・ACL `S-1-15-2-1` 自動付与・VC++ 再頒布（DeepWiki fligger/FateInjector）
- WIC (`IWICImagingFactory` / `WICCreateImagingFactory_Proxy`) による JPEG エンコード: Microsoft Learn "Windows Imaging Component" / "IWICBitmapEncoder"
- UWP/GDK のキャプチャと Raw Input の既知問題（cua.ai の解説、MCPE issue、Reddit 報告）
