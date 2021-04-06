#include "../include/Profile.hpp"
#include "../include/ServerComm.hpp"
#include "../include/Signal.hpp"

#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <regex>

#include <unistd.h>

ServerComm comm_manager;
ProfileManager profile_manager;

// Map of usernames indexed by sockets pair
std::map<skt_pair, std::string> usernames_map;

bool server_quit = false;               // signal flag
std::map<skt_pair, bool> quit_flags;    // Map of quit flags for each session
std::mutex quit_flags_mutex;            // Mutex for quit_flags map

// Fetch element from map while respecting the mutex
bool get_quit_flag(skt_pair sockets)
{
    quit_flags_mutex.lock();
    bool b = quit_flags.at(sockets);
    quit_flags_mutex.unlock();
    return b;
}

// Set element in map while respecting the mutex
void set_quit_flag(skt_pair sockets, bool b)
{
    quit_flags_mutex.lock();
    quit_flags.at(sockets) = b;
    quit_flags_mutex.unlock();
}

void sig_int_handler(int signum)
{
    // Set quit flag so that the comm manager exits the accept() waiting.
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
        skt_pair sockets = comm_manager._accept();

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
    skt_pair sockets = *((skt_pair*)args);
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
        quit_flags.erase(sockets);
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
    if (!profile_manager.create_session(username, sockets))
    {
        pkt = create_packet(reply_login, 0, 0, "FAILED");
        comm_manager.write_pkt(cmd_sockfd, pkt);
        close(cmd_sockfd);
        close(ntf_sockfd);
        quit_flags.erase(sockets);
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

    // Free a spot in the connection-count semaphore
    profile_manager.end_session(username, sockets);

    quit_flags.erase(sockets);

    std::cout << "User " << username << " logged out." << std::endl;

    return NULL;
}

void* run_client_cmd_thread(void* args)
{
    skt_pair sockets = *((skt_pair*)args);
    int cmd_sockfd = sockets.first;     // The commands socket

    std::string username = usernames_map.at(sockets);

    packet pkt;

    while(!get_quit_flag(sockets) && !server_quit)
    {
        // Blocking read, wait for packet
        comm_manager.read_pkt(cmd_sockfd, &pkt);

        // If client sent command to exit, set quit flag for this session
        // so that the notif thread can exit too
        if (pkt.type == client_halt)
        {
            set_quit_flag(sockets, true);
            break;
        }

        // If the server wants to exit, break (this is the last step, see comment on server notif thread)
        if (pkt.type == server_halt) break;

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
    skt_pair sockets = *((skt_pair*)args);
    int ntf_sockfd = sockets.second;    // The notifications socket

    std::string username = usernames_map.at(sockets);

    packet pkt;

    while(!get_quit_flag(sockets) && !server_quit)
    {
        // Get message from first notification in queue, if there is one. Empty string is returned
        // otherwise, so that the busy waiting is done here
        std::string info = profile_manager.consume_notification(username, sockets);

        // If the cmd thread sets the quit flag, the ntf thread notifies the ntf thread of the client
        if (get_quit_flag(sockets))
        {
            pkt = create_packet(client_halt, 0, 0, std::string());
            comm_manager.write_pkt(ntf_sockfd, pkt);
        }

        // If server receives SIGINT (ctrl+c), the sessions' notif threads send this halting command for the
        // clients' notif threads, which notify the clients' cmd threads, which notify the server cmd threads
        if (server_quit)
        {
            pkt = create_packet(server_halt, 0, 0, std::string());
            comm_manager.write_pkt(ntf_sockfd, pkt);
        }

        // If didn't quit, and if not busy waiting, send notification to client
        if (info != std::string())
        {
            pkt = create_packet(notification, 0, 0, info);
            comm_manager.write_pkt(ntf_sockfd, pkt);
        }
    }

    return NULL;
}