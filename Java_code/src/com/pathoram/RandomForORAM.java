package com.pathoram;

import java.util.Random;

/*
 * get a new random path for the current reading block
 * the path id is the tree leaf id
 */
public class RandomForORAM {

	Random rnd_generator;
	
	public RandomForORAM(){
		rnd_generator = new Random();
	}
	
	public int getRandomLeaf(){
		int randomLeaf = rnd_generator.nextInt(Configs.LEAF_COUNT);
		return randomLeaf;
	}
}
