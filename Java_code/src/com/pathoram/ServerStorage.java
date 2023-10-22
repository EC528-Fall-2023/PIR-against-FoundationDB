package com.pathoram;
/*
 * store each bucket to a file
 */

import java.io.IOException;
import java.util.Arrays;

import com.foundationdb.*;
import com.foundationdb.tuple.Tuple;
import com.foundationdb.async.Function;
import com.foundationdb.Transaction;

public class ServerStorage {

    private Database db;
    ByteSerializable seria;
    public ServerStorage(){
        this.seria = new ByteSerializable();
        FDB fdb = FDB.selectAPIVersion(300); // Use the appropriate API version
        db = fdb.open();
	}
	
	//write bucket to file, a bucket in a file
	public void set_bucket(int id,Bucket bkt) throws IOException{
        byte[] key = Tuple.from("bucket", id).pack();
        byte[] value = seria.bucketSerializable(bkt);
        System.out.println("Storing key: " + Arrays.toString(key));
        System.out.println("Storing value: " + Arrays.toString(value));
        db.run(new Function<Transaction, Void>() {
            public Void apply(Transaction tr) {
                try {
                    tr.set(key, value);
                    tr.commit().get();
                    System.out.println("FDB transaction success!");
                } catch (FDBException e) {
                    System.out.println("write bucket " + id + " failed");
                    e.printStackTrace();
                }
                return null;
            }
        });
	}

    public Bucket get_bucket(int id) throws IOException {
        byte[] key = Tuple.from("bucket", id).pack();

        byte[] result = db.run(tr -> tr.get(key)).get();

        if (result != null) {
            return seria.bucketFromSerializable(result);
        } else {
            System.out.println("Error: When reading bucket " + id + ", found no record in storage!");
            return null; // Handle non-existing bucket if needed
        }
    }
}
