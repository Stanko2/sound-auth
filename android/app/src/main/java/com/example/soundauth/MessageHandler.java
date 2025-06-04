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
            this.data = new byte[data.length - 5];
            System.arraycopy(data, 5, this.data, 0, this.data.length);
        }
    }
    void OnMessage(Message msg);
}
