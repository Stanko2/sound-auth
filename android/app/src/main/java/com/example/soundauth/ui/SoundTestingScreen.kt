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
    val logger = Logger()
    
    @Composable
    fun UI() {

        val context = LocalContext.current

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
        var playInput by remember { mutableStateOf("17000|15000|0|17000") }
        var msg by remember { mutableStateOf("") }

        fun startRecording() {
            if (!hasRecordAudioPermission) {
                permissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
                return
            }

            isRecording = true
            OpenStreams();
        }

        fun stopRecording() {
            isRecording = false
            CloseStreams();
        }

        fun startPlaying() {
            PlayFrequencies(playInput)
        }

        Column(
            modifier = Modifier.fillMaxSize(),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            FrequencyGenerator({
                playInput = it
            }, {
                msg = it
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
                    startPlaying()
                }, enabled = playInput.isNotEmpty()) {
                    Text("Play")
                }
                Spacer(modifier = Modifier.width(16.dp))
                Button(onClick = {
                    sendData(msg.encodeToByteArray())
                }, enabled = msg.isNotEmpty()) {
                    Text("Send Data")
                }
            }

            logger.UI()
        }
    }

    external fun OpenStreams()

    external fun PlayFrequencies(data: String)

    external fun sendData(data: ByteArray)

    external fun CloseStreams()
}
