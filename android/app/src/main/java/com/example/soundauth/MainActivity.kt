package com.example.soundauth

import android.annotation.SuppressLint
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LargeTopAppBar
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextOverflow
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.example.soundauth.ui.MainScreen
import com.example.soundauth.ui.SoundTestingScreen

class MainActivity : ComponentActivity() {

    companion object {
        const val TAG = "MainActivity"

    }

    private sealed class Screen {
        object Main : Screen()
        object SoundTest : Screen()
    }

    @OptIn(ExperimentalMaterial3Api::class)
    @Composable
    fun GetTopBar(onNavigationIconClick: () -> Unit) {

        LargeTopAppBar(
            colors = TopAppBarDefaults.topAppBarColors(
                containerColor = MaterialTheme.colorScheme.primaryContainer,
                titleContentColor = MaterialTheme.colorScheme.primary,
            ),
            title = {
                Text(
                    "Sound Auth",
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis
                )
            },
            navigationIcon = {
                IconButton(onClick = onNavigationIconClick) {
                    Icon(
                        imageVector = Icons.Filled.Settings,
                        contentDescription = "Switch screen"
                    )
                }
            }
        )
    }

    @SuppressLint("NewApi")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        System.loadLibrary("soundAuth")
        val soundTestingScreen = SoundTestingScreen()
        soundTestingScreen.start()

        setContent {
            var currentScreen: Screen by remember { mutableStateOf(Screen.Main) }

            MaterialTheme {
                Scaffold(
                    topBar = {
                        GetTopBar {
                            currentScreen = when (currentScreen) {
                                Screen.Main -> Screen.SoundTest
                                Screen.SoundTest -> Screen.Main
                            }
                            if (currentScreen == Screen.SoundTest) {
                                stopService(Intent(this, ListenService::class.java))
                            }
                        }
                    },
                ) { innerPadding ->
                    Box(modifier = Modifier.padding(innerPadding)) {
                        when (currentScreen) {
                            Screen.Main -> MainScreen(this@MainActivity).UI()
                            Screen.SoundTest -> soundTestingScreen.UI()
                        }
                    }
                }
            }
        }
    }

    override fun onResume() {
        super.onResume()
        LocalBroadcastManager.getInstance(this)
            .registerReceiver(audioMessageReceiver, IntentFilter("message"))
        LocalBroadcastManager.getInstance(this)
            .registerReceiver(errorReceiver, IntentFilter("error"))
    }

    override fun onPause() {
        LocalBroadcastManager.getInstance(this)
            .unregisterReceiver(audioMessageReceiver)
        LocalBroadcastManager.getInstance(this)
            .unregisterReceiver(errorReceiver)
        super.onPause()
    }

    private val audioMessageReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            Log.d(TAG, "onReceive: ${intent.getStringExtra("message")}")
        }
    }

    private val errorReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            Toast.makeText(
                context,
                intent.getStringExtra("message"),
                Toast.LENGTH_SHORT
            ).show()
        }
    }

}