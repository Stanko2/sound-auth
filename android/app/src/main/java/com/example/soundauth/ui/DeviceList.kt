package com.example.soundauth.ui

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.SharedPreferences
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.example.soundauth.DeviceInfo
import com.example.soundauth.PreferencesManager


@Composable
fun DeviceListScreen(
    prefs: SharedPreferences,
    modifier: Modifier = Modifier
) {
    val p = remember { PreferencesManager(prefs) }
    var devices by remember { mutableStateOf(p.getDevices().toList()) }

    fun addDevice(d: DeviceInfo) {
        val set = p.getDevices()
        set.add(d)
        p.saveDevices(set)
        devices = set.toList()
    }

    fun removeDevice(d: DeviceInfo) {
        val set = p.getDevices()
        set.remove(d)
        p.saveDevices(set)
        devices = set.toList()
    }

    // Listen for broadcast receiver (device_add)
    val context = LocalContext.current

    DisposableEffect(Unit) {
        val receiver = object : BroadcastReceiver() {
            override fun onReceive(ctx: Context, intent: Intent) {
                val data = intent.getByteArrayExtra("device")!!
                val secret = intent.getByteArrayExtra("secret")
                val addr = intent.getByteArrayExtra("id")!!

                val dev = DeviceInfo(data, addr).apply {
                    this.secret = secret
                }
                addDevice(dev)
            }
        }

        val filter = IntentFilter("device_add")
        LocalBroadcastManager.getInstance(context).registerReceiver(receiver, filter)

        onDispose {
            LocalBroadcastManager.getInstance(context).unregisterReceiver(receiver)
        }
    }

    // UI
    Column(modifier.padding(12.dp)) {
        if (devices.isEmpty()) {
            Text("No devices")
        } else {
            LazyColumn {
                items(devices) { device ->
                    Row(
                        Modifier
                            .fillMaxWidth()
                            .padding(vertical = 6.dp),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text(device.name)
                        Button(onClick = { removeDevice(device) }) {
                            Text("Forget")
                        }
                    }
                }
            }
        }
    }
}
