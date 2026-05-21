package com.example.soundauth

import android.content.Context
import android.content.SharedPreferences
import android.util.Log
import androidx.core.content.edit

class SoundTransferWrapper {
    private lateinit var address: ByteArray

    companion object  {
        val instance: SoundTransferWrapper by lazy {
            SoundTransferWrapper()
        }
    }

    fun init(address: ByteArray) {
        this.address = address
    }


    fun launch(prefs: AppPrefs) {
        openStreams(
            fftSize = prefs.fftSize,
            markerF1 = prefs.markerF1.toInt(),
            markerF2 = prefs.markerF2.toInt(),
            lowestStrength = prefs.lowestStr,
            strengthThreshold = prefs.strengthThresh,
            chunkSize = prefs.chunkSize,
            modulationType = prefs.modType,
            startFrequency = prefs.startFreq,
            spacing = prefs.spacing,
            bitsPerFrame = prefs.bitsPerFrame,
            regionSize = prefs.regionSize,
            numRegions = prefs.numRegions,
            f1 = prefs.simpleF1,
            f2 = prefs.simpleF2
        )
    }

    external fun openStreams(
        fftSize: Int = 1024,
        markerF1: Int = 15000,
        markerF2: Int = 17000,
        lowestStrength: Float = -110f,
        strengthThreshold: Float = -90f,
        chunkSize: Int = 64,
        modulationType: String?,
        startFrequency: Int,
        spacing: Int,
        bitsPerFrame: Int,
        regionSize: Int,
        numRegions: Int,
        f1: Int,
        f2: Int
    )

    external fun closeStreams()

    external fun send(data: ByteArray)

    external fun recv(len: Int, clear: Boolean = false): ByteArray

    fun crc16(data: ByteArray, length: Int): Int {
        var crc = 0xFFFF

        for (i in 0 until length) {
            crc = crc xor (data[i].toInt() and 0xFF)

            for (j in 0 until 8) {
                crc = if ((crc and 1) != 0) {
                    (crc ushr 1) xor 0xA001
                } else {
                    crc ushr 1
                }
            }
        }

        return crc and 0xFFFF
    }

    @Synchronized
    fun enqueueMessage(data: ByteArray, cmd: Byte, to: ByteArray) {
        val msg = ByteArray(data.size + 7)
        msg[0] = to[0]
        msg[1] = to[1]
        msg[2] = address[0]
        msg[3] = address[1]
        msg[4] = cmd
        System.arraycopy(data, 0, msg, 5, data.size)

        val crc = crc16(msg, msg.size - 2)

        msg[msg.size - 2] = ((crc ushr 8) and 0xFF).toByte()
        msg[msg.size - 1] = (crc and 0xFF).toByte()


        Log.d("MessageHandler", "enqueueMessage: ${ListenService.bytesToHex(msg)}")
        send(msg)
    }
}

class AppPrefs(context: Context) {
    private val p = context.getSharedPreferences("sound_settings", Context.MODE_PRIVATE)

    // Protocol
    var fftSize by IntPref(p, "fftSize", 1024)
    var markerF1 by FloatPref(p, "markerF1", 15000f)
    var markerF2 by FloatPref(p, "markerF2", 17000f)
    var lowestStr by FloatPref(p, "lowestStr", -110f)
    var strengthThresh by FloatPref(p, "strengthThresh", -90f)
    var chunkSize by IntPref(p, "chunkSize", 64)

    // Modulation
    var modType by StringPref(p, "modType", "2tone")
    var startFreq by IntPref(p, "startFreq", 15000)
    var spacing by IntPref(p, "spacing", 5)
    var bitsPerFrame by IntPref(p, "bitsPerFrame", 4)
    var regionSize by IntPref(p, "regionSize", 4)
    var numRegions by IntPref(p, "numRegions", 2)
    var simpleF1 by IntPref(p, "simpleF1", 15000)
    var simpleF2 by IntPref(p, "simpleF2", 17000)
}

class IntPref(val p: SharedPreferences, val k: String, val d: Int) {
    operator fun getValue(t: Any?, prop: Any?) = p.getInt(k, d)
    operator fun setValue(t: Any?, prop: Any?, v: Int) = p.edit { putInt(k, v) }
}
class FloatPref(val p: SharedPreferences, val k: String, val d: Float) {
    operator fun getValue(t: Any?, prop: Any?) = p.getFloat(k, d)
    operator fun setValue(t: Any?, prop: Any?, v: Float) = p.edit {putFloat(k, v)}
}
class StringPref(val p: SharedPreferences, val k: String, val d: String) {
    operator fun getValue(t: Any?, prop: Any?) = p.getString(k, d)
    operator fun setValue(t: Any?, prop: Any?, v: String?) = p.edit { putString(k, v) }
}