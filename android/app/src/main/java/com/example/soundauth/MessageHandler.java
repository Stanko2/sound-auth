package com.example.soundauth;
public interface MessageHandler {
    class Message {
        int address;
        String command;
        byte[] data;

        public Message(byte[] data) {
            address = data[0];
        }
    }
    void OnMessage(Message msg);
}
