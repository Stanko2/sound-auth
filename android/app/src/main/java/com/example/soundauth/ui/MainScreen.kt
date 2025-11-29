package com.example.soundauth.ui

import android.app.ActivityManager
import android.content.Context
import android.content.Context.ACTIVITY_SERVICE
import android.content.Context.MODE_PRIVATE
import android.content.Intent
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat.startForegroundService
import com.example.soundauth.ListenService


class MainScreen(private val context: Context){
    @Composable
    fun UI(
    ) {

        var serviceRunning by remember { mutableStateOf(isMyServiceRunning(ListenService::class.java)) }

        val statusText = if (serviceRunning) "Service running" else "Service stopped"
        val buttonText = if (serviceRunning) "Stop service" else "Start service"

        Column(
            modifier = Modifier.Companion
                .fillMaxSize()
                .padding(16.dp)
        ) {

            Text("Paired devices", style = MaterialTheme.typography.titleMedium)

            Spacer(Modifier.Companion.height(12.dp))

            // If you still need the Fragment list → keep AndroidView
            Box(
                Modifier.Companion
                    .height(215.dp)
                    .fillMaxWidth()
            ) {
                DeviceListScreen(context.getSharedPreferences("prefs", MODE_PRIVATE))
            }

            Spacer(Modifier.Companion.height(24.dp))

            Button(
                onClick = {
                    serviceRunning = !serviceRunning
                    if (serviceRunning) {
                        context.startForegroundService(Intent(context, ListenService::class.java))
                    } else {
                        context.stopService(Intent(context, ListenService::class.java))
                    }
                },
                modifier = Modifier.Companion.fillMaxWidth()
            ) {
                Text(buttonText)
            }

            Spacer(Modifier.Companion.height(24.dp))

            Text(statusText)
        }
    }

    private fun isMyServiceRunning(serviceClass: Class<*>): Boolean {
        val manager = context.getSystemService(ACTIVITY_SERVICE) as ActivityManager

        return manager.getRunningServices(Int.MAX_VALUE)
            .any { it.service.className == serviceClass.name }
    }
}
