package com.client;

import com.pathoram.Bucket;

public interface ClientInterface {
	//client operation includes read and write
	public enum Operation {READ, WRITE};

	public void initServer();//initialize server
	
	public void readPath(int pathID);//read path
	
	public void writePath(int pathID,Bucket[] bucket_list);//write path
	
	public void close();//close connect
	
}
