#ifndef _TTS_UTILS_H_
#define _TTS_UTILS_H_

#include<iostream>
#include<fstream>
#include<sstream>
#include<vector>
#include<list>
#include<map>
#include<utility>
#include<string>
#include<cstdlib>
#include<cstdio>

#include<fcntl.h>
#include"sys/stat.h"

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::string;


struct textStruct {
    bool isCN = true;
    std::string text;
    textStruct(bool _isCN, std::string& _text) :isCN(_isCN), text(std::move(_text)) {}
};

struct wavSliceStruct{
    int32_t startIdx = 0;
    int32_t endIdx = 0;
    vector<int16_t> wavData;
};

int ttsLoadModel(char* ttsModelName, float** ttsModel);
void tts_free_data(void* data);
void saveToWavFile(const char* path, vector<wavSliceStruct>& data, int totalAudioLen);
vector<uint8_t> saveToWavBuffer(vector<wavSliceStruct>& data, int totalAudioLen);

class TextSet {
public:
    std::list<textStruct> textList;

    TextSet(const char* path) {
        struct stat st;
        if (stat(path, &st) < 0 || st.st_size == 0) {
            std::cerr << "File [" << path << "] get stat fail.\n";
            exit(-1);
        }

        std::stringstream ss;
        ss << std::ifstream(path).rdbuf();
        auto str = preProcess(ss.str()); // 预处理：只保留汉字，英文，数字，替换部分符号

        int idxLast = 0, idx = 0;
        while (idx < str.length()) { // 分割中英文
            while (idx < str.length() && (((uint8_t)str[idx]) >= 128 || isdigit(str[idx]) || str[idx] == ' '))idx++;
            if (idx > idxLast) {
                auto tmp = Trim(str.substr(idxLast, idx - idxLast));
                if (tmp.length() > 0)
                    textList.emplace_back(textStruct{ true, tmp });
            }
            idxLast = idx;

            while (idx < str.length() && ((uint8_t)str[idx]) < 128 && !isdigit(str[idx]))idx++;
            if (idx > idxLast) {
                auto tmp = Trim(str.substr(idxLast, idx - idxLast));
                if (tmp.length() == 1)
                    tmp += '.'; // 若句子只有一个字母，模型在推导时，程序会崩
                if (tmp.length() > 0)
                    textList.emplace_back(textStruct{ false, tmp });
            }
            idxLast = idx;
        }
    }

    static size_t strReplace(std::string& src, const std::string& oldBlock, const std::string& newBlock) {
        if (oldBlock.empty())return 0;
        size_t cnt = 0, nextBeginIdx = 0, foundIdx;
        while ((foundIdx = src.find(oldBlock, nextBeginIdx)) != std::string::npos) {
            src.replace(foundIdx, oldBlock.length(), newBlock);
            nextBeginIdx = foundIdx + newBlock.length();
            cnt++;
        }
        return cnt;
    }

