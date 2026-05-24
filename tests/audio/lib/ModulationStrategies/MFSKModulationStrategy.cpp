#include "../modulation.h"
#include "../waveforms.h"
#include "modulationStrategy.h"
#include <ostream>
#include <string>
#include <vector>
#include <cmath>
#include <iostream>

MFSKModulationStrategy::MFSKModulationStrategy(ProtocolConfig* p, int start_freq, int freq_spacing,
                                               int M, int num_regions) {
    this->p = p;
    this->M = M;
    this->freqs.clear();

    this->freqs.resize(num_regions);

    for (int i = 0; i < num_regions; i++) {
        for (int j = 0; j < M; j++) {
            this->freqs[i].push_back(freq_to_bin(start_freq) + (i * M * freq_spacing) + (j * freq_spacing));
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

        for (size_t i = 0; i < region.size(); ++i) {
            float strength = s->strength(region[i]);
            if (strength > max_strength) {
                max_strength = strength;
                best_index = static_cast<int>(i);
            }
        }

        for (int i = bits_per_symbol - 1; i >= 0; i--) {
            out.push_back((best_index >> i) & 1);
        }
    }

    return out;
}

std::string MFSKModulationStrategy::modulate(const std::vector<bool> &data) {
    int bits_per_symbol = static_cast<int>(std::log2(M));
    int bits_per_frame = static_cast<int>(freqs.size()) * bits_per_symbol;
    std::vector<bool> to_transmit(data);

    while (to_transmit.size() % bits_per_frame != 0) {
        to_transmit.push_back(0);
    }

    std::string out = "";
    int num_frames = static_cast<int>(data.size() / bits_per_frame);

    for (int i = 0; i < num_frames; i++) {
        for (size_t region = 0; region < freqs.size(); region++) {
            int symbol_index = 0;

            for (int j = 0; j < bits_per_symbol; j++) {
                int bit_idx = (i * bits_per_frame) + (region * bits_per_symbol) + j;
                if (to_transmit[bit_idx]) {
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
