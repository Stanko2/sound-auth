package com.example.soundauth;

import android.content.SharedPreferences;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashSet;
import java.util.Set;

public class PreferencesManager {
    public static String TAG = "PreferencesManager";
    private SharedPreferences p;

    public PreferencesManager(SharedPreferences p) {
        this.p = p;
    }

    public Set<DeviceInfo> getDevices() {
        var r = p.getStringSet("deviceList", new HashSet<>());
        var ret = new HashSet<DeviceInfo>();
        for (var i: r) {
            Log.d(TAG, "getDevices: " + i);
            try {
                JSONObject j = new JSONObject(i);
                ret.add(new DeviceInfo(j));
            } catch (JSONException ignored) {
            }
        }
        return ret;
    }

    public void saveDevices(Set<DeviceInfo> devices) {
        var s = new HashSet<String>();
        for(var d : devices) {
            s.add(d.json());
        }
        p.edit().putStringSet("deviceList", s).apply();
    }
}
