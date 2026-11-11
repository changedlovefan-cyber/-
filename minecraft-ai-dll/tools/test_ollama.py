#!/usr/bin/env python3
"""Ollama + gemma4 が動いているか / vision と JSON 強制が効くかをテストする。

inject なしで切り分けできる:
  1) /api/tags でモデル一覧と有無を確認
  2) テキストだけで JSON Schema 強制が効くか
  3) 小さな画像を送って vision 入力が通るか
使い方:
  python tools/test_ollama.py
  python tools/test_ollama.py --model gemma4:12b
"""
import argparse
import base64
import json
import struct
import sys
import urllib.request
import zlib

DEFAULT_MODEL = "gemma4:e4b"
BASE = "http://127.0.0.1:11434"

SCHEMA = {
    "type": "object",
    "properties": {
        "keys": {"type": "array", "items": {"type": "string"}},
        "mouse_dx": {"type": "integer"},
        "mouse_dy": {"type": "integer"},
        "click_left": {"type": "boolean"},
        "click_right": {"type": "boolean"},
        "duration_ms": {"type": "integer"},
        "reason": {"type": "string"},
    },
    "required": [
        "keys", "mouse_dx", "mouse_dy",
        "click_left", "click_right", "duration_ms", "reason",
    ],
}

PROMPT = (
    "あなたは Minecraft 統合版を操作する AI です。"
    "今は何も見えていないので、その場で 1 秒前進してください。"
    "JSON のみで返してください。"
)


def _png(w=64, h=64):
    """依存なしでテスト用の単色 PNG を作る。"""
    raw = b"".join(b"\x00" + bytes([60, 120, 200]) * w for _ in range(h))

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw))
        + chunk(b"IEND", b"")
    )


def _post(path, body, timeout=180):
    req = urllib.request.Request(
        BASE + path,
        data=json.dumps(body).encode(),
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return json.loads(r.read().decode())


def list_models():
    try:
        with urllib.request.urlopen(BASE + "/api/tags", timeout=10) as r:
            tags = json.loads(r.read().decode())
    except Exception as e:
        print("NG: Ollama に接続できません:", e)
        print("  → `ollama serve` を起動してください（Ollama 0.20 以上が必要）。")
        return None
    names = [m.get("name", "") for m in tags.get("models", [])]
    print("=== インストール済みモデル ===")
    for n in names:
        print("  -", n)
    if not names:
        print("  (なし)")
    return names


def check_version():
    try:
        with urllib.request.urlopen(BASE + "/api/version", timeout=10) as r:
            v = json.loads(r.read().decode()).get("version", "?")
        print("Ollama version:", v)
        major_minor = [int(x) for x in v.split(".")[:2] if x.isdigit()]
        if len(major_minor) == 2 and (major_minor[0] < 0 or (major_minor[0] == 0 and major_minor[1] < 20)):
            print("  警告: gemma4 には Ollama 0.20 以上が必要です。`ollama update` してください。")
    except Exception:
        pass


def text_json_test(model):
    print("\n=== 1) テキスト + JSON Schema 強制 ===")
    body = {
        "model": model,
        "stream": False,
        "think": False,
        "format": SCHEMA,
        "options": {"temperature": 0},
        "messages": [{"role": "user", "content": PROMPT}],
    }
    try:
        data = _post("/api/chat", body)
    except Exception as e:
        print("NG:", e)
        print("  → モデル名が違う可能性。`ollama pull %s` を実行してください。" % model)
        return False
    content = data.get("message", {}).get("content", "")
    print("応答:", content[:400])
    try:
        act = json.loads(content)
        assert isinstance(act.get("keys"), list)
        print("OK: JSON パース成功 / keys =", act["keys"])
        return True
    except Exception as e:
        print("NG: JSON として不正:", e)
        return False


def vision_test(model):
    print("\n=== 2) 画像入力 (vision) ===")
    img = base64.b64encode(_png()).decode()
    body = {
        "model": model,
        "stream": False,
        "think": False,
        "messages": [{
            "role": "user",
            "content": "この画像の色を一言で。「青」のように答えだけ。",
            "images": [img],
        }],
    }
    try:
        data = _post("/api/chat", body)
    except Exception as e:
        print("NG:", e)
        print("  → vision 非対応のモデルか、古い Ollama の可能性。")
        return False
    content = data.get("message", {}).get("content", "")
    print("応答:", content[:200])
    if content.strip():
        print("OK: 画像が読めています")
        return True
    print("NG: 空の応答")
    return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", default=DEFAULT_MODEL,
                    help="使用する Ollama モデル (既定: %s)" % DEFAULT_MODEL)
    args = ap.parse_args()

    names = list_models()
    if names is None:
        sys.exit(1)
    check_version()

    if not any(n.split(":")[0] == args.model.split(":")[0] for n in names):
        print("\n警告: %s が見つかりません。`ollama pull %s` を実行してください。" % (args.model, args.model))

    ok1 = text_json_test(args.model)
    ok2 = vision_test(args.model) if ok1 else False

    print("\n=== 結果 ===")
    print("JSON 強制:", "OK" if ok1 else "NG")
    print("vision   :", "OK" if ok2 else "NG")
    sys.exit(0 if (ok1 and ok2) else 1)


if __name__ == "__main__":
    main()
