#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>

#include "SynthesizerTrn.h"
#include "utils.h"

using std::cout;
using std::cerr;
using std::endl;
using std::vector;


int main(int argc, char** argv) {
    if (argc < 5) {
        cerr << "Need 4 parameters.\nUsage: ./SummerTTS_VS.exe textPath CN_modelPath EN_modelPath outputPath\n\n";
        return -1;
    }
    setvbuf(stdout, NULL, _IONBF, 0);//禁用输出缓冲


    cout << "Loading text... [" << argv[1];
    TextSet textSet(argv[1]);
    cout << "] Done.\n";

    cout << "Loading model... ";
    ModelData cnModel(argv[2]), enModel(argv[3]);
    SynthesizerTrn synthesizerCN(cnModel.get(), cnModel.size); // 中文
    SynthesizerTrn synthesizerEN(enModel.get(), enModel.size); // 英文
    cout << " Done.\n";

    vector<retStruct> retList;
    int32_t cnt = 0, totalLen = 0;
    cout << "\rInferring ...";
    for (auto& t : textSet.textList) {
        retStruct ret;
        if (t.isCN)ret.wavData = synthesizerCN.infer(t.text, 0, 1.0, ret.len);
        else ret.wavData = synthesizerEN.infer(t.text, 0, 1.0, ret.len);

        int32_t i = 0;
        while (i < ret.len && abs(ret.wavData[i]) < 50) i++;
        ret.startIdx = i;

        i = ret.len - 1;
        while (i > 0 && abs(ret.wavData[i]) < 50) i--;
        ret.endIdx = i;

        //保留有效语音片段，尽量去除头尾空白音频
        if (ret.endIdx > ret.startIdx) {
            totalLen += ret.endIdx - ret.startIdx;
            retList.emplace_back(ret);
        }

        cnt++;
        int percent = (cnt * 50 / textSet.textList.size());
        cout << "\rInferring [";
        for (int i = 0; i < 50; i++) cout << (i < percent ? '>' : '-');
        cout << "] [" << cnt << '/' << textSet.textList.size() << ']';
    }

    cout << "\nSaving [" << argv[4] << "]... ";
    saveToWavFile(argv[4], retList, totalLen * sizeof(int16_t));
    cout << "Done.\n";

    return 0;
}


