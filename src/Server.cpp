#include "../include/ServerComm.hpp"
#include "../include/Signal.hpp"
#include <mutex>
#include <atomic>

std::atomic<bool> quit(false);    // signal flag

void sigIntHandler(int signum)
{
    // TODO: notify all clients that server is down (i.e. send server_halt message)
    quit.store(true);
}

void* run_client_threads(void* args);
void* run_client_cmd_thread(void* args);
void* run_client_notif_thread(void* args);

ServerComm comm_manager;

int main()
{
    std::cout << "Initializing server..." << std::endl;

    std::list<pthread_t> threads = std::list<pthread_t>();

    pthread_mutex_t comm_manager_lock;
    pthread_mutex_init(&comm_manager_lock, NULL);

    // Set sigIntHandler() as the handler for signal SIGINT (ctrl+c)
    set_signal_action(SIGINT, sigIntHandler);

    std::cout << "Server initialized." << std::endl;

    // Run the server until SIGINT
    while(!quit.load())
    {
        // Accept first pending connection
        // TODO: (need to fix) when SIGINT is received, _accept() returns errno 4 (Interrupted system call) 
        int client_sockfd = comm_manager._accept();

        client_thread_params ctp = create_client_thread_params(client_sockfd, &comm_manager_lock);

        if (quit.load()) break;

        // Launch new thread to deal with client
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, run_client_threads, &ctp);
        threads.push_back(client_thread);
    }

    // Wait for joining all threads
    for (pthread_t t : threads) pthread_join(t, NULL);

    pthread_mutex_destroy(&comm_manager_lock);

    std::cout << "Exiting..." << std::endl;

    return 0;
}

void* run_client_threads(void* args)
{
    // Run the two client threads (for commands and notifications)
    pthread_t client_cmd_thread;
    pthread_t client_notif_thread;
    pthread_create(&client_cmd_thread, NULL, run_client_cmd_thread, args);
    // pthread_create(&client_notif_thread, NULL, run_client_notif_thread, args);

    pthread_join(client_cmd_thread, NULL);
    // pthread_join(client_notif_thread, NULL);

    // When both threads are joined, close the dedicated socket
    client_thread_params ctp = *((client_thread_params*)args);
    close(ctp.new_sockfd);

    return NULL;
}

void* run_client_cmd_thread(void* args)
{
    client_thread_params ctp = *((client_thread_params*)args);
    int sockfd = ctp.new_sockfd;
    pthread_mutex_t comm_manager_lock = *ctp.comm_manager_lock;

    packet pkt;
    while(strcmp(pkt.payload, "exit") != 0)
    {
        // TODO: this mutex logic seems wrong...
        pthread_mutex_lock(&comm_manager_lock);
        comm_manager.read_pkt(sockfd, &pkt);
        pthread_mutex_unlock(&comm_manager_lock);

        std::cout << "type: " << pkt.type << std::endl;
        std::cout << "seqn: " << pkt.seqn << std::endl;
        std::cout << "timestamp: " << pkt.timestamp << std::endl;
        std::cout << "payload: " << pkt.payload << std::endl;

        pkt = create_packet(reply_command, -1, 0, std::string("Message received!"));
        pthread_mutex_lock(&comm_manager_lock);
        comm_manager.write_pkt(sockfd, pkt);
        pthread_mutex_unlock(&comm_manager_lock);
    }

    return NULL;
}

void* run_client_notif_thread(void* args)
{
    return NULL;
}