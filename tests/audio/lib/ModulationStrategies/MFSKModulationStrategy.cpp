#include "../modulation.h"
#include "../waveforms.h"
#include "modulationStrategy.h"
#include <ostream>
#include <string>
#include <vector>
#include <cmath>    // Required for log2
#include <iostream> // Required for std::cerr

MFSKModulationStrategy::MFSKModulationStrategy(int start_freq, int freq_spacing,
                                               int M, int num_regions) {
    this->M = M;
    this->freqs.clear();

    // Resize freqs to hold the number of regions
    this->freqs.resize(num_regions);

    for (int i = 0; i < num_regions; i++) {
        for (int j = 0; j < M; j++) {
            // Correctly map frequencies to specific regions
            this->freqs[i].push_back(start_freq + (i * M * freq_spacing) + (j * freq_spacing));
        }
    }
}

std::vector<bool> MFSKModulationStrategy::demodulate(int frame_offset) {
    Spectrum *s = sm->get_spectrum(frame_offset);
    std::vector<bool> out;
    int bits_per_symbol = static_cast<int>(std::log2(M));

    for (const auto& region : freqs) {
        float max_strength = p->lowest_strength;
        int best_index = 0;

        // Find the index (0 to M-1) with the highest strength
        for (size_t i = 0; i < region.size(); ++i) {
            float strength = s->strength(region[i]);
            if (strength > max_strength) {
                max_strength = strength;
                best_index = static_cast<int>(i);
            }
        }

        // Convert the index back to bits
        for (int i = bits_per_symbol - 1; i >= 0; i--) {
            out.push_back((best_index >> i) & 1);
        }
    }

    return out;
}

std::string MFSKModulationStrategy::modulate(const std::vector<bool> &data) {
    int bits_per_symbol = static_cast<int>(std::log2(M));
    int bits_per_frame = static_cast<int>(freqs.size()) * bits_per_symbol;

    if (data.size() % bits_per_frame != 0) {
        std::cerr << "Data not aligned to frame size." << std::endl;
        return "";
    }

    std::string out = "";
    int num_frames = static_cast<int>(data.size() / bits_per_frame);

    for (int i = 0; i < num_frames; i++) {
        for (size_t region = 0; region < freqs.size(); region++) {
            int symbol_index = 0;

            // Extract the bits for this specific region/symbol
            for (int j = 0; j < bits_per_symbol; j++) {
                int bit_idx = (i * bits_per_frame) + (region * bits_per_symbol) + j;
                if (data[bit_idx]) {
                    symbol_index |= (1 << (bits_per_symbol - 1 - j));
                }
            }

            int bin = freqs[region][symbol_index];
            out += std::to_string(bin_to_freq(bin));

            if (region < freqs.size() - 1) {
                out += ",";
            }
        }

        if (i < num_frames - 1) {
            out += "|";
        }
    }

    return out;
}

bool MFSKModulationStrategy::has_noise(Spectrum *s) {
    for (const auto& r : freqs) {
        for (int f : r) {
            if (s->strength(f) > p->strength_threshold) {
                return true;
            }
        }
    }
    return false;
}

void MFSKModulationStrategy::print(std::ostream &os) const {
    os << "MFSK" << std::endl;
}

int MFSKModulationStrategy::bits_per_frame() {
  return (int)log2(M) * freqs.size();
}
