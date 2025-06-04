package com.example.soundauth;

import android.util.Log;

import androidx.annotation.Nullable;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Arrays;
import java.util.Base64;
import java.util.Objects;
import java.util.Stack;

public class DeviceInfo {
    public final byte[] id;
    public final byte[] secret;
    public final String name;

    public DeviceInfo(JSONObject json) throws JSONException {
        this.id = ListenService.hexToByteArray(json.getString("id"));
        this.secret = ListenService.hexToByteArray(json.getString("secret"));
        this.name = json.getString("name");
    }

    public DeviceInfo(byte[] data, byte[] address) {
//        Log.d("", "DeviceInfo: " + new String(data));
        this.id = new byte[] {address[0], address[1]};
        var s = new String(data);
        this.name = s.split(":")[0];
        var idx = s.indexOf(':') + 1;
        secret = new byte[Auth.KEY_LENGTH];
        System.arraycopy(data, idx, secret, 0, Auth.KEY_LENGTH);
        Log.d("", "New device:" + this);
    }

    public String json() {
        var json = new JSONObject();
        try {
            json.put("id", ListenService.bytesToHex(id));
            json.put("name", name);
            json.put("secret", ListenService.bytesToHex(secret));
        } catch (JSONException ignored) {

        }
        return json.toString();
    }

    @Override
    public String toString() {
        return "DeviceInfo{" +
                "id=" + ListenService.bytesToHex(id) +
                ", secret=" + ListenService.bytesToHex(secret) +
                ", name='" + name + '\'' +
                '}';
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
