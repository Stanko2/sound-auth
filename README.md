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

After successful installation click the Receive button to start the background authentication service. 

On android the configuration is done via GUI in the application. Tap on the settings icon on top left corner to open the configuration menu. All settings are explained in the configuration file section.

### Linux

In order to build the PAM module you need to have following packages installed (On Ubuntu): `libpam0g-dev`, `cmake`, `build-essential`, `pkg-config`, `fftw3-dev`.
Before proceeding, make sure you have all the necessary packages installed.

Build the PAM module using the following commands:

```bash
cd linux/
cmake .
make
sudo make install
```

This should build 2 files - `pam_sound_auth.so` and `sound-auth` binary. The `sound-auth` binary is used for setup and testing purposes.


#### Configuration

1. To set up the PAM module, you need to add the following line to the top of the PAM configuration file of the service you want to apply the module to:

```
auth    sufficient      pam_sound_auth.so
```

2. You tweak some values in the `/etc/sound-auth.cfg` file. Make sure to have the same configuration on all 
devices.

3. Run `sound-auth setup` to transfer credentials and set up the authentication module.

## Configuration file

- Devices - which device to use for capture or playback (sending and receiving messages). All connected devices could be queried by `sound-auth list`. Leaving to `auto` uses the system default device.
- address - the address of this computer, will be set automatically on the first run.
- Protocol settings
  - **fftSize** - the size of the FFT window (how many samples per one frame)
  - **markerF1, markerF2** - frequencies to use for marker detection (in Hz)
  - **lowestStrength** - the accepted lowest amplitude (in dB). Everything below will be clamped to this value. Typically around $-120$ to $-90$ dB.
  - **strengthThreshold** - threshold for marker detection (in dB).
    - if not detecting any messages, lower this value
    - if detecting false positives, raise this value
  - **chunkSize** - how frames per chunk. Larger messages will be split into multiple chunks and processed individually.
- Modulation settings
  - **modulationType** - the type of modulation to use. Must be one of `simple`, `2tone`, `MFSK`
  - Simple
    - **F1, F2** - frequencies to use for modulation (in Hz)
  - 2tone
    - **startFrequency** - the starting frequency for modulation (in Hz)
    - **spacing** - the spacing between two used frequencies (in bins)
    - **bitsPerFrame** - the number of bits per frame - frequencies played simultaneously
  - MFSK
    - **startFrequency, spacing** - same as 2tone
    - **regionSize** - the size of one region (in number of frequencies) - must be a power of 2
    - **numRegions** - the number of regions to use

## Testing

To test your setup you can run one of 3 tests:
- `sound-auth test tx` - transmits 8 chunks of data (on android there is a button in app)
- `sound-auth test rx` - receives 8 chunks of data and prints out bit accuracy (on android there is a button in app)
- `sound-auth test auth` - tests the whole authentication process (the devices must be paired first)

## Protocol

Every device has some sort of "address", which is 2 bytes long. Messages received with wrong address wont be processed. An address of `0` means broadcast. After destination address we need to specify the type of the message, which would be 1 byte. So the message format is `[destination][source][type][data][crc]`. The CRC 
at the end is optional checksum of the whole message.

### Setup

Setup message broadcasted to transfer the credentials of a computer to the phone. The data format is: `[user]@[device_name]:[public_key]`. The type of this message is `0x01`
- The `[user]` is the unix user of the computer for whom the auth is being set up
- `[device_name]` - user friendly name of the device (hostname)
- `[public_key]` - public key of the computer

Phone should answer to this message with its public key, but only when CRC checksum is correct so that both can
derive a shared secret used for authentication.

### Authentication

At first computer sends message to phone with a message containing random bytes. The message address needs to match the address of a phone. The message type must be `0x02` and both source and destination address must be set (cannot be broadcast).
The phone then computes the hash of `[key] + [msg]` and sends that value back to the computer. Computer then calculates the same and checks if hashes match.
