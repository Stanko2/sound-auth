package com.example.soundauth

import android.util.Log

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


    external fun openStreams(fftSize: Int = 1024, markerF1: Int = 15000, markerF2: Int = 17000, lowestStrength: Float = -110f, strengthThreshold: Float = -90f)

    external fun closeStreams()

    external fun send(data: ByteArray)

    external fun recv(len: Int, clear: Boolean = false): ByteArray

    @Synchronized
    fun enqueueMessage(data: ByteArray, cmd: Byte, to: ByteArray) {
        val msg = ByteArray(data.size + 5)
        msg[0] = to[0]
        msg[1] = to[1]
        msg[2] = address[0]
        msg[3] = address[1]
        msg[4] = cmd
        System.arraycopy(data, 0, msg, 5, data.size)
        Log.d("MessageHandler", "enqueueMessage: ${ListenService.bytesToHex(msg)}")
        send(msg)
    }
}