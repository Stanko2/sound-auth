package com.example.soundauth;

import java.lang.reflect.Array;

public interface MessageHandler {
    class Message {
        byte[] address;
        byte[] source;
        byte command;
        byte[] data;

        public Message(byte[] data) {
            address = new byte[]{data[0], data[1]};
            source = new byte[]{data[2], data[3]};
            command = data[4];
            this.data = new byte[data.length - 3];
            System.arraycopy(data, 3, this.data, 0, data.length - 3);
        }
    }
    void OnMessage(Message msg);
}
