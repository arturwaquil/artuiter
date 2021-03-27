all:
	g++ src/Client.cpp src/ClientUI.cpp -o app_client
	g++ src/Server.cpp -o app_server