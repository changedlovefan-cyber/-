#pragma once
#include <string>
#include <vector>
#include <windows.h>

inline std::string Base64Encode(const std::vector<unsigned char>& data) {
    static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    for (size_t i = 0; i < data.size(); i += 3) {
        int remaining = (int)data.size() - (int)i;
        unsigned int v = data[i] << 16;
        if (remaining > 1) v |= data[i + 1] << 8;
        if (remaining > 2) v |= data[i + 2];
        out.push_back(tbl[(v >> 18) & 63]);
        out.push_back(tbl[(v >> 12) & 63]);
        out.push_back(remaining > 1 ? tbl[(v >> 6) & 63] : '=');
        out.push_back(remaining > 2 ? tbl[v & 63] : '=');
    }
    return out;
}
