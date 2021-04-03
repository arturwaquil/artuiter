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

#include <unistd.h>

std::atomic<bool> quit(false);    // signal flag

ServerComm comm_manager;
ProfileManager profile_manager;
UI ui;

void sig_int_handler(int signum)
{
    // TODO: notify all clients that server is down (i.e. send server_halt message)

    // Set quit flag so that the comm manager exits the accept waiting.
    comm_manager.set_quit();

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

    // Set UI to server comm manager, as it is declared globally
    comm_manager.set_ui(ui);

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

    // Create new user if it doesn't exist
    if (!profile_manager.user_exists(username)) profile_manager.new_user(username);

    // If user is already connected twice, send negative reply to client
    if (!profile_manager.trywait_semaphore(username))
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
    profile_manager.post_semaphore(username);

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

            // The exit command is handled on the client side, the same way as the SIGINT signal.
            // This way it's easier to disconnect both related server threads.

            std::string reply;

            if (std::regex_match(full_message, std::regex("(SEND|send) .+")))
            {
                // TODO: Write message to profiles structure
                std::string message = full_message.substr(full_message.find(" ")+1);
                ui.write("Message (" + std::to_string(pkt.type) + "," + std::to_string(pkt.seqn) + ","
                    + std::to_string(pkt.timestamp) + ") from user " + username + ": " + message);

                // TODO: Positive reply
                pkt = create_packet(reply_command, 0, 0, std::string("Message received!"));
            }
            else if (std::regex_match(full_message, std::regex("(FOLLOW|follow) @[a-z]*")))
            {
                // Add username to desired profile's followers list

                std::string target_user = full_message.substr(full_message.find(" ")+1);

                if (target_user == username)
                {
                    reply = std::string("You can't follow yourself...");
                }
                else if (!profile_manager.user_exists(target_user))
                {
                    reply = std::string("User " + target_user + " doesn't exist.");
                }
                else if (profile_manager.is_follower(username, target_user))
                {
                    reply = std::string("You already follow " + target_user + ".");
                }
                else
                {
                    // If target_user exists and is not equal to username,
                    // add username to target_user's followers list
                    profile_manager.add_follower(username, target_user);
                    ui.write("User " + username + " followed user " + target_user);
                    reply = std::string("Followed user ") + target_user + std::string("!");
                }

            }
            else
            {
                reply = std::string("Unknown command.");
            }

            // Send reply to client
            pkt = create_packet(reply_command, 0, 0, reply);
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