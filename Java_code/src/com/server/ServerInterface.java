package com.server;

public interface ServerInterface {

	//initialize server storage
	public void initServer();
	
	//read path from storage, return bytes array of the path
	public byte[] readPath(int pathID);
	
	//write path to server storage
	public void writePath(int pathID, byte[] pathBytes);
}
