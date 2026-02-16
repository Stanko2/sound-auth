package com.example.soundauth.ui

import android.Manifest
import android.content.pm.PackageManager
import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.AudioTrack
import android.media.MediaRecorder
import android.util.Log
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

private const val TAG = "SoundTestingScreen"

class SoundTestingScreen() {
    val sampleRate = 48000
    val channelConfigIn = AudioFormat.CHANNEL_IN_MONO
    val channelConfigOut = AudioFormat.CHANNEL_OUT_MONO
    val bufferSize = 2048 * 4 //AudioRecord.getMinBufferSize(sampleRate, channelConfig, audioFormat)
    val audioFormat: AudioFormat = AudioFormat.Builder()
        .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
        .setSampleRate(sampleRate)
        .setChannelMask(channelConfigIn)
        .build()

    val audioFormatOut: AudioFormat = AudioFormat.Builder()
        .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
        .setSampleRate(sampleRate)
        .setChannelMask(channelConfigOut)
        .build()

    val audioAttributes: AudioAttributes = AudioAttributes.Builder()
        .setUsage(AudioAttributes.USAGE_MEDIA)
        .setContentType(AudioAttributes.CONTENT_TYPE_UNKNOWN)
        .build()

    val logger = Logger()
    
    @Composable
    fun UI() {

        val context = LocalContext.current

        val coroutineScope = rememberCoroutineScope()

        var hasRecordAudioPermission by remember {
            mutableStateOf(
                ContextCompat.checkSelfPermission(
                    context,
                    Manifest.permission.RECORD_AUDIO
                ) == PackageManager.PERMISSION_GRANTED
            )
        }

        val permissionLauncher = rememberLauncherForActivityResult(
            ActivityResultContracts.RequestPermission()
        ) { isGranted: Boolean ->
            hasRecordAudioPermission = isGranted
        }

        var isRecording by remember { mutableStateOf(false) }
        var isPlaying by remember { mutableStateOf(false) }
        var audioData by remember { mutableStateOf<FloatArray?>(null) }
        var audioRecord by remember { mutableStateOf<AudioRecord?>(null) }
        var audioTrack by remember { mutableStateOf<AudioTrack?>(null) }
        var recordingJob by remember { mutableStateOf<Job?>(null) }
        var playbackJob by remember { mutableStateOf<Job?>(null) }
        var playInput by remember { mutableStateOf<String>("17000|15000|0|17000") }

        fun startRecording() {
            if (!hasRecordAudioPermission) {
                permissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
                return
            }

            isRecording = true

            recordingJob = coroutineScope.launch(Dispatchers.IO) {
                val record = AudioRecord.Builder()
                    .setAudioSource(MediaRecorder.AudioSource.UNPROCESSED)
                    .setAudioFormat(audioFormat)
                    .setBufferSizeInBytes(bufferSize)
                    .build()

                withContext(Dispatchers.Main) { audioRecord = record }

                val buffer = FloatArray(bufferSize / 4)
                val recordedData = mutableListOf<Float>()
                record.startRecording()

                while (isRecording) {
                    val readSize = record.read(buffer, 0, buffer.size, AudioRecord.READ_BLOCKING)
                    if (readSize > 0) {
                        val freq = RunFFT(buffer)
                        if (freq.isNotEmpty()) {
                            logger.log("frame: ${freq.contentToString()}")
                        }
                    }
                }

                record.stop()
                record.release()

                withContext(Dispatchers.Main) {
                    audioData = recordedData.toFloatArray()
                    audioRecord = null
                }
            }
        }

        fun stopRecording() {
            isRecording = false
        }

        fun startPlaying() {
            isPlaying = true
            playbackJob = coroutineScope.launch(Dispatchers.IO) {
                val track = AudioTrack.Builder()
                    .setAudioAttributes(audioAttributes)
                    .setAudioFormat(audioFormatOut)
                    .setBufferSizeInBytes(bufferSize)
                    .build()
                withContext(Dispatchers.Main) { audioTrack = track }

                track.play()

                var offset = 0
                var data = GenerateFrequencies(playInput)

                if (data.size < sampleRate) {
                    val padded = FloatArray(sampleRate)
                    System.arraycopy(data, 0, padded, 0, data.size)
                    data = padded
                }

                Log.d(TAG, "startPlaying: $playInput")
                while (isPlaying) {
                    val writeSize = minOf(data.size - offset, bufferSize / 4)
                    track.write(data, offset, writeSize, AudioTrack.WRITE_BLOCKING)
                    offset += writeSize
                    if (offset >= data.size) {
                        isPlaying = false
                        offset = 0
//                        data = GenerateFrequencies(playInput)
                    }
                }

//                delay((data.size / sampleRate * 5000).toLong())
                if (track.playState == AudioTrack.PLAYSTATE_PLAYING) {
                    track.stop()
                }
                track.release()

                withContext(Dispatchers.Main) {
                    isPlaying = false
                    audioTrack = null
                }
            }
        }

        fun stopPlaying() {
            isPlaying = false // This will cause the playback loop to terminate
        }


        DisposableEffect(Unit) {
            onDispose {
                isRecording = false
                isPlaying = false
                audioRecord?.release()
                audioTrack?.release()
                recordingJob?.cancel()
                playbackJob?.cancel()
            }
        }

        Column(
            modifier = Modifier.fillMaxSize(),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            FrequencyGenerator({
                playInput = it
            }).UI()

            Row {
                Button(onClick = {
                    if (isRecording) {
                        stopRecording()
                    } else {
                        startRecording()
                    }
                }) {
                    Text(if (isRecording) "Stop Recording" else "Start Recording")
                }
                Spacer(modifier = Modifier.width(16.dp))
                Button(onClick = {
                    if (isPlaying) {
                        stopPlaying()
                    } else {
                        startPlaying()
                    }
                }, enabled = playInput.isNotEmpty()) {
                    Text(if (isPlaying) "Stop Playback" else "Start Playback")
                }
            }

            logger.UI()
        }
    }

    external fun RunFFT(input: FloatArray): FloatArray

    external fun GenerateFrequencies(input: String): FloatArray
}
