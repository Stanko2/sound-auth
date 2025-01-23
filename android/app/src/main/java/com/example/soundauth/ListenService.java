package com.example.soundauth;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.IBinder;
import android.util.Log;

import java.nio.charset.StandardCharsets;
import java.util.Objects;
import java.util.Timer;

public class ListenService extends Service {
    private static final String TAG = "ListenService";

    // Used to load the 'soundauth' library on application startup.
    static {
        System.loadLibrary("soundAuth");
    }

    private final float sample_rate = 48000;
    private final int bufferSize = 500000;
    private MessageReceiver receiver;
    private MessageSender sender;


    @Override
    public void onCreate() {
        super.onCreate();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        initGGwave(sample_rate, bufferSize);
        AudioManager manager = getSystemService(AudioManager.class);

        receiver = new MessageReceiver((int)sample_rate, manager);
        sender = new MessageSender((int) sample_rate, manager);

        var listenThread = new Thread(receiver);
        listenThread.setDaemon(true);
        listenThread.setName("Sound-Auth receiver thread");
        listenThread.start();

        var sendThread = new Thread(sender);
        sendThread.setDaemon(true);
        sendThread.setName("Sound-Auth send thread");
        sendThread.start();


        if (intent.hasExtra("message")) {
            sender.enqueueMessage(Objects.requireNonNull(intent.getStringExtra("message")).getBytes());
        }
        return Service.START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public boolean stopService(Intent name) {
        receiver.stop();
        sender.stop();
        return super.stopService(name);
    }


    private native boolean initGGwave(float sample_rate, int bufferSize);
}