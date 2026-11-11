#pragma once
#include "config.h"
#include "input_sim.h"
#include <string>

// 画面 base64 + 目標を Ollama (gemma系 vision) に投げて AIAction を返す
// 失敗したら false を返し、a は未設定
bool AskGemma(const std::string& base64Image, const Config& cfg, AIAction& out, std::string& rawResponse);
