#pragma once
#include <windows.h>
#include <string>
#include <vector>

struct AIAction {
    std::vector<std::string> keys; // "W","A","S","D","SPACE","SHIFT","CTRL","E","Q","F","R","1".."9" など
    int mouseDx = 0;   // 視点回転
    int mouseDy = 0;
    bool clickLeft = false;
    bool clickRight = false;
    int durationMs = 600; // キーを押し続ける時間
    std::string reason;
};

// AIAction を SendInput で実行する
void ExecuteAction(const AIAction& a);

// 全キーを離す（緊急停止用）
void ReleaseAllKeys();
