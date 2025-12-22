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

class FrequencyGenerator(val onOutput: (freqs: String)->Unit) {

    @Composable
    fun UI(){
        val freqInput = remember { mutableStateOf("") }
        val isPlaying = remember { mutableStateOf(false) }

        Column {
            Text("Frequency Generator")
            TextField(
                value = freqInput.value,
                onValueChange = { value ->
                    freqInput.value = value
                    try {
                        this@FrequencyGenerator.onOutput(freqInput.value)
                    } catch (err: Exception) {
                        Log.d(TAG, "UI: $err")
                    }

                },
                label = {
                    Text("Enter Frequencies")
                },
                modifier = Modifier.fillMaxWidth()
            )
        }
    }

}