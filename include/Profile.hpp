#ifndef PROFILE_HPP
#define PROFILE_HPP

#include "Notification.hpp"

#include <nlohmann/json.hpp>

#include <list>
#include <map>
#include <string>

#include <semaphore.h>

class Profile
{
    public:
        Profile(std::string _name);
        Profile(std::string _name, std::list<std::string> _followers);

        std::string name;
        std::list<std::string> followers;

        // Tweets by this profile waiting to be sent to all followers
        std::map<uint16_t, Notification> sent_notifications;

        // References to tweets by accounts this profile follows, waiting to be sent to it
        std::list<std::pair<std::string, uint16_t>> pending_notifications;

        sem_t sem_connections_limit;
        // TODO: (SEM)
        // bool _sem_try_wait();
        // void _sem_post();
        
        pthread_mutex_t mutex_sent_notifications;
        pthread_mutex_t mutex_pending_notifications;

        void print_info();
    
    // TODO: (SEM)
    // private:
    //     int sem_connections_limit;
    //     int available_connections;


};

class ProfileManager
{
    public:
        ProfileManager();
        ~ProfileManager();

        void read_from_database();
        void write_to_database();

        void new_user(std::string username);
        void new_user(std::string username, std::list<std::string> followers);

        void add_follower(std::string follower, std::string followed);
        void send_notification(std::string message, std::string username);
        std::string consume_notification(std::string username);

        bool trywait_semaphore(std::string username);
        void post_semaphore(std::string username);

        bool user_exists(std::string username);
        bool is_follower(std::string follower, std::string followed);

        void print_profiles();
    
    private:
        std::map<std::string, Profile> profiles;    // Store all profiles, indexed by username

};

#endif