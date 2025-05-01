package com.example.soundauth;

import java.lang.reflect.Array;

public interface MessageHandler {
    class Message {
        byte[] address;
        byte command;
        byte[] data;

        public Message(byte[] data) {
            address = new byte[]{data[0], data[1]};
            command = data[2];
            this.data = new byte[data.length - 3];
            System.arraycopy(data, 3, this.data, 0, data.length - 3);
        }
    }
    void OnMessage(Message msg);
}
