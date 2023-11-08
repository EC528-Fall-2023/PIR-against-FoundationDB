CC := g++
CFLAGS = -std=c++17 -Wall -Wextra -Werror
CPPFLAGS := -Iinclude
LDLIBS := -lm -lpthread -lrt

ifeq ($(DEBUG), 1)
	CFLAGS += -g -DDEBUG
endif

SRC_DIR := src
OBJ_DIR := obj
INC_DIR := include
SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))
INC := $(wildcard $(INC_DIR)/*.h)

.PHONY: all single_client multiclient init clean

all: single_client multiclient

single_client: server app_single_client

multiclient: server app_multiclient master_client

server: $(OBJ_DIR)/server.o $(OBJ_DIR)/block.o
	$(CC) $^ /lib64/libfdb_c.so $(LDLIBS) -o $@
	
app_single_client: $(OBJ_DIR)/app_single_client.o $(OBJ_DIR)/single_client.o $(OBJ_DIR)/block.o
	$(CC) $^ $(LDLIBS) -o $@

app_multiclient: $(OBJ_DIR)/app_multiclient.o $(OBJ_DIR)/multiclient.o $(OBJ_DIR)/block.o
	$(CC) $^ $(LDLIBS) -o $@

master_client: $(OBJ_DIR)/master_client.o $(OBJ_DIR)/single_client.o $(OBJ_DIR)/block.o
	$(CC) $^ $(LDLIBS) -o $@

init: init_values init_random

init_values: testing/init_values.cpp $(OBJ_DIR)/block.o
	$(CC) $(CPPFLAGS) $^ /lib64/libfdb_c.so $(LDLIBS) -o $@

init_random: testing/init_random.cpp $(OBJ_DIR)/block.o
	$(CC) $(CPPFLAGS) $^ /lib64/libfdb_c.so $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(INC_DIR)/%.h | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir $@

clean:
	rm $(OBJ_DIR)/*.o server app_single_client app_multiclient master_client
