CC := g++
CFLAGS := -std=c++17 -g -Wall -Wextra
#-Werror
CPPFLAGS := -Iinclude
LDLIBS := -lm -lpthread -lrt

SRC_DIR := src
OBJ_DIR := obj
SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))

.PHONY: all clean
	
all: $(OBJ) app server

app: $(OBJ_DIR)/app.o $(OBJ_DIR)/client.o $(OBJ_DIR)/block.o
	$(CC) $^ $(LDLIBS) -o $@

server: $(OBJ_DIR)/server.o $(OBJ_DIR)/tree.o $(OBJ_DIR)/bucket.o $(OBJ_DIR)/block.o
	$(CC) $^ /lib64/libfdb_c.so $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir $@

clean:
	rm $(OBJ_DIR)/*.o app server
