MinecraftAI.dll (MinGW-w64 x86_64 ビルド / 未署名)
=====================================================

置き方
------
このフォルダ (dist) ごと C:\MinecraftAI などへコピーしてください。
  C:\MinecraftAI\MinecraftAI.dll
  C:\MinecraftAI\config.json
  C:\MinecraftAI\minecraft_ai.log   ← inject 後に自動生成

inject
------
1. Minecraft 統合版を起動（ワールドに入る）
2. FateInjector を「管理者として実行」
3. Process: Minecraft.Windows.exe を選択
4. DLL: C:\MinecraftAI\MinecraftAI.dll を追加
5. Inject

操作
----
F8: AI ON/OFF   F9: 緊急停止

前提
----
- Ollama 0.20 以上 + `ollama pull gemma4`
- x64 DLL（このファイル）
- ゲーム設定で Raw Input を OFF
- DLL の ACL に ALL APPLICATION PACKAGES (S-1-15-2-1) の読み取り権限
  （FateInjector が自動で付与します。手動なら:
    icacls MinecraftAI.dll /grant *S-1-15-2-1:(RX) )

詳細は ../README.md を参照。
