#include "../include/Database.hpp"
#include "../include/Profile.hpp"
#include "../include/ServerComm.hpp"
#include "../include/Signal.hpp"
#include "../include/UI.hpp"

#include <atomic>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <regex>
#include <semaphore.h>

#include <unistd.h>

std::atomic<bool> quit(false);    // signal flag

ServerComm comm_manager;
std::map<std::string, Profile> profiles;    // Store all profiles, indexed by username
std::map<std::string, sem_t*> connections_limit_semaphore_map; // One semaphore for each user
UI ui;

void sig_int_handler(int signum)
{
    // TODO: notify all clients that server is down (i.e. send server_halt message)

    // Update profile/followers info in database
    json_from_profiles(profiles);

    // Set quit flag so that the comm manager exits the accept waiting.
    comm_manager.set_quit();

    // Free memory from malloc-ed semaphores
    for (std::pair<const std::string, sem_t *> item : connections_limit_semaphore_map)
    {
        sem_t* semaphore = item.second;
        sem_destroy(semaphore);
    }

    quit.store(true);
}

void* run_client_threads(void* args);
void* run_client_cmd_thread(void* args);
void* run_client_notif_thread(void* args);

int main()
{
    std::cout << "Initializing server..." << std::endl;

    std::list<pthread_t> threads = std::list<pthread_t>();

    pthread_mutex_t comm_manager_lock;
    pthread_mutex_init(&comm_manager_lock, NULL);

    // Set sig_int_handler() as the handler for signal SIGINT (ctrl+c)
    set_signal_action(SIGINT, sig_int_handler);

    // Fetch profile/followers info from database
    profiles = profiles_from_json();

    // Set UI to server comm manager, as it is declared globally
    comm_manager.set_ui(ui);

    // Init the counting-semaphores map to control the number of connections of each user (and keep it <= 2)
    for (std::pair<const std::string, Profile> item : profiles)
    {
        std::string username = std::string(item.first);

        // Create new semaphore allowing two connections
        sem_t* semaphore = (sem_t*) malloc(sizeof(semaphore));
        sem_init(semaphore, 0, 2);

        connections_limit_semaphore_map.emplace(username, semaphore);
    }

    ui.write("Server initialized.");

    // Run the server until SIGINT
    while(!quit.load())
    {
        // Accept first pending connection
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

    ui.write("\nExiting...");

    return 0;
}

void* run_client_threads(void* args)
{
    client_thread_params ctp = *((client_thread_params*)args);
    int sockfd = ctp.new_sockfd;
    pthread_mutex_t comm_manager_lock = *ctp.comm_manager_lock;

    packet pkt;
    sem_t* semaphore;

    // Read login-attempt packet from client. Assert that it is of type "login"
    pthread_mutex_lock(&comm_manager_lock);
    comm_manager.read_pkt(sockfd, &pkt);
    pthread_mutex_unlock(&comm_manager_lock);
    if (pkt.type != login)
    {
        pkt = create_packet(reply_login, 0, 0, "FAILED");
        pthread_mutex_lock(&comm_manager_lock);
        comm_manager.write_pkt(sockfd, pkt);
        pthread_mutex_unlock(&comm_manager_lock);
        close(ctp.new_sockfd);
        return NULL;
    }

    std::string username = std::string(pkt.payload);

    // If user doesn't exist, create new profile (and new semaphore)
    if (!search_by_username(profiles, username))
    {
        profiles.emplace(username, Profile(username));

        sem_t* semaphore = (sem_t*) malloc(sizeof(semaphore));
        sem_init(semaphore, 0, 2);
        connections_limit_semaphore_map.emplace(username, semaphore);
    }

    // If user is already connected twice, send negative reply to client
    semaphore = connections_limit_semaphore_map[username];
    if (sem_trywait(semaphore) == -1)
    {
        pkt = create_packet(reply_login, 0, 0, "FAILED");
        pthread_mutex_lock(&comm_manager_lock);
        comm_manager.write_pkt(sockfd, pkt);
        pthread_mutex_unlock(&comm_manager_lock);
        close(ctp.new_sockfd);
        return NULL;
    }

    // Update args so that the client threads receive the username
    ((client_thread_params*)args)->username = username;

    // Send positive reply
    pkt = create_packet(reply_login, 0, 0, "OK");
    pthread_mutex_lock(&comm_manager_lock);
    comm_manager.write_pkt(sockfd, pkt);
    pthread_mutex_unlock(&comm_manager_lock);
    ui.write("User " + username + " logged in.");

    // Run the two client threads (for commands and notifications)
    pthread_t client_cmd_thread;
    // pthread_t client_notif_thread;
    pthread_create(&client_cmd_thread, NULL, run_client_cmd_thread, args);
    // pthread_create(&client_notif_thread, NULL, run_client_notif_thread, args);

    pthread_join(client_cmd_thread, NULL);
    // pthread_join(client_notif_thread, NULL);

    // When both threads are joined, close the dedicated socket
    close(ctp.new_sockfd);

    ui.write("User " + username + " logged out.");

    // Free a spot in the connection-count semaphore
    sem_post(semaphore);

    return NULL;
}

void* run_client_cmd_thread(void* args)
{
    client_thread_params ctp = *((client_thread_params*)args);
    std::string username = ctp.username;
    int sockfd = ctp.new_sockfd;
    pthread_mutex_t comm_manager_lock = *ctp.comm_manager_lock;

    packet pkt;

    bool exit = false;

    while(!exit)
    {
        // TODO: this mutex logic seems wrong...
        pthread_mutex_lock(&comm_manager_lock);
        comm_manager.read_pkt(sockfd, &pkt);
        pthread_mutex_unlock(&comm_manager_lock);

        if (pkt.type == client_halt) break;

        if (pkt.type == command)
        {
            std::string full_message = std::string(pkt.payload);
            std::string message = full_message.substr(full_message.find(" ")+1);

            // The exit command is handled on the client side, the same way as the SIGINT signal.
            // This way it's easier to disconnect both related server threads.

            if (std::regex_match(full_message, std::regex("(SEND|send) +.+")))
            {
                // TODO: Write message to profiles structure
                ui.write("Message (" + std::to_string(pkt.type) + "," + std::to_string(pkt.seqn) + ","
                    + std::to_string(pkt.timestamp) + ") from user " + username + ": " + message);

                // TODO: Positive reply
                pkt = create_packet(reply_command, 0, 0, std::string("Message received!"));
            }
            else if (std::regex_match(full_message, std::regex("(FOLLOW|follow) +@[a-z]*")))
            {
                // TODO: Add username to desired profile's followers list
                ui.write("Message (" + std::to_string(pkt.type) + "," + std::to_string(pkt.seqn) + ","
                    + std::to_string(pkt.timestamp) + ") from user " + username + ": " + message);

                // TODO: Positive reply or negative reply
                std::string reply = std::string("Followed user ") + message + std::string("!");
                pkt = create_packet(reply_command, 0, 0, reply);
            }
            else
            {
                // Fail message as reply
                pkt = create_packet(reply_command, 0, 0, std::string("Unknown command."));
            }

            // Send reply to client
            pthread_mutex_lock(&comm_manager_lock);
            comm_manager.write_pkt(sockfd, pkt);
            pthread_mutex_unlock(&comm_manager_lock);
        }

    }

    return NULL;
}

void* run_client_notif_thread(void* args)
{
    return NULL;
}