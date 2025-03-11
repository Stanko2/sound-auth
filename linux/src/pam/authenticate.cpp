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
#define GGWAVE_DISABLE_LOG

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
    std::signal(SIGINT, stop);
    challenge = get_challenge();
    AudioControl* audio = new AudioControl();
    a = audio;

    Communication* comm = new Communication(audio);
    std::vector<uint8_t> message;

    message.insert(message.end(), challenge.begin(), challenge.end());

    comm->encode_message(message);
    comm->receive_callback = [comm, audio](){
        int ret = comm->get_data(const_cast<std::vector<uint8_t>&>(data));
        success = verify(data, challenge);

        if (success) {
            audio->end_loop();
        } else {
            std::cout << "Authentication failed, trying again..." << std::endl;
            retries ++;
            challenge = get_challenge();
            comm->encode_message(challenge);
            auto waveform = comm->get_waveform();
            audio->queue_audio(waveform);

            if (retries == MAX_RETRIES) {
                std::cout << "Maximum retries reached, exiting ..." << std::endl;
                audio->end_loop();
            }
        }
    };
    std::vector<uint8_t> waveform = comm->get_waveform();
    audio->queue_audio(waveform);
    audio->start_loop();

    std::signal(SIGINT, SIG_DFL);

    if (retries == MAX_RETRIES) {
        return PAM_MAXTRIES;
    }

    if (success) {
        return PAM_SUCCESS;
    } else return PAM_AUTH_ERR;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
  return PAM_SUCCESS;
}
