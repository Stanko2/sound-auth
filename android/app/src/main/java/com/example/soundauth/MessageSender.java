package com.example.soundauth;

import static androidx.core.content.ContextCompat.getSystemService;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

import java.util.LinkedList;
import java.util.Queue;

public class MessageSender implements Runnable {
    public static String TAG = "MessageSender";
    private AudioTrack output;
    private AudioManager manager;
    private MessageReceiver receiver;
    private int bufferSize;
    private boolean isRunning = true;

    private Queue<byte[]> messagesToSend;

    public synchronized void enqueueMessage(byte[] message) {
        messagesToSend.add(message);
    }



    public void start(){
        isRunning = true;
        mainLoop();
    }

    public void stop(){
        isRunning = false;
    }

    public MessageSender(int sampleRate, AudioManager manager) {
        this.manager = manager;
        receiver = MessageReceiver.getInstance();
        manager.setMode(AudioManager.MODE_RINGTONE);
        manager.setSpeakerphoneOn(true);
        int outChannel = AudioFormat.CHANNEL_OUT_MONO;
        bufferSize = AudioTrack.getMinBufferSize((int) sampleRate, outChannel, AudioFormat.ENCODING_PCM_16BIT);
        bufferSize *= 2;
        Log.d(TAG, "onStartCommand: bufferSize " + bufferSize);
        output = new AudioTrack.Builder().setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_ALARM).setContentType(AudioAttributes.CONTENT_TYPE_SPEECH).build())
                .setAudioFormat(new AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate(sampleRate).setChannelMask(outChannel).build()).setBufferSizeInBytes(bufferSize)
                .setTransferMode(AudioTrack.MODE_STREAM).build();
        messagesToSend = new LinkedList<>();
    }

    @Override
    public void run() {
        mainLoop();
    }

    private void mainLoop() {
        while(isRunning) {
            if (messagesToSend.isEmpty()) continue;
            byte[] message = messagesToSend.remove();
            try {
                sendMessage(message);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }
    }

    private void sendMessage(byte[] message) throws InterruptedException {
        byte[] audio = encode(message);
        int max = 0;
        for (byte b : audio) {
            max = Math.max(max, b);
        }
        Log.d(TAG, "playMessage: generated " + audio.length + " max: " + max);

        int offset = 0;
        output.play();
        receiver.isPlaying = true;
        while (offset < audio.length) {
            int remaining = audio.length - offset;
            int toWrite = Math.min(bufferSize, remaining);
            int written = output.write(audio, offset, toWrite, AudioTrack.WRITE_BLOCKING);
            if (written < 0) {
                Log.e(TAG, "playMessage: Error writing " + written);
            }
            offset += toWrite;
        }
        Thread.sleep(30);
        receiver.isPlaying = false;
    }


    private native byte[] encode(byte[] message) throws SoundProcessException;

}
