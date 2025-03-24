package com.example.soundauth;

import androidx.appcompat.app.AppCompatActivity;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import android.annotation.SuppressLint;
import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import com.example.soundauth.databinding.ActivityMainBinding;

import java.util.HashSet;
import java.util.List;

public class MainActivity extends AppCompatActivity {
    public static String TAG = "MainActivity";

    private ActivityMainBinding binding;

    @SuppressLint("NewApi")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        var prefs = getPreferences(MODE_PRIVATE);
        var devices = prefs.getStringSet("devices", null);
        if (devices == null) {
            var tv = new TextView(this);
            tv.setText("No devices paired");
            binding.devices.addView(tv);
        }


        binding.receive.setOnClickListener((e)-> {
            if (!isMyServiceRunning(ListenService.class)) {
                Intent i = new Intent(this, ListenService.class);
                startForegroundService(i);
                binding.statusText.setText("Service running");
                binding.receive.setText("Stop service");
            } else {
                stopService(new Intent(this, ListenService.class));
                binding.statusText.setText("Service stopped");
                binding.receive.setText("Start service");
            }
        });

    }


    @Override
    protected void onPause() {
        LocalBroadcastManager.getInstance(this).unregisterReceiver(audioMessageReceiver);
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        LocalBroadcastManager.getInstance(this).registerReceiver(audioMessageReceiver, new IntentFilter("Message"));
    }

    private final BroadcastReceiver audioMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "onReceive: " + intent.getStringExtra("message"));
        }
    };

    private boolean isMyServiceRunning(Class<?> serviceClass) {
        ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (serviceClass.getName().equals(service.service.getClassName())) {
                return true;
            }
        }
        return false;
    }
}