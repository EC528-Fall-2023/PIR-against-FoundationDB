CC := g++
CFLAGS = -std=c++17 -Wall -Wextra -Werror
CPPFLAGS := -Iinclude
LDLIBS := -lm -pthread -lrt -lssl -lcrypto

ifdef DEBUG
	CFLAGS += -g3 -DDEBUG
endif

ifdef SEED
	CFLAGS += -DSEEDED_RANDOM
endif

ifdef BYTES
	CFLAGS += -DBLOCK_SIZE=$(BYTES)
endif

ifdef BLOCKS
	CFLAGS += -DBLOCKS_PER_BUCKET=$(BLOCKS)
endif

ifdef LEVELS
	CFLAGS += -DTREE_LEVELS=$(LEVELS)
endif

SRC_DIR := src
OBJ_DIR := obj
INC_DIR := include
BMK_DIR := bmk
BIN_DIR := bin
SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))
INC := $(wildcard $(INC_DIR)/*.h)

.PHONY: all server single_client multiclient bm startup fdb clear_fdb clean

all: single_client multiclient

single_client: app_single_client

multiclient: app_multiclient master_client

bm: server startup fdb clear_fdb

server: $(OBJ_DIR)/server.o $(OBJ_DIR)/block.o | $(BIN_DIR)
	$(CC) $^ /lib64/libfdb_c.so $(LDLIBS) -o $(BIN_DIR)/$@
	
app_single_client: $(OBJ_DIR)/app_single_client.o $(OBJ_DIR)/single_client.o $(OBJ_DIR)/block.o | $(BIN_DIR) server
	$(CC) $^ $(LDLIBS) -o $(BIN_DIR)/$@

app_multiclient: $(OBJ_DIR)/app_multiclient.o $(OBJ_DIR)/multiclient.o $(OBJ_DIR)/block.o | $(BIN_DIR) master_client server
	$(CC) $^ $(LDLIBS) -o $(BIN_DIR)/$@
	
master_client: $(OBJ_DIR)/master_client.o $(OBJ_DIR)/single_client.o $(OBJ_DIR)/block.o | $(BIN_DIR) server
	$(CC) $^ $(LDLIBS) -o $(BIN_DIR)/$@

startup: $(OBJ_DIR)/startup.o $(OBJ_DIR)/single_client.o $(OBJ_DIR)/block.o | $(BIN_DIR) server clear_fdb
	$(CC) $^ $(LDLIBS) -o $(BIN_DIR)/$@

fdb: $(OBJ_DIR)/fdb.o $(OBJ_DIR)/single_client.o $(OBJ_DIR)/block.o | $(BIN_DIR) server
	$(CC) $^ $(LDLIBS) -o $(BIN_DIR)/$@

clear_fdb: $(OBJ_DIR)/clear_fdb.o | $(BIN_DIR)
	$(CC) $^ /lib64/libfdb_c.so $(LDLIBS) -o $(BIN_DIR)/$@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(INC_DIR)/%.h | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(BMK_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR) $(BIN_DIR):
	mkdir $@

clean: clear_fdb
	-sudo pkill -f $(PWD)/bin/server
	-sudo pkill -f $(PWD)/bin/master_client
	$(BIN_DIR)/clear_fdb
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*
