#pragma once

#include "../ModulationStrategies/modulationStrategy.h"
#include "../modulation.h"
#include <cstdint>
#include <functional>
#include <vector>

/*
 * Compares two messages, returns number of bits that differ
 */
int compare(const std::vector<uint8_t> &msg1, const std::vector<uint8_t> &msg2);

/*
 * Generates a random message of given length seeded by seed.
 */
std::vector<uint8_t> gen_random_msg(int seed, int length);

/*
 * Callback types used by tests:
 *  - TxCallback receives the generated waveform that would be
 * played/transmitted.
 *  - RxCallback receives the received message bytes when a message is
 * available.
 */
using TxCallback = std::function<void(const waveform &)>;
using RxCallback = std::function<void(const std::vector<uint8_t> &)>;

/*
 * generates random messages and transfers them - calls tx_callback for each
 * transmit
 *
 * Parameters:
 *  - sm: initialized SignalModulation Instance
 *  - tx_callback: called for each generated waveform
 *  - messages: number of messages to generate/transmit (default 8)
 *  - msg_len: message length in bytes (default 8)
 *  - base_seed: base seed used for reproducible generation (if 0,
 * implementation may use time)
 */
void test_tx(SignalModulation* sm, const TxCallback &tx_callback,
             int messages = 8, int msg_len = 16, int base_seed = 0);

/*
 * receives given messages, compares them with generated ones and prints number
 * of broken bits for each one.
 *
 * Parameters:
 *  - strategy: modulation strategy (may be used by receiver logic)
 *  - rx_callback: will be invoked when a message is received; test harness
 * should compare it
 *  - messages: number of messages expected (default 8)
 *  - msg_len: expected message length in bytes (default 8)
 *  - base_seed: base seed used to generate the expected messages (must match
 * test_tx)
 */
void test_rx(SignalModulation *sm,
             int messages = 8, int msg_len = 16, int base_seed = 0);
