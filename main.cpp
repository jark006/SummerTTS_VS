#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>

#include "SynthesizerTrn.h"
#include "utils.h"

#include <windows.h>
#include <conio.h>
#include <Mmsystem.h>
#pragma comment(lib,"winmm.lib")

using std::cout;
using std::cerr;
using std::endl;
using std::vector;

// 默认调试参数： .\test.txt D:\SummerTTSModels\single_speaker_fast.bin D:\SummerTTSModels\single_speaker_english_fast.bin D:\out.wav

int main(int argc, char** argv) {
    if (argc < 5) {
        cerr << "Need 4 parameters.\nUsage: ./SummerTTS_VS.exe textPath CN_modelPath EN_modelPath outputPath\n\n";
        return -1;
    }
    setvbuf(stdout, NULL, _IONBF, 0);//禁用输出缓冲

    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    cout << "Loading text... [" << argv[1];
    TextSet textSet(argv[1]);
    cout << "] Done.\n";

    cout << "Loading model... ";
    ModelData cnModel(argv[2]), enModel(argv[3]);
    SynthesizerTrn synthesizerCN(cnModel.get(), cnModel.fileSize); // 中文
    SynthesizerTrn synthesizerEN(enModel.get(), enModel.fileSize); // 英文
    cout << " Done.\n";

    vector<wavSliceStruct> retList;
    int32_t cnt = 0, totalLen = 0;
    cout << "\rInferring ...";
    for (auto& t : textSet.textList) {
        wavSliceStruct ret;
        if (t.isCN) ret.wavData = synthesizerCN.infer(t.text, 0, 1.0);
        else ret.wavData = synthesizerEN.infer(t.text, 0, 1.0);

        int32_t i = 0;
        while (i < ret.wavData.size() && abs(ret.wavData[i]) < 50) i++;
        ret.startIdx = i;

        i = ret.wavData.size() - 1;
        while (i > 0 && abs(ret.wavData[i]) < 50) i--;
        ret.endIdx = i;

        //保留有效语音片段，尽量去除头尾空白音频
        if (ret.endIdx > ret.startIdx) {
            totalLen += ret.endIdx - ret.startIdx;
            retList.emplace_back(ret);
        }

        cnt++;
        int percent = (cnt * 50ULL / textSet.textList.size());
        cout << "\rInferring [";
        for (int i = 0; i < 50; i++) cout << (i < percent ? '>' : '-');
        cout << "] [" << cnt << '/' << textSet.textList.size() << ']';
    }

    auto res = saveToWavBuffer(retList, totalLen * sizeof(int16_t));

    cout << "\nPlaying... ";
    PlaySound((LPCSTR)res.data(), 0, SND_MEMORY | SND_SYNC);
    cout << "Done.\n";

    cout << "\nSaving [" << argv[4] << "]... ";
    FILE* fpOut = fopen(argv[4], "wb");
    if (!fpOut) {
        std::cerr << "\nFile [" << argv[4] << "] write fail.\n";
        exit(-1);
    }
    fwrite(res.data(), 1, res.size(), fpOut);
    fclose(fpOut);
    cout << "Done.\n";

    return 0;
}


