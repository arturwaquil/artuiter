#include "../include/Profile.hpp"
#include "../include/ServerComm.hpp"
#include "../include/Signal.hpp"

#include <atomic>
#include <iostream>
#include <list>
#include <map>
#include <mutex>

#include <unistd.h>

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
std::map<std::string, Profile> profiles;    // Store all profiles, indexed by username

int main()
{
    std::cout << "Initializing server..." << std::endl;

    std::list<pthread_t> threads = std::list<pthread_t>();

    pthread_mutex_t comm_manager_lock;
    pthread_mutex_init(&comm_manager_lock, NULL);

    // Set sigIntHandler() as the handler for signal SIGINT (ctrl+c)
    set_signal_action(SIGINT, sigIntHandler);

    // TODO: fetch profile info from database

    std::cout << "Server initialized." << std::endl;

    // Run the server until SIGINT
    while(!quit.load())
    {
        // Accept first pending connection
        // TODO: (need to fix) when SIGINT is received, _accept() returns errno 4 (Interrupted system call) 
        int client_sockfd = comm_manager._accept();

        client_thread_params ctp = create_client_thread_params(std::string(), client_sockfd, &comm_manager_lock);

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
    client_thread_params ctp = *((client_thread_params*)args);
    int sockfd = ctp.new_sockfd;
    pthread_mutex_t comm_manager_lock = *ctp.comm_manager_lock;

    packet pkt;

    // Attempt to login with provided username
    pthread_mutex_lock(&comm_manager_lock);
    comm_manager.read_pkt(sockfd, &pkt);
    pthread_mutex_unlock(&comm_manager_lock);
    if (pkt.type == login)
    {
        std::string username = std::string(pkt.payload);

        // TODO: test if not already connected twice!

        // If user doesn't exist, create new profile
        if (!search_by_username(profiles, username)) profiles.emplace(username, Profile(username));
        // TODO: why does "profiles[username] = Profile(username)" not work?

        // Update args so that the client threads receive the username
        ((client_thread_params*)args)->username = username;

        // Send positive reply
        pkt = create_packet(reply_login, 0, 0, "OK");
        pthread_mutex_lock(&comm_manager_lock);
        comm_manager.write_pkt(sockfd, pkt);
        pthread_mutex_unlock(&comm_manager_lock);
        std::cout << "User " << username << " logged in.\n";
    }
    else
    {
        pkt = create_packet(reply_login, 0, 0, "FAILED");
        pthread_mutex_lock(&comm_manager_lock);
        comm_manager.write_pkt(sockfd, pkt);
        pthread_mutex_unlock(&comm_manager_lock);
        close(ctp.new_sockfd);
        return NULL;
    }

    // Run the two client threads (for commands and notifications)
    pthread_t client_cmd_thread;
    // pthread_t client_notif_thread;
    pthread_create(&client_cmd_thread, NULL, run_client_cmd_thread, args);
    // pthread_create(&client_notif_thread, NULL, run_client_notif_thread, args);

    pthread_join(client_cmd_thread, NULL);
    // pthread_join(client_notif_thread, NULL);

    // When both threads are joined, close the dedicated socket
    close(ctp.new_sockfd);

    return NULL;
}

void* run_client_cmd_thread(void* args)
{
    client_thread_params ctp = *((client_thread_params*)args);
    std::string username = ctp.username;
    int sockfd = ctp.new_sockfd;
    pthread_mutex_t comm_manager_lock = *ctp.comm_manager_lock;

    packet pkt;
    while(true)
    {
        // TODO: this mutex logic seems wrong...
        pthread_mutex_lock(&comm_manager_lock);
        comm_manager.read_pkt(sockfd, &pkt);
        pthread_mutex_unlock(&comm_manager_lock);

        if (pkt.type == client_halt) break;

        std::string metadata = std::string("(") + std::to_string(pkt.type) + std::string(",") 
                                                + std::to_string(pkt.seqn) + std::string(",") 
                                                + std::to_string(pkt.timestamp) + std::string(")");
        std::cout << "Message " << metadata << " from user " << username << ": " << pkt.payload << std::endl;

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