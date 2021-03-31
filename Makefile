CC = g++
CFLAGS = -Wall -lpthread
SRC_DIR = ./src
OBJ_DIR = ./obj

.PHONY: all clean

all: app_client app_server

CLIENT_DEPS += $(OBJ_DIR)/Client.o
CLIENT_DEPS += $(OBJ_DIR)/ClientUI.o
CLIENT_DEPS += $(OBJ_DIR)/ClientComm.o
app_client: $(CLIENT_DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

SERVER_DEPS += $(OBJ_DIR)/Server.o
SERVER_DEPS += $(OBJ_DIR)/ServerComm.o
app_server: $(SERVER_DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf app_client app_server $(OBJ_DIR)/*.o

redo: clean all