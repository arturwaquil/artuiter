#ifndef SERVER_COMM_HPP
#define SERVER_COMM_HPP

#include "Packet.hpp"
#include "UI.hpp"

class ServerComm {
    public:
        ServerComm();
        ~ServerComm();
        void set_ui(UI _ui);
        int get_sockfd();
        int read_pkt(int socket, packet* pkt);
        int write_pkt(int socket, packet pkt);
        int _accept();

        void set_quit();

    private:
        int _create();
        int _bind();
        int _listen();

        void error(std::string error_message);

        int socket_file_descriptor;
        int port;
        UI ui;

        bool quit;
};

typedef struct _client_thread_params
{
    std::string username;
    ServerComm* comm_manager;
    int new_sockfd;
    pthread_mutex_t* comm_manager_lock;

} client_thread_params;

client_thread_params create_client_thread_params(std::string username, int new_sockfd, pthread_mutex_t* comm_manager_lock);

#endif