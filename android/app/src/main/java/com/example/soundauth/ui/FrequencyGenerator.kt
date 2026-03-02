package com.example.soundauth.ui

import android.util.Log
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier

private const val TAG = "FrequencyGenerator";

class FrequencyGenerator(val onFreqOutput: (freqs: String)->Unit, val onMsgOutput: (msg: String)->Unit) {

    @Composable
    fun UI(){
        val freqInput = remember { mutableStateOf("17000|15000|0|17000") }
        val msgInput = remember { mutableStateOf("") }
        val isPlaying = remember { mutableStateOf(false) }

        Column {
            Text("Frequency Generator - frequency string")
            TextField(
                value = freqInput.value,
                onValueChange = { value ->
                    freqInput.value = value
                    try {
                        this@FrequencyGenerator.onFreqOutput(freqInput.value)
                    } catch (err: Exception) {
                        Log.d(TAG, "UI: $err")
                    }

                },
                label = {
                    Text("Enter Frequencies")
                },
                modifier = Modifier.fillMaxWidth()
            )

            Text("Frequency generator - data")
            TextField(
                value = msgInput.value,
                onValueChange = {value ->
                    msgInput.value = value
                    this@FrequencyGenerator.onMsgOutput(msgInput.value)
                },
                label = {
                    Text("Enter message")
                }
            )
        }
    }

}