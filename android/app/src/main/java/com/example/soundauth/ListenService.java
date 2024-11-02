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
import android.provider.MediaStore;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URI;
import java.nio.charset.StandardCharsets;

public class ListenService extends Service {
    private static String TAG = "ListenService";

    // Used to load the 'soundauth' library on application startup.
    static {
        System.loadLibrary("soundAuth");
    }

    private AudioRecord record;
    private final float sample_rate = 48000;
    private int bufferSize = 500000;
    private Context context;
    private AudioTrack output;
    private int outChannel = AudioFormat.CHANNEL_OUT_MONO;

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
        bufferSize = AudioTrack.getMinBufferSize((int)sample_rate, outChannel, AudioFormat.ENCODING_PCM_8BIT);
        bufferSize *= 2;
        Log.d(TAG, "onStartCommand: bufferSize " + bufferSize);
        output = new AudioTrack.Builder().setAudioAttributes(
                new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_MEDIA)
                        .setContentType(AudioAttributes.CONTENT_TYPE_MOVIE).build()
            ).setAudioFormat(new AudioFormat.Builder().
                setEncoding(AudioFormat.ENCODING_PCM_8BIT)
                        .setSampleRate((int)sample_rate)
                        .setChannelMask(outChannel).build())
                .setBufferSizeInBytes(bufferSize).setTransferMode(AudioTrack.MODE_STREAM).build();
        Log.d(TAG, "Sample rate: " + output.getSampleRate());
        String message = intent.getStringExtra("message");
        assert message != null;
        playMessage(message.getBytes(StandardCharsets.UTF_8));

        try {
            output.play();
            output.pause();
        } catch (Exception e) {
            Log.e(TAG, "AudioTrack play/pause test failed: " + e.getMessage());
        }


        return Service.START_STICKY;
    }

    public void playMessage(byte[] message) {
        byte[] audio = encode(message, message.length);
        Log.d(TAG, "playMessage: generated " + audio.length);
        new Thread(()->{
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

    @Override
    public IBinder onBind(Intent intent) {
        throw new UnsupportedOperationException("Not yet implemented");
    }

    private native boolean initGGwave(float sample_rate, int bufferSize);

    private native byte[] encode(byte[] message, int messageSize) throws SoundProcessException;

    private native byte[] decode(byte[] audio, int audioSize) throws SoundProcessException;
}