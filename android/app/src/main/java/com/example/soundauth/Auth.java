package com.example.soundauth;

import android.content.SharedPreferences;
import android.util.Log;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Set;

public class Auth {
    public static final int KEY_LENGTH = 16;
    private DeviceInfo dev;

    public Auth(DeviceInfo device) {
        dev = device;
    }

    private byte[] getSecretKey() {
        return dev.secret;
    }

    public byte[] respond(byte[] challenge) {
        try {
            var key = getSecretKey();
            Log.d("Auth", "Key: " + ListenService.bytesToHex(key));
            byte[] msg = new byte[challenge.length + key.length];
            System.arraycopy(challenge, 0, msg, 0, challenge.length);
            System.arraycopy(key, 0, msg, challenge.length, key.length);

            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            return digest.digest(msg);
        } catch (NoSuchAlgorithmException ignored) {
            return null;
        }
    }
}
