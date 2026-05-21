package com.example.soundauth;

import java.util.ArrayList;

public interface MessageHandler {

    void onMessage(Message msg);

    class Message {
        public final byte[] address;
        public final byte[] source;
        public final byte command;

        private final ArrayList<Byte> rawData = new ArrayList<>();

        public Message(byte[] header) {
            if (header == null || header.length < 5) {
                throw new IllegalArgumentException("Header must contain at least 5 bytes");
            }

            address = new byte[]{header[0], header[1]};
            source = new byte[]{header[2], header[3]};
            command = header[4];

            for (int i = 0; i < 5; i++) {
                rawData.add(header[i]);
            }
        }

        /**
         * Receive payload bytes and append them to CRC buffer.
         */
        public byte[] recv(int len) {
            byte[] ret = SoundTransferWrapper.Companion
                    .getInstance()
                    .recv(len, false);

            for (byte b : ret) {
                rawData.add(b);
            }

            return ret;
        }

        /**
         * Read trailing CRC16 and validate message integrity.
         */
        public void end() throws InvalidCrcException {
            byte[] crcBytes = SoundTransferWrapper.Companion
                    .getInstance()
                    .recv(2, false);

            if (crcBytes.length != 2) {
                throw new InvalidCrcException("Missing CRC bytes");
            }

            byte[] data = toByteArray(rawData);

            int calculated = SoundTransferWrapper.Companion
                    .getInstance()
                    .crc16(data, data.length);

            int received =
                    ((crcBytes[0] & 0xFF) << 8) |
                            (crcBytes[1] & 0xFF);

            if (calculated != received) {
                throw new InvalidCrcException(
                        String.format(
                                "CRC mismatch: calculated=0x%04X received=0x%04X",
                                calculated,
                                received
                        )
                );
            }
        }

        private byte[] toByteArray(ArrayList<Byte> list) {
            byte[] arr = new byte[list.size()];

            for (int i = 0; i < list.size(); i++) {
                arr[i] = list.get(i);
            }

            return arr;
        }
    }

    class InvalidCrcException extends Exception {
        public InvalidCrcException() {
            super();
        }

        public InvalidCrcException(String msg) {
            super(msg);
        }
    }
}