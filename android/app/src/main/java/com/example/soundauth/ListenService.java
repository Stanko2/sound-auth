package com.example.soundauth;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaPlayer;
import android.media.MediaRecorder;
import android.net.Uri;
import android.os.IBinder;
import android.os.Process;
import android.provider.MediaStore;
import android.util.Log;

import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URI;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.Objects;

public class ListenService extends Service {
    private static final String TAG = "ListenService";

    // Used to load the 'soundauth' library on application startup.
    static {
        System.loadLibrary("soundAuth");
    }

    private final float sample_rate = 48000;
    private int bufferSize = 500000;
    private boolean listening = true;
    private Context context;
    private AudioTrack output;

    @Override
    public void onCreate() {
        super.onCreate();
        context = this;
    }

    public ListenService() throws SecurityException {

//        record = new AudioRecord(
//            MediaRecorder.AudioSource.MIC,
//            sample_rate,
//            AudioFormat.CHANNEL_IN_MONO,
//            AudioFormat.ENCODING_PCM_16BIT,
//            bufferSize
//        );
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        initGGwave(sample_rate, bufferSize);
        if (intent == null) {
            Log.e(TAG, "onStartCommand: Intent null");
            return Service.START_STICKY;
        }
        
        if(Objects.equals(intent.getStringExtra("mode"), "Listen")){
            new Thread(this::listen).start();
        } else {
            init_play(intent);
        }

        return Service.START_STICKY;
    }


    private void init_play(Intent intent) {
        AudioManager manager = getSystemService(AudioManager.class);
        manager.setMode(AudioManager.MODE_RINGTONE);
        manager.setSpeakerphoneOn(true);
        int outChannel = AudioFormat.CHANNEL_OUT_MONO;
        bufferSize = AudioTrack.getMinBufferSize((int) sample_rate, outChannel, AudioFormat.ENCODING_PCM_16BIT);
        bufferSize *= 2;
        Log.d(TAG, "onStartCommand: bufferSize " + bufferSize);
        output = new AudioTrack.Builder().setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_MEDIA).setContentType(AudioAttributes.CONTENT_TYPE_MOVIE).build())
                .setAudioFormat(new AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate((int) sample_rate).setChannelMask(outChannel).build()).setBufferSizeInBytes(bufferSize)
                .setTransferMode(AudioTrack.MODE_STREAM).build();
        Log.d(TAG, "Sample rate: " + output.getSampleRate());
        String message = intent.getStringExtra("message");
        assert message != null;

        playMessage(message.getBytes(StandardCharsets.UTF_8));
    }

    public void playMessage(byte[] message) {
        byte[] audio = encode(message);
        int max = 0;
        for (byte b : audio) {
            max = Math.max(max, b);
        }
        Log.d(TAG, "playMessage: generated " + audio.length + " max: " + max);


        new Thread(() -> {
            try {
                int offset = 0;
                output.play();
                while (offset < audio.length) {
                    int remaining = audio.length - offset;
                    int toWrite = Math.min(bufferSize, remaining);
                    int written = output.write(audio, offset, toWrite, AudioTrack.WRITE_BLOCKING);
                    if (written < 0) {
                        Log.e(TAG, "playMessage: Error writing " + written);
                    }
                    offset += toWrite;
                }
            } catch (Exception e) {
                Log.e(TAG, "playMessage: ", e);
            }
        }).start();
    }

    public void listen() throws SecurityException {
        Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO);
        var bufferSize = 8192; //AudioRecord.getMinBufferSize((int)sample_rate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);

        var audio = new AudioRecord(MediaRecorder.AudioSource.DEFAULT,
                (int)sample_rate,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize);

        Log.d("ListenService", "buffer size = " + bufferSize);
        Log.d("ListenService", "Sample rate = " + audio.getSampleRate());


        short[] audioData = new short[bufferSize];
        audio.startRecording();
        int received = 0;
        while(listening) {
            int len = audio.read(audioData, 0, bufferSize);
            byte[] data = decode(audioData);
            received += len;
            if (data != null) {
                Log.d("Message", new String(data));
                Intent intent = new Intent("Message received");
                intent.putExtra("message", new String(data));
                LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
            }
        }

        audio.stop();
        audio.release();
        Log.v(TAG, String.format("Capturing stopped. Samples read: %d", received));
    }

    @Override
    public IBinder onBind(Intent intent) {
        throw new UnsupportedOperationException("Not yet implemented");
    }



    private native boolean initGGwave(float sample_rate, int bufferSize);

    private native byte[] encode(byte[] message) throws SoundProcessException;

    private native byte[] decode(short[] audioData) throws SoundProcessException;
}