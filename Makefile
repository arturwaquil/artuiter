all:
	g++ src/Client.cpp src/ClientUI.cpp src/ClientComm.cpp -o app_client
	g++ src/Server.cpp src/ServerComm.cpp -o app_server