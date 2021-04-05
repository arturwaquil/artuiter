#include "../include/Profile.hpp"
#include "../include/ServerComm.hpp"
#include "../include/Signal.hpp"

#include <atomic>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <regex>

#include <unistd.h>

bool server_quit = false;    // signal flag

ServerComm comm_manager;
ProfileManager profile_manager;

// Map of quit flags indexed by each client's sockets pair
std::map<std::pair<int,int>, bool> quit_flags;

// Map of usernames indexed by sockets pair
std::map<std::pair<int,int>, std::string> usernames_map;

void sig_int_handler(int signum)
{
    // TODO: notify all clients that server is down (i.e. send server_halt message)

    // Set quit flag so that the comm manager exits the accept waiting.
    comm_manager.set_quit();

    server_quit = true;
}

void* run_client_threads(void* args);
void* run_client_cmd_thread(void* args);
void* run_client_notif_thread(void* args);

int main()
{
    std::cout << "Initializing server..." << std::endl;

    std::list<pthread_t> threads = std::list<pthread_t>();

    // Set sig_int_handler() as the handler for signal SIGINT (ctrl+c)
    set_signal_action(SIGINT, sig_int_handler);

    std::cout << "Server initialized." << std::endl;

    // Run the server until SIGINT
    while(!server_quit)
    {
        // Accept first two pending connections
        std::pair<int,int> sockets = comm_manager._accept();

        if (server_quit) break;

        quit_flags.emplace(sockets, false);

        // Launch new thread to deal with client
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, run_client_threads, &sockets);
        threads.push_back(client_thread);
    }

    // Wait for joining all threads
    for (pthread_t t : threads) pthread_join(t, NULL);

    std::cout << "\nExiting..." << std::endl;

    return 0;
}

void* run_client_threads(void* args)
{
    std::pair<int,int> sockets = *((std::pair<int,int>*)args);
    int cmd_sockfd = sockets.first;
    int ntf_sockfd = sockets.second;

    packet pkt;

    // Read login-attempt packet from client. Assert that it is of type "login"
    comm_manager.read_pkt(cmd_sockfd, &pkt);
    if (pkt.type != login)
    {
        pkt = create_packet(reply_login, 0, 0, "FAILED");
        comm_manager.write_pkt(cmd_sockfd, pkt);
        close(cmd_sockfd);
        close(ntf_sockfd);
        return NULL;
    }

    std::string username = std::string(pkt.payload);

    // Create new user if it doesn't exist
    if (!profile_manager.user_exists(username))
    {
        std::cout << "Creating new user " << username << std::endl;
        profile_manager.new_user(username);
    }

    // If user is already connected twice, send negative reply to client
    // TODO: why not working anymore????
    if (!profile_manager.trywait_semaphore(username))
    {
        pkt = create_packet(reply_login, 0, 0, "FAILED");
        comm_manager.write_pkt(cmd_sockfd, pkt);
        close(cmd_sockfd);
        close(ntf_sockfd);
        return NULL;
    }

    // Add username to map indexed by sockets, so that the cmd and ntf threads can access it
    usernames_map.emplace(sockets, username);

    // Send positive reply
    pkt = create_packet(reply_login, 0, 0, "OK");
    comm_manager.write_pkt(cmd_sockfd, pkt);
    std::cout << "User " << username << " logged in." << std::endl;

    // Run the two client threads (for commands and notifications)
    pthread_t client_cmd_thread, client_notif_thread;
    pthread_create(&client_cmd_thread, NULL, run_client_cmd_thread, args);
    pthread_create(&client_notif_thread, NULL, run_client_notif_thread, args);

    pthread_join(client_cmd_thread, NULL);
    pthread_join(client_notif_thread, NULL);

    // When both threads are joined, close the dedicated sockets
    close(cmd_sockfd);
    close(ntf_sockfd);

    std::cout << "User " << username << " logged out." << std::endl;

    // Free a spot in the connection-count semaphore
    profile_manager.post_semaphore(username);

    quit_flags.erase(sockets);

    return NULL;
}

void* run_client_cmd_thread(void* args)
{
    std::pair<int,int> sockets = *((std::pair<int,int>*)args);
    int cmd_sockfd = sockets.first;     // The commands socket

    std::string username = usernames_map.at(sockets);

    packet pkt;

    while(!quit_flags.at(sockets))
    {
        comm_manager.read_pkt(cmd_sockfd, &pkt);

        if (pkt.type == client_halt)
        {
            quit_flags[sockets] = true;
            break;
        }

        if (pkt.type == command)
        {
            std::string full_message = std::string(pkt.payload);

            // The exit command is handled on the client side, the same way as the SIGINT signal.
            // This way it's easier to disconnect both related server threads.

            std::string reply;

            if (std::regex_match(full_message, std::regex("(SEND|send) .+")))
            {
                // Post notification (add it to user's sent list and followers' pending lists)
                std::string message = full_message.substr(full_message.find(" ")+1);
                profile_manager.send_notification(message, username);

                std::cout << "Message from " << username << ": " << message << std::endl;
                
                reply = std::string("Sent message \"") + message + std::string("\".");
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
                    std::cout << "User " << username << " followed user " << target_user << std::endl;
                    reply = std::string("Followed user ") + target_user + std::string("!");
                }

            }
            else
            {
                std::string invalid_command;
                if (full_message.size() > 10) invalid_command = full_message.substr(0,10) + std::string("...");
                else invalid_command = full_message;

                reply = invalid_command + std::string(" is not a valid command.");
            }

            // Send reply to client
            pkt = create_packet(reply_command, 0, 0, reply);
            comm_manager.write_pkt(cmd_sockfd, pkt);
        }

    }

    return NULL;
}

void* run_client_notif_thread(void* args)
{
    std::pair<int,int> sockets = *((std::pair<int,int>*)args);
    int ntf_sockfd = sockets.second;    // The notifications socket

    std::string username = usernames_map.at(sockets);

    packet pkt;

    while(!quit_flags.at(sockets))
    {
        // Listen to profiles' notifications lists

        // TODO: deal with multiple sessions of the same user

        std::string info = profile_manager.consume_notification(username);

        // Ignore invalid notification that is sent to do the busy waiting in this function
        if (info != std::string())
        {
            pkt = create_packet(notification, 0, 0, info);
            comm_manager.write_pkt(ntf_sockfd, pkt);
        }

        // If the cmd thread sets the quit flag, the ntf thread notifies the ntf thread of the client
        if (quit_flags[sockets])
        {
            pkt = create_packet(client_halt, 0, 0, std::string());
            comm_manager.write_pkt(ntf_sockfd, pkt);
        }
    }

    return NULL;
}