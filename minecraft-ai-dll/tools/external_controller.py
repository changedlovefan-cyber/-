#!/usr/bin/env python3
"""inject なし版（外部コントローラ）。
DLL inject が UWP でブロックされる場合の代替。
画面キャプチャ(mss) → Ollama → pyautogui で操作する。
シングルプレイの自分のワールドでのみ使ってください。

使い方:
  pip install mss pyautogui pillow requests
  python tools/external_controller.py --goal "木を集める"

F8: ON/OFF, F9: 緊急停止（このスクリプトは F9 で終了）
"""
import argparse, base64, io, json, time, sys, ctypes
import requests

try:
    import mss
    from PIL import Image
    import pyautogui
except ImportError:
    print("不足モジュールがあります: pip install mss pyautogui pillow requests")
    sys.exit(1)

KEYMAP = {"W":"w","A":"a","S":"s","D":"d","SPACE":"space","SHIFT":"shift","CTRL":"ctrl",
          "E":"e","Q":"q","F":"f","R":"r","ESC":"esc","TAB":"tab","ENTER":"enter"}

def capture_b64(width=800):
    with mss.mss() as sct:
        shot = sct.grab(sct.monitors[1])
        img = Image.frombytes("RGB", shot.size, shot.rgb)
        h = int(img.height * width / img.width)
        img = img.resize((width, h))
        buf = io.BytesIO()
        img.save(buf, format="JPEG", quality=80)
        return base64.b64encode(buf.getvalue()).decode()

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
    "required": ["keys", "mouse_dx", "mouse_dy", "click_left",
                 "click_right", "duration_ms", "reason"],
}


def ask(img_b64, model, goal):
    prompt = (
        "あなたは Minecraft 統合版を操作する AI です。スクリーンショットを見て JSON だけで返してください。\n"
        f"目標: {goal}\n"
        '{"keys":["W"],"mouse_dx":0,"mouse_dy":0,"click_left":false,"click_right":false,"duration_ms":600,"reason":"..."}'
    )
    r = requests.post("http://127.0.0.1:11434/api/chat", json={
        "model": model, "stream": False, "think": False, "format": SCHEMA,
        "options": {"temperature": 0.1},
        "messages": [{"role": "user", "content": prompt, "images": [img_b64]}],
    }, timeout=120)
    r.raise_for_status()
    content = r.json()["message"]["content"]
    s, e = content.find("{"), content.rfind("}")
    return json.loads(content[s:e+1])

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", default="gemma4:e4b")
    ap.add_argument("--goal", default="木を集めて作業台を作る")
    ap.add_argument("--interval", type=float, default=1.2)
    args = ap.parse_args()
    print("起動。F8=ON/OFF F9=終了。Minecraft をフォアグラウンドにしてください。")
    enabled = True
    while True:
        # F8/F9 ポーリング（pyautogui）
        if pyautogui.isKeyDown("f9"):
            break
        if pyautogui.isKeyDown("f8"):
            enabled = not enabled
            print("AI", "ON" if enabled else "OFF")
            time.sleep(0.5)
        if not enabled:
            time.sleep(0.2); continue
        try:
            act = ask(capture_b64(), args.model, args.goal)
        except Exception as e:
            print("Ollama エラー:", e); time.sleep(2); continue
        print(act)
        dx, dy = int(act.get("mouse_dx", 0)), int(act.get("mouse_dy", 0))
        if dx or dy:
            pyautogui.moveRel(dx, dy, duration=0.05)
        if act.get("click_left"):
            pyautogui.click(button="left")
        if act.get("click_right"):
            pyautogui.click(button="right")
        keys = [KEYMAP.get(k.upper()) for k in act.get("keys", [])]
        keys = [k for k in keys if k]
        dur = max(0.1, min(2.0, int(act.get("duration_ms", 600)) / 1000))
        for k in keys:
            pyautogui.keyDown(k)
        time.sleep(dur)
        for k in keys:
            pyautogui.keyUp(k)
        time.sleep(args.interval)

if __name__ == "__main__":
    main()
