package com.pathoram;

import java.util.HashMap;
import java.util.Map;

/*
 * stash is a temporary storage in the client
 * client stores path into the stash
 * client write path back to the server from the stash
 */
public class Stash {

	private Map<Integer,Block> stash_hash;
	
	public Stash(){
		stash_hash  = new HashMap<Integer,Block>();
	}
	
	public void add(Block block) {
		//stash block is always the freshest block
		if (!stash_hash.containsKey(block.getIndex())) {
			stash_hash.put(block.getIndex(), block);
		}
	}
	
	public Block find_by_index(int index) {
		if (!stash_hash.containsKey(index)) {
			return null;
		}
		return stash_hash.get(index);
	}
	
	public void updataBlock(Block block) {
		if (stash_hash.containsKey(block.getIndex())) {
			stash_hash.remove(block.getIndex());
		}
		stash_hash.put(block.getIndex(), block);
	}
	
	public Block[] remove_by_bucket(int position) {
		Block[] stash_list = new Block[Configs.Z];
		for (int i = 0; i < Configs.Z; i++) {
			stash_list[i] = new Block();
		}
		int count = 0;
		for (Integer index : stash_hash.keySet()) {
			int leaf = stash_hash.get(index).getLeaf_id();
			if (leaf >= position) {
				for (int pos = leaf; pos >= 0; pos = (pos - 1) >> 1) {
					if (pos == position) {//intersect--'position' is in the path of 'leaf'
						//can write block into bucket
						stash_list[count] = stash_hash.get(index);
						count++;
					}
					if (pos == 0)
						break;
				}
			}
			if (count == Configs.Z)//bucket is full
				break;
		}
		// remove blocks from stash
		for (int i = 0; i < Configs.Z; i++) {
			if (stash_list[i].getIndex() != -1) {
				stash_hash.remove(stash_list[i].getIndex());
			}
		}
		return stash_list;
	}
}
