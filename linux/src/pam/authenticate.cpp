#include <csignal>
#include <cstddef>
#include <cstdint>
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <string>
#include <vector>
#include "../audio/audio-control.h"
#include <iostream>

std::vector<uint8_t> data;
AudioControl* a;

void stop (int signal) {
    std::cout << "Ending" << std::endl;
    a->end_loop();
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    std::cout << "Hello from pam_sm_authenticate" << std::endl;
    std::signal(SIGINT, stop);


    AudioControl* audio = new AudioControl();
    a = audio;
    Communication* comm = new Communication(audio);
    std::vector<uint8_t> message;
    std::string msg = "Hello";
    message.insert(message.end(), msg.begin(), msg.end());

    comm->encode_message(message);
    comm->receive_callback = [comm, audio](){
        std::cout << "Received message" << std::endl;
        int ret = comm->get_data(const_cast<std::vector<uint8_t>&>(data));
        std::cout << "Data size: " << ret << " " << data.size() << std::endl;
        std::cout << "Message: " << std::endl;
        for (auto c : data) {
            std::cout << c;
        }
        audio->end_loop();
        delete audio;
    };
    std::vector<uint8_t> waveform = comm->get_waveform();
    audio->queue_audio(waveform);
    audio->start_loop();
    for(std::size_t i = 0; i < msg.size(); i++) {
        if (msg[i] != data[i]) {
            std::cout << "ACCESS DENIED" << std::endl;
            return PAM_AUTH_ERR;
        }
    }

    std::cout << "ACCESS GRANTED" << std::endl;
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
  return PAM_SUCCESS;
}
