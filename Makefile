CC = g++
CFLAGS = -Wall -lpthread -lncurses
SRC_DIR = ./src
OBJ_DIR = ./obj

.PHONY: all clean

all: app_client app_server

CLIENT_DEPS += $(OBJ_DIR)/Client.o
CLIENT_DEPS += $(OBJ_DIR)/ClientComm.o
CLIENT_DEPS += $(OBJ_DIR)/Packet.o
CLIENT_DEPS += $(OBJ_DIR)/Signal.o
CLIENT_DEPS += $(OBJ_DIR)/ClientUI.o
app_client: $(CLIENT_DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

SERVER_DEPS += $(OBJ_DIR)/Server.o
SERVER_DEPS += $(OBJ_DIR)/Notification.o
SERVER_DEPS += $(OBJ_DIR)/Packet.o
SERVER_DEPS += $(OBJ_DIR)/Profile.o
SERVER_DEPS += $(OBJ_DIR)/ServerComm.o
SERVER_DEPS += $(OBJ_DIR)/Signal.o
app_server: $(SERVER_DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf app_client app_server $(OBJ_DIR)/*.o

redo: clean all