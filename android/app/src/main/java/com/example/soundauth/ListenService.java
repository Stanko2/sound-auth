package com.example.soundauth;

import android.app.Service;
import android.content.Intent;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.IBinder;

public class ListenService extends Service {
    private AudioRecord record;
    private int sample_rate = 44100;
    private int bufferSize = 10000;
    public ListenService() throws SecurityException {
        initGGwave(sample_rate, bufferSize);
        record = new AudioRecord(
            MediaRecorder.AudioSource.MIC,
            sample_rate,
            AudioFormat.CHANNEL_IN_MONO,
            AudioFormat.ENCODING_PCM_16BIT,
            bufferSize
        );
    }

    @Override
    public IBinder onBind(Intent intent) {
        throw new UnsupportedOperationException("Not yet implemented");
    }

    public native boolean initGGwave(int sample_rate, int bufferSize);
}