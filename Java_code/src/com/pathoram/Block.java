package com.pathoram;

import java.io.Serializable;

public class Block implements Serializable{
	/**
	 * @param leaf_id: block path id
	 * @param inex: block unique id
	 * @param data: block payload
	 */
	private static final long serialVersionUID = 1L;
	
	private int leaf_id;
	private int index;
	private byte[] data;
	
	public Block(){
		leaf_id = -1;
		index = -1;
		data = new byte[Configs.BLOCK_DATA_LEN];
	}
	
	public Block(int leaf_id, int index, byte[] data){
		this.leaf_id = leaf_id;
		this.index = index;
		this.data = data;
	}
	
	public int getLeaf_id() {
		return leaf_id;
	}
	public void setLeaf_id(int leaf_id) {
		this.leaf_id = leaf_id;
	}
	public int getIndex() {
		return index;
	}
	public void setIndex(int index) {
		this.index = index;
	}
	public byte[] getData() {
		return data;
	}
	public void setData(byte[] data) {
		this.data = data;
	}	
}
