package com.server;
/*
 * read path from storage
 * write path to storage
 */
import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.AsynchronousChannelGroup;
import java.nio.channels.AsynchronousServerSocketChannel;
import java.nio.channels.AsynchronousSocketChannel;
import java.nio.channels.CompletionHandler;
import java.util.Arrays;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import com.google.common.primitives.Bytes;
import com.google.common.primitives.Ints;
import com.pathoram.*;

public class Server implements ServerInterface {
	
	ServerStorage storage;
	ByteSerializable seria;

	boolean initedServer = false;

	public Server() {
		this.storage = new ServerStorage();
		this.seria = new ByteSerializable();
	}

	public void run() {
		try {
			AsynchronousChannelGroup threadGroup = AsynchronousChannelGroup.withFixedThreadPool(Configs.THREAD_FIXED,
					Executors.defaultThreadFactory());
			AsynchronousServerSocketChannel channel = AsynchronousServerSocketChannel.open(threadGroup)
					.bind(new InetSocketAddress(Configs.SERVER_PORT));
			System.out.println("server wait for connection.");
			channel.accept(null, new CompletionHandler<AsynchronousSocketChannel, Void>() {

				@Override
				public void completed(AsynchronousSocketChannel serveClientChannel, Void att) {
					// TODO Auto-generated method stub
					// start listening for other connections
					try {
						System.out.println("Client connected from " + serveClientChannel.getRemoteAddress());
					} catch (IOException e) {
						throw new RuntimeException(e);
					}
					channel.accept(null, this);
					// start up a new thread to serve this connection
					Runnable serializeProcedure = () -> serveClient(serveClientChannel);
					new Thread(serializeProcedure).start();
				}

				@Override
				public void failed(Throwable arg0, Void arg1) {
					// TODO Auto-generated method stub
				}
			});
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@SuppressWarnings("rawtypes")
	public void serveClient(AsynchronousSocketChannel mChannel) {
		try {
			while (true) {
				ByteBuffer messageTypeAndSize = ByteBuffer.allocate(4 + 4);
				Future messageRead = mChannel.read(messageTypeAndSize);
				messageRead.get();
				messageTypeAndSize.flip();
				int[] messageTypeAndSizeInt = MessageUtility.parseTypeAndLength(messageTypeAndSize);
				int type = messageTypeAndSizeInt[0];// message type
				int size = messageTypeAndSizeInt[1];// data size
				messageTypeAndSize = null;

				ByteBuffer message = null;
				if (size != 0) {
					// read rest of the message
					message = ByteBuffer.allocate(size);
					while (message.remaining() > 0) {
						Future entireRead = mChannel.read(message);
						entireRead.get();
					}
					message.flip();
				}

				byte[] serializedResponse = null;// response data
				byte[] responseHeader = null;// response message head

				if (type == MessageUtility.ORAM_INIT) {
					Lock lock = new ReentrantLock();
					lock.lock();
					try {
						if (!initedServer) {
							System.out.println("server is initializing!");
							initServer();
						} else {
							System.out.println("server had been initialized!");
							lock.unlock();
							break;
						}
						initedServer = true;
					} catch (Exception e) {
						e.printStackTrace();
					} finally {
						lock.unlock();
					}
					byte[] responseBytes = { 1 };
					serializedResponse = responseBytes;
					responseHeader = MessageUtility.createMessageHeaderBytes(MessageUtility.ORAM_INIT,
							serializedResponse.length);
					System.out.println("server INIT successful!");
					responseBytes = null;
				}
				if (type == MessageUtility.ORAM_READPATH) {
					// get path id
					byte[] pathIdBytes = new byte[4];
					message.get(pathIdBytes);
					int pathId = Ints.fromByteArray(pathIdBytes);
					System.out.println("Received ORAM_READPATH request from client for path " + pathId);
					// read path
					byte[] pathBytes = readPath(pathId);
					serializedResponse = pathBytes;
					responseHeader = MessageUtility.createMessageHeaderBytes(MessageUtility.ORAM_READPATH,
							serializedResponse.length);
					System.out.println("server READPATH "+pathId+" successful!");
					pathIdBytes = null;
					pathBytes = null;
				}
				if (type == MessageUtility.ORAM_WRITEPATH) {
					//a path combined by HEIGHT buckets, and a bucket combined by Z blocks,
					//and a block combined by leaf id, index and data array
					int pathByteLen = Configs.HEIGHT * (Configs.BLOCK_DATA_LEN + 4 + 4) * Configs.Z;
					byte[] pathIdBytes = new byte[4];
					byte[] pathBytes = new byte[pathByteLen];
					message.get(pathIdBytes);
					message.get(pathBytes);
					int pathId = Ints.fromByteArray(pathIdBytes);
					System.out.println("Received ORAM_WRITEPATH request from client for path " + pathId);

					//synchronize write back path
					Lock lock = new ReentrantLock();
					lock.lock();
					try {
						writePath(pathId, pathBytes);
					} catch (Exception e) {
						e.printStackTrace();
					} finally {
						lock.unlock();
					}
					byte[] responseBytes = { 1 };
					serializedResponse = responseBytes;
					responseHeader = MessageUtility.createMessageHeaderBytes(MessageUtility.ORAM_WRITEPATH,
							serializedResponse.length);
					System.out.println("server WRITEPATH "+pathId+" successful!");
					System.out.println();
					pathIdBytes = null;
					pathBytes = null;
				}
				// send response to client
				ByteBuffer responseMessage = ByteBuffer.wrap(Bytes.concat(responseHeader, serializedResponse));
				while (responseMessage.remaining() > 0) {
					Future responseMessageRead = mChannel.write(responseMessage);
					responseMessageRead.get();
				}
			}
		} catch (Exception e) {
			try {
				mChannel.close();
			} catch (IOException e1) {

			}
		}
	}

	@Override
	public void initServer() {
		// TODO Auto-generated method stub
		// init storage
		for (int i = 0; i < Configs.BUCKET_COUNT; i++) {
			Bucket bkt = new Bucket();
			try {
				storage.set_bucket(i, bkt);
				System.out.println("Initialized bucket " + i);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				System.out.println("Failed to initialize bucket " + i);
				e.printStackTrace();
			}
			// System.out.println("init bucket "+i+" successful.");
		}
		System.out.println("init server" + " successful.");
		System.out.println();
	}

	@Override
	public byte[] readPath(int pathID) {
		System.out.println("Accessed read bucket/path " + pathID);
		// TODO Auto-generated method stub
		byte[][] path_bytes = new byte[Configs.HEIGHT][];
		int count = 0;
		// get bucket from leaf to root
		for (int pos_run = pathID; pos_run >= 0; pos_run = (pos_run - 1) >> 1) {
			Bucket bucket = new Bucket();
			try {
				bucket = storage.get_bucket(pos_run);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			// every row store a bucket bytes
			path_bytes[count] = seria.bucketSerializable(bucket);
			count++;
			if (pos_run == 0)
				break;
		}
		byte[] returnData = path_bytes[0];
		// all buckets bytes to a byte array
		for (int i = 1; i < Configs.HEIGHT; i++) {
			returnData = Bytes.concat(returnData, path_bytes[i]);
		}
		path_bytes = null;
		System.out.println("Data received for operation: " + Arrays.toString(returnData));
		return returnData;
	}

	@Override
	public void writePath(int pathID, byte[] pathBytes) {
		System.out.println("Accessed write bucket/path " + pathID);
		// TODO Auto-generated method stub
		int startIndex = 0;
		int bucket_bytes_len = Configs.Z*(Configs.BLOCK_DATA_LEN+4+4);
		for (int pos_run = pathID; pos_run >= 0; pos_run = (pos_run - 1) >> 1) {
			byte[] bucket_bytes = Arrays.copyOfRange(pathBytes, startIndex, startIndex+bucket_bytes_len);
			startIndex += bucket_bytes_len;
			Bucket bucket = seria.bucketFromSerializable(bucket_bytes);
			try {
				storage.set_bucket(pos_run, bucket);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		Server server = new Server();
		server.run();
	}

}
