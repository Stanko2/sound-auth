package com.example.soundauth;

import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.*;

import com.example.soundauth.databinding.FragmentDeviceInfoBinding;

public class deviceList extends Fragment {
    private static final String TAG = "DeviceList";
    private FragmentDeviceInfoBinding binding = null;
    private PreferencesManager p;


    public deviceList() {}





    private void removeDevice(DeviceInfo d) {
        var set = p.getDevices();
        set.remove(d);
        p.saveDevices(set);
        updateUI();
    }

    public void addDevice(DeviceInfo d) {
        var set = p.getDevices();
        set.add(d);
        p.saveDevices(set);
        updateUI();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    private void updateUI() {
        var devices = p.getDevices();
        var ctx = requireContext();
        LinearLayout l = binding.list;
        var p = new ViewGroup.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, WRAP_CONTENT);
        l.removeAllViews();
        if (devices.isEmpty()) {
            var tv = new TextView(ctx);
            tv.setLayoutParams(p);
            tv.setText("No devices");
            l.addView(tv);
            return;
        }
        for(var d : devices) {
            var v = new LinearLayout(ctx);
            v.setOrientation(LinearLayout.HORIZONTAL);
            v.setPadding(0,0,8,8);
            v.setGravity(Gravity.CENTER_VERTICAL);

            var tv = new TextView(ctx);
            tv.setText(d.name);
            tv.setLayoutParams(p);

            var btn = new Button(ctx);
            btn.setText("forget");
            btn.setOnClickListener((e)->{
                removeDevice(d);
            });

            tv.setLayoutParams(new LinearLayout.LayoutParams(0, WRAP_CONTENT, 1));
            btn.setLayoutParams(new LinearLayout.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));


            v.addView(tv);
            v.addView(btn);


            l.addView(v);
        }
        Log.d(TAG, "updateUI: updated with " + l.getChildCount() + " devices.");
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        binding = FragmentDeviceInfoBinding.inflate(inflater, container, false);
        SharedPreferences x = requireActivity().getSharedPreferences("prefs", Context.MODE_PRIVATE);
        p = new PreferencesManager(x);

        return binding.getRoot();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        updateUI();
        LocalBroadcastManager.getInstance(requireContext()).registerReceiver(addBroadcastReceiver, new IntentFilter("device_add"));
    }

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);
    }

    @Override
    public void onDetach() {
        LocalBroadcastManager.getInstance(requireContext()).unregisterReceiver(addBroadcastReceiver);
        super.onDetach();
    }

    private final BroadcastReceiver addBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            var data = intent.getByteArrayExtra("device");
            var address = intent.getByteArrayExtra("id");
            assert data != null;
            assert address != null;
            var dev = new DeviceInfo(data, address);
            Log.d(TAG, "newDevice: " + dev);

            addDevice(dev);
        }
    };
}