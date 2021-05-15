#ifndef SERVER_COMM_HPP
#define SERVER_COMM_HPP

#include "Packet.hpp"
#include "GeneralComm.hpp"
#include "Typedefs.hpp"

class ServerComm : public GeneralComm {
    public:
        ServerComm(){};
        ~ServerComm();
        void init(int id);

        int get_sockfd();

        skt_pair _accept();
        void set_quit();

    private:
        int _create();
        int _bind();
        int _listen();

        int socket_file_descriptor;
        int port;

        bool quit;
};

#endif