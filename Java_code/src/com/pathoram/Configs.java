package com.pathoram;

public class Configs {
	//thread fixed number
	public static int THREAD_FIXED = 4;
	
	//server host name and port
	public static String SERVER_HOSTNAME = "localhost";
	public static int SERVER_PORT = 12339;
	
	//path oram configuration, the tree is a full binary tree
	public static int BUCKET_COUNT = 7;//total bucket stored in the server
	public static int LEAF_COUNT = (BUCKET_COUNT+1)/2;//total leaf count
	public static int LEAF_START = BUCKET_COUNT - LEAF_COUNT;//leaf start index in tree node(root is 0)
	public static int Z = 4;//block count in every bucket
	public static int BLOCK_COUNT = BUCKET_COUNT * Z;//total block count
	public static int HEIGHT = (int) (Math.log(BUCKET_COUNT)/Math.log(2) + 1);//tree height
	public static int BLOCK_DATA_LEN = 8;//block payload length in byte
	
	//storage file directory
	public static String STORAGE_PATH = 
			"D:\\BU\\Cloud Computing\\Java PathORAM\\PathORAM\\serverStorage\\bucket_";
	
	
	
}
