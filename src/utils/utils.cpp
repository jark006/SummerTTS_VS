#include "utils.h"
#include "stdint.h"
#include "tts_logger.h"
#include "tts_file_io.h"
#include "stdlib.h"
#include "stdio.h"

int ttsLoadModel(char * ttsModelName, float **ttsModel)
{
    TTS_STAT_t st;
    if(-1 == tts_stat(ttsModelName, &st))
    {
        return -1 ;
    }

    TTS_FILE_t *fp = tts_fopen(ttsModelName);
    if(!fp)
    {
        tts_log(TTS_LOG_ERROR,"TTS_SYNC: Fail to open am model file\n");
        return -1;
    }

    //printf("file size :%d\n",st.size_);
    float * modelData = (float *)malloc(st.size_);

    tts_fread(modelData,st.size_,fp);
    tts_fclose(fp);

    *ttsModel = modelData;
    
    return st.size_;
}

void tts_free_data(void * data)
{
     free(data);
}

vector<uint8_t> saveToWavBuffer(vector<wavSliceStruct>& data, int totalAudioLen) {
    char header[44];
    int byteRate = 16 * 16000 * 1 / 8;
    int totalDataLen = totalAudioLen + 36;
    int channels = 1;
    int longSampleRate = 16000;

    header[0] = 'R'; // RIFF/WAVE header
    header[1] = 'I';
    header[2] = 'F';
    header[3] = 'F';
    header[4] = (char)(totalDataLen & 0xff);
    header[5] = (char)((totalDataLen >> 8) & 0xff);
    header[6] = (char)((totalDataLen >> 16) & 0xff);
    header[7] = (char)((totalDataLen >> 24) & 0xff);
    header[8] = 'W';
    header[9] = 'A';
    header[10] = 'V';
    header[11] = 'E';
    header[12] = 'f'; // 'fmt ' chunk
    header[13] = 'm';
    header[14] = 't';
    header[15] = ' ';
    header[16] = 16; // 4 bytes: size of 'fmt ' chunk
    header[17] = 0;
    header[18] = 0;
    header[19] = 0;
    header[20] = 1; // format = 1
    header[21] = 0;
    header[22] = (char)channels;
    header[23] = 0;
    header[24] = (char)(longSampleRate & 0xff);
    header[25] = (char)((longSampleRate >> 8) & 0xff);
    header[26] = (char)((longSampleRate >> 16) & 0xff);
    header[27] = (char)((longSampleRate >> 24) & 0xff);
    header[28] = (char)(byteRate & 0xff);
    header[29] = (char)((byteRate >> 8) & 0xff);
    header[30] = (char)((byteRate >> 16) & 0xff);
    header[31] = (char)((byteRate >> 24) & 0xff);
    header[32] = (char)(1 * 16 / 8); // block align
    header[33] = 0;
    header[34] = 16; // bits per sample
    header[35] = 0;
    header[36] = 'd';
    header[37] = 'a';
    header[38] = 't';
    header[39] = 'a';
    header[40] = (char)(totalAudioLen & 0xff);
    header[41] = (char)((totalAudioLen >> 8) & 0xff);
    header[42] = (char)((totalAudioLen >> 16) & 0xff);
    header[43] = (char)((totalAudioLen >> 24) & 0xff);

    int len = 0;
    for (const auto& r : data) {
        if (r.wavData.empty() || r.startIdx == r.wavData.size())
            continue;
        len += (r.endIdx - r.startIdx);
    }

    vector<uint8_t> ret(sizeof(header) + len * 2);
    memcpy(ret.data(), header, sizeof(header));

    int16_t* ptr = (int16_t*)(ret.data() + sizeof(header));

    int cnt = 0;
    for (const auto& r : data) {
        if (r.wavData.empty() || r.startIdx == r.wavData.size())
            continue;
        int len = (r.endIdx - r.startIdx);
        memcpy(ptr + cnt, r.wavData.data() + r.startIdx, len * sizeof(int16_t));
        cnt += len;
    }
    return ret;
}

void saveToWavFile(const char* path, vector<wavSliceStruct>& data, int totalAudioLen) {
    auto res = saveToWavBuffer(data, totalAudioLen);

    FILE* fpOut = fopen(path, "wb");
    if (!fpOut) {
        std::cerr << "\nFile [" << path << "] write fail.\n";
        exit(-1);
    }

    fwrite(res.data(), 1, res.size(), fpOut);
    fclose(fpOut);
}

