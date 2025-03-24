# Sound authentication platform

Simple program to enable authentication to linux computers using an Android phone. The whole system consists
of two parts - a PAM module for linux and an Android app.

## Setup

### Android

#### Building the app

There are 2 options to build the app:
- open it in Android Studio and build it there
- use the gradle wrapper to build the app from the command line (needs to have Java and Android SDK installed):

  ```bash
  cd android/
  ./gradlew build
  ```
  The `.apk` file will be located in `android/app/build/outputs/apk/release/app-release-unsigned.apk`.


Install the app by copying the `.apk` file to your phone and opening it.
Make sure to allow the app to access the microphone.

After successful installation click the Receive button to start the background service. The app will start listening for
audio messages and retransmitting them back.

### Linux

In order to build the PAM module you need to have following packages installed (On Ubuntu): `libpam0g-dev`, `libsdl2-dev`, `cmake`, `build-essential`, `pkg-config`.
Before proceeding, make sure you have all the necessary packages installed.

Build the PAM module using the following commands:

```bash
cd linux/
cmake .
make
```

This should build 2 files - `pam_sound_auth.so` and `sound_auth` binary. The `sound_auth` binary is used for testing purposes.


#### Configuration

1. In order to use PAM module you need to copy `pam_sound_auth.so` to the PAM modules directory (for example: `/lib/security/`).

2. To set up the PAM module, you need to add the following line to the top of the PAM configuration file of the service you want to apply the module to:

```
auth    sufficient      pam_sound_auth.so
```


### Testing

To test the sound message transmission, run the `sound_auth` binary with some file as an argument. It will send the file as a sound message to the phone.
If your phone has the service running, it should receive the message and play it back. You should see the message in your terminal.

## Protocol

Message Format `[destination address][command][data]`
