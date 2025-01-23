package com.example.soundauth;

import android.content.Intent;
import android.media.AudioFocusRequest;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Process;
import android.util.Log;

import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import java.util.LinkedList;
import java.util.List;
import java.util.Queue;

import java.util.Arrays;
import java.util.stream.Stream;

public class MessageReceiver implements Runnable, AudioManager.OnAudioFocusChangeListener {
    public static String TAG = "MessageReceiver";
    private Queue<byte[]> messages;
    private AudioRecord audioRecord;
    private short[] audioBuffer;
    private final int sampleRate;
    private boolean isRunning = true;
    private static MessageReceiver instance;
    private AudioManager audioManager;

    public static MessageReceiver getInstance() {
        return instance;
    }

    public boolean isPlaying;

    public void start(){
        isRunning = true;
        audioRecord.startRecording();
        listenLoop();
    }

    public void stop(){
        isRunning = false;
        audioRecord.stop();
        audioRecord.release();
        instance = null;
    }

    private native byte[] decode(short[] audioData) throws SoundProcessException;

    public MessageReceiver(int sampleRate, AudioManager manager) {
        audioManager = manager;
        this.sampleRate = sampleRate;
        messages = new LinkedList<>();
        if (instance != null) {
            throw new RuntimeException("Multiple Receivers not allowed");
        }
        instance = this;
    }

    @Override
    public void run() throws SecurityException {
        Log.d(TAG, "Starting MessageReceiver");
        android.os.Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO);
        var bufferSize = 8192; //AudioRecord.getMinBufferSize((int)sample_rate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);

        initializeRecording(bufferSize);
        listenLoop();
    }

    private void initializeRecording(int bufferSize) throws SecurityException {
        audioManager.requestAudioFocus(new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN).setOnAudioFocusChangeListener(this).build());
        var audio = new AudioRecord(MediaRecorder.AudioSource.MIC,
                sampleRate,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize);

        Log.d("ListenService", "buffer size = " + bufferSize);
        Log.d("ListenService", "Sample rate = " + audio.getSampleRate());


        audioBuffer = new short[bufferSize];
        audio.startRecording();
        audioRecord = audio;
    }

    private void listenLoop() {

        while(isRunning) {
            int len = audioRecord.read(audioBuffer, 0, audioBuffer.length);
            int max = 0;
            for (int i = 0; i < len; i++) {
                max = Math.max(max, audioBuffer[i]);
            }
            if (max == 0) {
                Log.w(TAG, "listenLoop: no data received");
                initializeRecording(audioBuffer.length);
            }
            if (len == 0) continue;
            if (isPlaying) continue;
            try {
                byte[] data = decode(audioBuffer);
                if (data != null) {
                    Log.d(TAG, "MessageReceived: " + new String(data));
                    messages.add(data);

                }
            } catch (SoundProcessException e) {
                messages.add(null);
            }
        }
    }

    @Override
    public void onAudioFocusChange(int focusChange) {
        Log.d(TAG, "onAudioFocusChange: " + focusChange);
    }
}
