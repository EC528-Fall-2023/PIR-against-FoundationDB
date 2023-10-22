package com.pathoram;

import java.util.ArrayList;
import java.util.Arrays;

import com.google.common.primitives.Bytes;
import com.google.common.primitives.Ints;


public class ByteSerializable {

	//transform block from object to byte array
	public byte[] blockSerializable(Block block){
		byte[] leafIDBytes = Ints.toByteArray(block.getLeaf_id());
		byte[] indexBytes = Ints.toByteArray(block.getIndex());
		byte[] returnBytes = Bytes.concat(leafIDBytes,indexBytes,block.getData());
		return returnBytes;
	}
	
	//transform byte array to block
	public Block blockFromSerializable(byte[] blockBytes){
		int startIndex = 0;
		int leaf_id = Ints.fromByteArray(Arrays.copyOfRange(blockBytes, startIndex, 4));
		startIndex += 4;
		int index = Ints.fromByteArray(Arrays.copyOfRange(blockBytes, startIndex, startIndex + 4));
		startIndex += 4;
		byte[] data = Arrays.copyOfRange(blockBytes, startIndex,startIndex + Configs.BLOCK_DATA_LEN);
		Block blk = new Block(leaf_id,index,data);
		return blk;
	}
	
	//transform bucket to byte array
	public byte[] bucketSerializable(Bucket bucket){
		if(bucket == null){
			System.out.println("when bucket serializable, null bucket!");
		}
		byte[][] block_bytes = new byte[Configs.Z][];
		ArrayList<Block> blocks = bucket.getBlocks();
		int count = 0;
		for(Block blk:blocks){
			block_bytes[count] = blockSerializable(blk);
			count++;
		}
		byte[] returndata = block_bytes[0];
		for(count = 1;count<Configs.Z;count++){
			returndata = Bytes.concat(returndata, block_bytes[count]);
		}
		return returndata;
	}
	
	//transform bucket from byte array
	public Bucket bucketFromSerializable(byte[] bucket_bytes){
		int startIndex = 0;
		ArrayList<Block> blocks = new ArrayList<Block>(Configs.Z);
		int block_bytes_len = Configs.BLOCK_DATA_LEN+4+4;
		for(int i =0;i<Configs.Z;i++){
			byte[] block_bytes = Arrays.copyOfRange(
					bucket_bytes, startIndex, startIndex+block_bytes_len);
			Block blk = blockFromSerializable(block_bytes);
			blocks.add(blk);
			startIndex += block_bytes_len;
		}
		Bucket bucket = new Bucket(blocks);
		return bucket;
	}
}
