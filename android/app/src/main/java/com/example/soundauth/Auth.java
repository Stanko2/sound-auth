package com.example.soundauth;

import android.content.SharedPreferences;
import android.util.Log;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Set;

public class Auth {
    private static final String TAG = "Auth";
    public static final int KEY_LENGTH = 32;
    public static final int CHALLENGE_LENGTH = 16;
    private DeviceInfo dev;
    private byte[] publicKey;

    public byte[] getPublicKey() {
        return publicKey;
    }

    private MessageSender sender;

    public void setDevice(DeviceInfo dev) {
        this.dev = dev;
    }

    public Auth(MessageSender sender) {
        this.sender = sender;
    }

    private byte[] getSecretKey() {
        return dev.secret;
    }

    public DeviceInfo handlePairRequest(MessageHandler.Message msg) {
        dev = new DeviceInfo(msg.source);
        publicKey = generateKey();
        Log.d(TAG,  "address: " + ListenService.bytesToHex(msg.source) + " Key: " + ListenService.bytesToHex(publicKey));
        dev.secret = getSecret(dev.publicKey);
        Log.d(TAG, "handlePairRequest: got secret: " + ListenService.bytesToHex(dev.secret));
        return dev;
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

    private native byte[] generateKey();

    private native byte[] getSecret(byte[] otherPubKey);
}
