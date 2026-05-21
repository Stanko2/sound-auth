package com.example.soundauth;

import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Arrays;
import java.util.Base64;
import java.util.Objects;
import java.util.Stack;

public class DeviceInfo {
    public final byte[] id;
    public byte[] publicKey;
    public byte[] secret;
    public final String name;

    public DeviceInfo(JSONObject json) throws JSONException {
        this.id = ListenService.hexToByteArray(json.getString("id"));
        this.secret = ListenService.hexToByteArray(json.getString("secret"));
        this.name = json.getString("name");
    }

    public DeviceInfo(MessageHandler.Message msg) throws MessageHandler.InvalidCrcException {
//        Log.d("", "DeviceInfo: " + new String(data));
        this.id = new byte[] {msg.source[0], msg.source[1]};

        StringBuilder s = new StringBuilder();
        byte x = 0;
        while (x != ':') {
            x = msg.recv(1)[0];
            s.append((char) x);
        }
        this.name = s.toString();
        this.publicKey = msg.recv(Auth.KEY_LENGTH);
        Log.d("", "New device:" + this);
        msg.end();
    }

    public String json() {
        var json = new JSONObject();
        try {
            json.put("id", ListenService.bytesToHex(id));
            json.put("name", name);
            if(secret != null){
                json.put("secret", ListenService.bytesToHex(secret));
            }
            if (publicKey != null) {
                json.put("publicKey", ListenService.bytesToHex(publicKey));
            }
        } catch (JSONException ignored) {

        }
        return json.toString();
    }

    @NonNull
    @Override
    public String toString() {
        var r = "DeviceInfo{" +
            "id=" + ListenService.bytesToHex(id) +
            ", name='" + name + '\'';
        if (secret != null) {
            r+=", secret=" + ListenService.bytesToHex(secret);
        }
        if (publicKey != null) {
            r+=", publicKey=" + ListenService.bytesToHex(publicKey);
        }
        r+='}';
        return r;
    }

    @Override
    public boolean equals(@Nullable Object obj) {
        if (!(obj instanceof DeviceInfo))
            return false;

        return Arrays.equals(((DeviceInfo) obj).id, id);
    }

    @Override
    public int hashCode() {
        return Objects.hash(Arrays.hashCode(id), Arrays.hashCode(secret), name);
    }
}