    bool myIsAlpha(int c) {
        return (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'));
    }
    bool myIsAlphaUpper(int c) {
        return ('A' <= c && c <= 'Z');
    }

    bool myIsAlphaNum(int c) {
        return (myIsAlpha(c) || ('0' <= c && c <= '9'));
    }

    std::string preProcess(std::string&& str) {
        static const std::map<const int, const std::string> asciiMap = {
            {'+', "加"},
            {'-', "减"},
            {'*', "乘"},
            {'/', "斜杠"},//除以，日期分隔符
            {'\\', "反斜杠"},
            {'_', "下划线"},
            {'~', "波浪号"},
            {'&', "与"},
            {'|', "或"},
            {'.', "点"},
            {'#', "井号"},
            {'$', "美元"},
            {'%', "百分比"},
            {'<', "小于"},
            {'=', "等于"},
            {'>', "大于"},
        };
        static const std::map<const std::string, const std::string> wordMap{
            {"fps", "帧"},
            {"℃", "摄氏度"},
            {"℉", "华氏度"},
            {"α", "阿尔法"},
            {"β", "贝塔"},
            {"γ", "伽马"},
            {"δ", "德尔塔"},
            {"ε", "艾普西隆"},
            {"ζ", "泽塔"},
            {"η", "伊塔"},
            {"θ", "西塔"},
            {"ι", "约塔"},
            {"κ", "卡帕"},
            {"λ", "兰姆达"},
            {"μ", "缪"},
            {"ν", "纽"},
            {"ξ", "克西"},
            {"ο", "欧米克隆"},
            {"π", "派"},
            {"ρ", "柔"},
            {"σ", "西格玛"},
            {"τ", "陶"},
            {"υ", "宇普西隆"},
            {"φ", "斐"},
            {"χ", "希"},
            {"ψ", "普赛"},
            {"ω", "欧米伽"}
        };
        for (auto& item : wordMap)
            strReplace(str, item.first, item.second);

        int lastCodePoint = 'A', codePoint = 0, i = 0;
        std::vector<char> out;
        out.reserve(str.length());

        while (i < str.length()) {
            // UTF-8 解码
            if ((str[i] & 0x80) == 0) {
                codePoint = str[i];
                i++;
            }
            else if ((str[i] & 0xe0) == 0xc0) { // 110x'xxxx 10xx'xxxx
                codePoint = ((str[i] & 0x1f) << 6) | (str[i + 1] & 0x3f);
                i += 2;
            }
            else if ((str[i] & 0xf0) == 0xe0) { // 1110'xxxx 10xx'xxxx 10xx'xxxx
                codePoint = ((str[i] & 0x0f) << 12) | ((str[i + 1] & 0x3f) << 6) | (str[i + 2] & 0x3f);
                i += 3;
            }
            else if ((str[i] & 0xf8) == 0xf0) { // 1111'0xxx 10xx'xxxx 10xx'xxxx 10xx'xxxx
                codePoint = ((str[i] & 0x07) << 18) | ((str[i + 1] & 0x3f) << 12) | ((str[i + 2] & 0x3f) << 6) | (str[i + 3] & 0x3f);
                i += 4;
            }
            else { // 10xx'xxxx 等等非法
                codePoint = ' ';
                i++;
            }

            if (codePoint < 128) { // ascii
                // 大写字母的前一个无论大小写，都插入空格隔开，单独发音，因为英文模型不会读全大写单词短语
                if (myIsAlphaUpper(codePoint) && myIsAlpha(lastCodePoint)) {
                    out.push_back(' ');
                    out.push_back(codePoint);
                }
                else if (myIsAlphaNum(codePoint)) {
                    out.push_back(codePoint);
                }
                else if (asciiMap.count(codePoint)) {
                    for (const char c : asciiMap.at(codePoint))
                        out.push_back(c);
                }
                else {
                    out.push_back(' ');
                }
            }
            else if (0x4e00 <= codePoint && codePoint <= 0x9fff) { // 中文区
                out.push_back(str[i - 3]);
                out.push_back(str[i - 2]);
                out.push_back(str[i - 1]);
            }
            else { //忽略其他码点
                out.push_back(' ');
            }
            lastCodePoint = codePoint;
        }

        std::string ret(out.begin(), out.end());
        while (strReplace(ret, "  ", " "));
        return ret;
    }

    // 从UTF-8字符串中过滤出汉字和数字
    static std::string filterChineseAndNum(const std::string& str) {
        if (str.empty())
            return "";

        int codePoint = 0, i = 0;
        std::vector<char> out;
        out.reserve(str.length());

        while (i < str.length()) {
            // UTF-8 解码
            if ((str[i] & 0x80) == 0) {
                codePoint = str[i];
                i++;
            }
            else if ((str[i] & 0xe0) == 0xc0) { // 110x'xxxx 10xx'xxxx
                codePoint = ((str[i] & 0x1f) << 6) | (str[i + 1] & 0x3f);
                i += 2;
            }
            else if ((str[i] & 0xf0) == 0xe0) { // 1110'xxxx 10xx'xxxx 10xx'xxxx
                codePoint = ((str[i] & 0x0f) << 12) | ((str[i + 1] & 0x3f) << 6) | (str[i + 2] & 0x3f);
                i += 3;
            }
            else if ((str[i] & 0xf8) == 0xf0) { // 1111'0xxx 10xx'xxxx 10xx'xxxx 10xx'xxxx
                codePoint = ((str[i] & 0x07) << 18) | ((str[i + 1] & 0x3f) << 12) | ((str[i + 2] & 0x3f) << 6) | (str[i + 3] & 0x3f);
                i += 4;
            }
            else { // 10xx'xxxx 等等非法
                codePoint = ' ';
                i++;
            }

            if ('0' <= codePoint && codePoint <= '9') { // ascii 0~9
                out.push_back(codePoint);
            }
            else if (0x4e00 <= codePoint && codePoint <= 0x9fff) { // 中文区
                out.push_back(str[i - 3]);
                out.push_back(str[i - 2]);
                out.push_back(str[i - 1]);
            }
            else { //忽略其他码点
                out.push_back(' ');
            }
        }

        std::string ret(out.begin(), out.end());
        while (strReplace(ret, "  ", " "));
        return ret;
    }

    std::string Trim(const std::string& s, const char* WHITESPACE = " \n\r\t") {
        size_t startpos = s.find_first_not_of(WHITESPACE);
        size_t endpos = s.find_last_not_of(WHITESPACE);
        if (startpos == std::string::npos || endpos == std::string::npos || startpos > endpos)
            return "";
        return s.substr(startpos, endpos - startpos + 1);
    }

    std::string TrimStart(const std::string& s, const char* WHITESPACE = " \n\r\t") {
        size_t startpos = s.find_first_not_of(WHITESPACE);
        return (startpos == std::string::npos) ? "" : s.substr(startpos);
    }

    std::string TrimEnd(const std::string& s, const char* WHITESPACE = " \n\r\t") {
        size_t endpos = s.find_last_not_of(WHITESPACE);
        return (endpos == std::string::npos) ? "" : s.substr(0, endpos + 1);
    }
};

class ModelData {
public:
    size_t fileSize = 0;
    std::vector<uint8_t> dataBuff;
    ModelData(const char* path) {
        struct stat st;
        if (stat(path, &st) < 0 || st.st_size == 0) {
            std::cerr << "File [" << path << "] get stat fail.\n";
            exit(-1);
        }

        fileSize = st.st_size;

        auto fp = fopen(path, "rb");
        if (!fp) {
            std::cerr << "File [" << path << "] open fail.\n";
            exit(-1);
        }
        dataBuff.resize(fileSize);
        auto len = fread(dataBuff.data(), 1, fileSize, fp);
        if (len != fileSize) {
            std::cerr << "Read [" << path << "] error.\n";
            std::cerr << "stat.size: " << fileSize << endl;
            std::cerr << "fread() return size: " << len << endl;
            fclose(fp);
            exit(-1);
        }
        fclose(fp);
    }
    float* get() {
        return (float*)dataBuff.data();
    }
};


#endif
