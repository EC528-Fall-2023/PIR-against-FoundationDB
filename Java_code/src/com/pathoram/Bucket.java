package com.pathoram;

import java.io.Serializable;
import java.util.ArrayList;

public class Bucket implements Serializable{

	/**
	 * implement Serializable in order to write bucket object to file
	 * bucket contains Z blocks
	 */
	private static final long serialVersionUID = 1L;
	
	private ArrayList<Block> blocks;

	public Bucket(){
		this.blocks = new ArrayList<Block>(Configs.Z);
		for(int i=0;i<Configs.Z;i++){
			this.blocks.add(new Block());
		}
	}
	
	public Bucket(ArrayList<Block> blocks){
		this.blocks = blocks;
	}
	
	public ArrayList<Block> getBlocks() {
		return blocks;
	}

	public void setBlocks(ArrayList<Block> blocks) {
		this.blocks = blocks;
	}

}
