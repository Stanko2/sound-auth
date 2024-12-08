package com.example.soundauth;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import com.example.soundauth.databinding.ActivityMainBinding;

import java.util.List;

public class MainActivity extends AppCompatActivity {


    private ActivityMainBinding binding;

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
            startService(i);
        });

        binding.receive.setOnClickListener((e)-> {
            Intent i = new Intent(this, ListenService.class);
            i.putExtra("mode", "Listen");
            i.putExtra("message", "");
            startService(i);
        });
    }

}