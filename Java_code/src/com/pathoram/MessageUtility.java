package com.pathoram;

import java.nio.ByteBuffer;

import com.google.common.primitives.Bytes;
import com.google.common.primitives.Ints;

public class MessageUtility {
	
	//server message type
	public static final int ORAM_INIT = 0;
	public static final int ORAM_READPATH = 1;
	public static final int ORAM_WRITEPATH = 2;
	public static final int ORAM_CLOSE = 3;
	
	//parse message received from server
	public static int[] parseTypeAndLength(ByteBuffer b) {
        int[] typeAndLength = new int[2];

        byte[] messageTypeBytes = new byte[4];
        byte[] messageLengthBytes = new byte[4];

        b.get(messageTypeBytes);
        int messageType = Ints.fromByteArray(messageTypeBytes);

        //message length excepts header length
        b.get(messageLengthBytes);
        int messageLength = Ints.fromByteArray(messageLengthBytes);

        typeAndLength[0] = messageType;
        typeAndLength[1] = messageLength;

        return typeAndLength;
    }
 
	public static byte[] createMessageHeaderBytes(int messageType, int messageSize) {
		byte[] messageTypeBytes = Ints.toByteArray(messageType);
		byte[] messageLengthBytes = Ints.toByteArray(messageSize);

		return Bytes.concat(messageTypeBytes, messageLengthBytes);
	}

	public static ByteBuffer createMessageHeaderBuffer(int messageType, int messageSize) {
		byte[] messageTypeBytes = Ints.toByteArray(messageType);
		byte[] messageLengthBytes = Ints.toByteArray(messageSize);

		return ByteBuffer.wrap(Bytes.concat(messageTypeBytes, messageLengthBytes));
	}
}
