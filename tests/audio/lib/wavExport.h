#pragma once
#include <cstdint>
#include <fstream>

struct WavHeader {
    // RIFF Chunk
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize;          // (RawDataSize + 36)
    char wave[4] = {'W', 'A', 'V', 'E'};

    // fmt Chunk
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;      // Size of the fmt chunk
    uint16_t audioFormat = 3;   // 1 for PCM
    uint16_t numChannels = 1;       // 1 for Mono, 2 for Stereo
    uint32_t sampleRate = 48000;
    uint32_t byteRate;          // (SampleRate * NumChannels * BitsPerSample / 8)
    uint16_t blockAlign;        // (NumChannels * BitsPerSample / 8)
    uint16_t bitsPerSample = 32;

    // data Chunk
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize;          // Total bytes of raw audio
};

void finalizeWav(std::ofstream& file, uint32_t totalRawBytes, uint16_t channels, uint32_t sampleRate, uint16_t bitsPerSample);
