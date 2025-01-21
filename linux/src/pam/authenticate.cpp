#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <string>
#include <vector>
#include "../audio/audio-control.h"
#include <filesystem>
#include <iostream>
// #include "../audio/communication.h"

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    std::cout << "Hello from pam_sm_authenticate" << std::endl;
    std::filesystem::path cwd = std::filesystem::current_path();
    std::cout << cwd.string() << std::endl;

    AudioControl* audio = new AudioControl();
    Communication* comm = new Communication(audio);
    std::vector<uint8_t> message;
    std::string msg = "Hello";
    message.insert(message.end(), msg.begin(), msg.end());

    comm->encode_message(message);
    std::vector<uint8_t> waveform = comm->get_waveform();
    audio->queueAudio(waveform);
    audio->start_loop();

    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}
