#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <security/_pam_types.h>
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <vector>
#include "../audio/audio-control.h"
#include "otp.cpp"
#include <iostream>

#define MAX_RETRIES 3

std::vector<uint8_t> data;
std::vector<uint8_t> challenge;
AudioControl* a;
bool success = false;
int retries = 0;

void stop (int signal) {
    if (a != NULL) {
        a->end_loop();
    }
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    std::cout << "Hello from pam_sm_authenticate" << std::endl;
    std::signal(SIGINT, stop);

    challenge = get_challenge();
    // std::vector<unsigned char> msg = generate(challenge);
    AudioControl* audio = new AudioControl();
    a = audio;

    Communication* comm = new Communication(audio);
    std::vector<uint8_t> message;

    message.insert(message.end(), challenge.begin(), challenge.end());

    comm->encode_message(message);
    comm->receive_callback = [comm, audio](){
        std::cout << "Received message" << std::endl;
        int ret = comm->get_data(const_cast<std::vector<uint8_t>&>(data));
        std::cout << "Data size: " << ret << " " << data.size() << std::endl;
        std::cout << "Message: " << std::endl;
        success = true;
        for (size_t i = 0; i < data.size(); i++) {
            if (data[i] != challenge[i]) {
                success = false;
                break;
            }
        }
        std::cout << "Success " << success << std::endl;
        if (success) {
            audio->end_loop();
        } else {
            retries ++;
            if (retries == MAX_RETRIES) {
                audio->end_loop();
            }
        }
    };
    std::vector<uint8_t> waveform = comm->get_waveform();
    audio->queue_audio(waveform);
    audio->start_loop();
    std::cout << "Ended" << std::endl;
    std::signal(SIGINT, SIG_DFL);
    if (success) {
        return PAM_SUCCESS;
    } else return PAM_AUTH_ERR;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
  return PAM_SUCCESS;
}
