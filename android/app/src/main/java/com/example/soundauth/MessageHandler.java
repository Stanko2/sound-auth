package com.example.soundauth;

import java.lang.reflect.Array;

public interface MessageHandler {
    class Message {
        byte[] address;
        byte[] source;
        byte command;

        public Message(byte[] data) {
            address = new byte[]{data[0], data[1]};
            source = new byte[]{data[2], data[3]};
            command = data[4];
        }
    }
    void OnMessage(Message msg);
}
