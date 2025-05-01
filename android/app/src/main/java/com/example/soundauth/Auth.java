package com.example.soundauth;

import android.content.SharedPreferences;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Set;

public class Auth {
    private Set<DeviceInfo> devices;

    public Auth(Set<DeviceInfo> devices) {
        this.devices = devices;
    }

    private byte[] getSecretKey() {
        return "12345678".getBytes(StandardCharsets.UTF_8);
    }

    public byte[] respond(byte[] challenge) {
        try {
            var key = getSecretKey();
            byte[] msg = new byte[challenge.length + key.length];
            System.arraycopy(challenge, 0, msg, 0, challenge.length);
            System.arraycopy(key, 0, msg, challenge.length, key.length);

            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            return digest.digest(msg);
        } catch (NoSuchAlgorithmException ignored) {
            return null;
        }
    }

    public boolean processMessage(MessageHandler.Message msg) {
        for(var d : devices) {
        }

        return false;
    }
}
