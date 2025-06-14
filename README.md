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

In order to build the PAM module you need to have following packages installed (On Ubuntu): `libpam0g-dev`, `libsdl2-dev`, `cmake`, `build-essential`, `pkg-config`, `libconfig++-dev`.
Before proceeding, make sure you have all the necessary packages installed.

Build the PAM module using the following commands:

```bash
cd linux/
cmake .
make
```

This should build 2 files - `pam_sound_auth.so` and `sound_auth` binary. The `sound_auth` binary is used for setup and testing purposes.


#### Configuration

1. In order to use PAM module you need to copy `pam_sound_auth.so` to the PAM modules directory (for example: `/lib/security/`).

2. To set up the PAM module, you need to add the following line to the top of the PAM configuration file of the service you want to apply the module to:

```
auth    sufficient      pam_sound_auth.so
```

3. You also need to copy the `sound-auth.cfg` file to `/etc` and make it readable by only root. It will contain secret keys for all users.

##### Configuration file

- `protocol` - which protocol to use when transferring messages - one of `ultrasound_fastest`, `ultrasound_fast`, `ultrasound_normal`, `audible_normal`, `audible_fast`, `audible_fastest`. Some speakers could not handle the ultrasound frequencies. And higher speeds tend to have more errors in transmission.
- `devices` - which device to use for capture or playback (sending and receiving messages). All connected devices could be queried by `./sound-auth list`. Leaving to `auto` chooses the device automatically.
- the rest of the file is auto-generated - contains `address` of this computer and user credentials (keys).

## Protocol

Every device has some sort of "address", which is 2 bytes long. Messages received with wrong address wont be processed. An address of `0` means broadcast. After destination address we need to specify the type of the message, which would be 1 byte. So the message format is `[destination][source][type][data]`

### Setup

Setup message is used to transfer the address of a computer to the phone. It can be intercepted, so you must ensure that no one listens to it while the transfer is happening. The data format is: `[user]@[computer address][device name]:[secret]`. The type of this message is `0x01`
- The `[user]` is the unix user of the computer for whom the
- `[computer address]` is the address of the computer that the phone should send the data to
- `[device name]` - user friendly name of the device (hostname)
- `[secret]` - secret key that is used to generate one time passwords

Phone should answer to this message with its address if he saved the secret successfully

### Authentication

At first computer sends message to phone with a message containing random bytes. The message address needs to match the address of a phone. The message type must be `0x02` and both source and destination address must be set (cannot be broadcast).
The phone then computes the hash of `[key] + [msg]` and sends that value back to the computer. Computer then calculates the same and checks if hashes match.
