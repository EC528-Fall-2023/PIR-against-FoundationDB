CC := gcc
CFLAGS := -std=c11 -g -Wall -Wextra -Werror
CPPFLAGS := -Iinclude
LDLIBS := -lm -lpthread -lrt

SRC_DIR := src
OBJ_DIR := obj
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all clean
	
all: app server

app: $(OBJ_DIR)/app.o $(OBJ_DIR)/client.o
	$(CC) $^ $(LDLIBS) -o $@

server: $(OBJ_DIR)/server.o
	$(CC) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir $@

clean:
	rm $(OBJ_DIR)/*.o app server
