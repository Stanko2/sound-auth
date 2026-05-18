#include <security/_pam_types.h>
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <vector>
#include "../audio/audio-control.h"
#include "../config.h"
#include "../util.h"
#include "otp.cpp"
#include <iostream>

#define MAX_RETRIES 3
#define GGWAVE_DISABLE_LOG

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    AudioControl* audio = new AudioControl();
    AuthConfig config = AuthConfig::instance();
    // std::cout << "Using protocol: " << config.getProtocol() << std::endl;
    // a = audio;

    Communication* comm = new Communication(audio, config.getAddress().data());

    bool success = false;
    for (int retries = 0; retries < MAX_RETRIES; retries++) {
        bool r = runAuth(comm);
        if (r) {
            success = true;
            break;
        }
    }

    if (!success) {
        return PAM_MAXTRIES;
    }

    if (success) {
        return PAM_SUCCESS;
    } else return PAM_AUTH_ERR;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
  return PAM_SUCCESS;
}
