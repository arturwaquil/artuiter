all:
	g++ src/Client.cpp src/ClientUI.cpp src/ClientComm.cpp -o app_client
	g++ src/Server.cpp src/ServerComm.cpp -lpthread -o app_server