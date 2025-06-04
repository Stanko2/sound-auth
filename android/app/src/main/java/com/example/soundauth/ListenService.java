package com.example.soundauth;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.IBinder;
import android.preference.Preference;
import android.preference.PreferenceManager;
import android.util.Log;

import androidx.core.app.NotificationCompat;
import androidx.core.app.ServiceCompat;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Objects;
import java.util.Random;
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

    private static final char[] HEX_ARRAY = "0123456789ABCDEF".toCharArray();
    public static String bytesToHex(byte[] bytes) {
        char[] hexChars = new char[bytes.length * 2];
        for (int j = 0; j < bytes.length; j++) {
            int v = bytes[j] & 0xFF;
            hexChars[j * 2] = HEX_ARRAY[v >>> 4];
            hexChars[j * 2 + 1] = HEX_ARRAY[v & 0x0F];
        }
        return new String(hexChars);
    }

    public static byte[] hexToByteArray(String s) {
        int len = s.length();
        assert len % 2 == 0;
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                    + Character.digit(s.charAt(i+1), 16));
        }
        return data;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        promoteToForeground();
    }

    private void processMessage(MessageHandler.Message msg) {
        SharedPreferences x = getSharedPreferences("prefs", MODE_PRIVATE);
        PreferencesManager p = new PreferencesManager(x);
        Log.d(TAG, "processMessage: " + new String(msg.data));
        var intent = new Intent();
        var addr = getAddress();
        switch (msg.command) {
            case 0x01:
                intent.setAction("device_add");
                intent.putExtra("device", msg.data);
                intent.putExtra("id", msg.source);
                sender.enqueueMessage(new byte[]{}, (byte)0x01, msg.source);
                break;
            case 0x02:
                if (msg.address[0] != addr[0] || msg.address[1] != addr[1])
                    break;
                DeviceInfo dev = null;
                for(var d : p.getDevices()){
                    if (d.id[0] == msg.source[0] && d.id[1] == msg.source[1]){
                        dev = d;
                    }
                }
                Log.d(TAG, "Received login challenge");
                if (dev == null) {
                    intent.setAction("error");
                    intent.putExtra("message", "Received login from unknown device");
                    break;
                }
                Auth auth = new Auth(dev);
                Log.d(TAG, "Chall: " + bytesToHex(msg.data));
                var res = auth.respond(msg.data);
                sender.enqueueMessage(res, (byte)0x02, dev.id);
                break;
            default:
                intent.setAction("error");
                intent.putExtra("message", "Unknown Command: " + msg.command);
        }
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    private byte[] getAddress() {
        SharedPreferences p = getSharedPreferences("prefs", MODE_PRIVATE);
        Random r = new Random();
        int defaultAddress = r.nextInt( 1 << 16 - 1) + 1;
        int addr = p.getInt("address", defaultAddress);
        if (addr == defaultAddress) {
            p.edit().putInt("address", defaultAddress).apply();
        }
        return new byte[] {
            (byte)(addr >>> 8),
            (byte)addr
        };
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        initGGwave(sample_rate, bufferSize);
        AudioManager manager = getSystemService(AudioManager.class);
        var address = getAddress();
        receiver = new MessageReceiver((int)sample_rate, manager, address);
        sender = new MessageSender((int) sample_rate, manager, address);
        receiver.setSender(sender);
        receiver.setMsgHandler((msg)->{
            processMessage(msg);
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
            sender.enqueueMessage(Objects.requireNonNull(intent.getStringExtra("message")).getBytes(), (byte)0x00, new byte[] {0x00,0x00});
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