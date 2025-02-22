package com.example.soundauth;

import androidx.appcompat.app.AppCompatActivity;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import com.example.soundauth.databinding.ActivityMainBinding;

import java.util.List;

public class MainActivity extends AppCompatActivity {


    private ActivityMainBinding binding;

    @SuppressLint("NewApi")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
//        tv.setText(stringFromJNI());
        binding.send.setOnClickListener((e)-> {
            Intent i = new Intent(this, ListenService.class);
            i.putExtra("message", binding.messageField.getText().toString());
            startForegroundService(i);
        });

        binding.receive.setOnClickListener((e)-> {
            Intent i = new Intent(this, ListenService.class);
            startForegroundService(i);
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
            binding.outputText.setText(intent.getStringExtra("message"));
        }
    };
}