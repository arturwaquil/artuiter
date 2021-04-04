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

std::atomic<bool> quit(false);    // signal flag

ServerComm comm_manager;
ProfileManager profile_manager;

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

    // Set sig_int_handler() as the handler for signal SIGINT (ctrl+c)
    set_signal_action(SIGINT, sig_int_handler);

    std::cout << "Server initialized." << std::endl;

    // Run the server until SIGINT
    while(!quit.load())
    {
        // Accept first two pending connections
        std::pair<int,int> sockets = comm_manager._accept();

        client_thread_params ctp = create_client_thread_params(std::string(), sockets);

        if (quit.load()) break;

        // Launch new thread to deal with client
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, run_client_threads, &ctp);
        threads.push_back(client_thread);
    }

    // Wait for joining all threads
    for (pthread_t t : threads) pthread_join(t, NULL);

    std::cout << "\nExiting..." << std::endl;

    return 0;
}

void* run_client_threads(void* args)
{
    client_thread_params ctp = *((client_thread_params*)args);
    int cmd_sockfd = ctp.sockets.first;
    int ntf_sockfd = ctp.sockets.second;

    packet pkt;

    // Read login-attempt packet from client. Assert that it is of type "login"
    comm_manager.read_pkt(cmd_sockfd, &pkt);
    if (pkt.type != login)
    {
        pkt = create_packet(reply_login, 0, 0, "FAILED");
        comm_manager.write_pkt(cmd_sockfd, pkt);
        close(cmd_sockfd);
        return NULL;
    }

    std::string username = std::string(pkt.payload);

    // Create new user if it doesn't exist
    if (!profile_manager.user_exists(username)) profile_manager.new_user(username);

    // If user is already connected twice, send negative reply to client
    if (!profile_manager.trywait_semaphore(username))
    {
        pkt = create_packet(reply_login, 0, 0, "FAILED");
        comm_manager.write_pkt(cmd_sockfd, pkt);
        close(cmd_sockfd);
        return NULL;
    }

    // Update args so that the client threads receive the username
    ((client_thread_params*)args)->username = username;

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

    return NULL;
}

void* run_client_cmd_thread(void* args)
{
    client_thread_params ctp = *((client_thread_params*)args);
    std::string username = ctp.username;
    int cmd_sockfd = ctp.sockets.first;     // The commands socket

    packet pkt;

    while(true)
    {
        comm_manager.read_pkt(cmd_sockfd, &pkt);

        if (pkt.type == client_halt) break;

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
                
                reply = std::string("Message received!");
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
                reply = std::string("Unknown command.");
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
    client_thread_params ctp = *((client_thread_params*)args);
    std::string username = ctp.username;
    int ntf_sockfd = ctp.sockets.second;    // The notifications socket

    packet pkt;

    // TODO: how to exit?
    while(true)
    {
        // listen to profiles' notifications lists

        Notification n = profile_manager.consume_notification(username);

        pkt = create_packet(notification, 0, 0, n.author + ": " + n.message);
        comm_manager.write_pkt(ntf_sockfd, pkt);
    }

    return NULL;
}