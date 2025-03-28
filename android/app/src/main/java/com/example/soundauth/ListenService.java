package com.example.soundauth;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.IBinder;
import android.util.Log;

import androidx.core.app.NotificationCompat;
import androidx.core.app.ServiceCompat;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

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
        promoteToForeground();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        initGGwave(sample_rate, bufferSize);
        AudioManager manager = getSystemService(AudioManager.class);

        receiver = new MessageReceiver((int)sample_rate, manager);
        sender = new MessageSender((int) sample_rate, manager);
        receiver.setSender(sender);
        receiver.setMsgHandler((msg)->{
            var i = new Intent("message");
            i.putExtra("data", msg.data);
            i.putExtra("command", msg.command);
            i.putExtra("address", msg.address);
            LocalBroadcastManager.getInstance(this).sendBroadcast(i);
        });

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
    public void onDestroy() {
        receiver.stop();
        sender.stop();
        Log.d(TAG, "stopService: Stopped");
        super.onDestroy();
    }

    private void promoteToForeground() {
        NotificationChannel channel = new NotificationChannel("sound-auth", "Sound-auth service", NotificationManager.IMPORTANCE_MIN);
        var manager = (NotificationManager)getSystemService(Context.NOTIFICATION_SERVICE);
        manager.createNotificationChannel(channel);

        var notification = new NotificationCompat.Builder(this, "sound-auth").setContentTitle("").setContentText("").build();
        startForeground(1, notification);
    }

    private native boolean initGGwave(float sample_rate, int bufferSize);
}