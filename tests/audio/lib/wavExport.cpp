#include <cstdint>
#include <fstream>
#include "wavExport.h"

void finalizeWav(std::ofstream& file, uint32_t totalRawBytes, uint16_t channels, uint32_t sampleRate, uint16_t bitsPerSample) {
    WavHeader header;
    header.numChannels = channels;
    header.sampleRate = sampleRate;
    header.bitsPerSample = bitsPerSample;

    // Calculate dependent values
    header.byteRate = sampleRate * channels * bitsPerSample / 8;
    header.blockAlign = channels * bitsPerSample / 8;
    header.dataSize = totalRawBytes;
    header.fileSize = totalRawBytes + 36;

    // Seek to start and write the filled header
    file.seekp(0, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.close();
}
