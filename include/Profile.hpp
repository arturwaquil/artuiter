#ifndef PROFILE_HPP
#define PROFILE_HPP

#include "Notification.hpp"
#include "Typedefs.hpp"

#include <nlohmann/json.hpp>

#include <list>
#include <map>
#include <string>

#include <semaphore.h>

class Profile
{
    public:
        Profile(std::string _name, str_list _followers);

        ~Profile();

        bool create_session(skt_pair sockets);
        void end_session(skt_pair sockets);

        std::string name;
        str_list followers;

        std::list<skt_pair> sessions;

        // Tweets by this profile waiting to be sent to all followers
        std::map<uint16_t, Notification> sent_notifications;

        // List of tweets by followed accounts, waiting to be sent to the user.
        // Each tweet is referenced by a pair < author, id >
        std::list<notif_info> pending_notifications;

        // Each session has a list of pending notifications
        std::map<skt_pair, str_list> sessions_pending_notifications;

        sem_t sem_connections_limit;

        pthread_mutex_t mutex_followers;
        pthread_mutex_t mutex_sessions;
        pthread_mutex_t mutex_sent_notifications;
        pthread_mutex_t mutex_pending_notifications;
        pthread_mutex_t mutex_sessions_pending_notifications;

        void print_info();
};

class ProfileManager
{
    public:
        ProfileManager();
        ~ProfileManager();

        void read_from_database();
        void write_to_database();

        void new_user(std::string username);
        void new_user(std::string username, str_list followers);

        void add_follower(std::string follower, std::string followed);
        void remove_follower(std::string follower, std::string followed);
        str_list list_followers(std::string username);
        str_list list_following(std::string username);
        void send_notification(std::string message, std::string username);
        std::string consume_notification(std::string username, skt_pair sockets);

        void update_sessions_pending_lists(std::string username);

        bool create_session(std::string username, skt_pair sockets);
        void end_session(std::string username, skt_pair sockets);

        bool user_exists(std::string username);
        bool is_follower(std::string follower, std::string followed);

        void print_profiles();
    
    private:
        std::string database_location;
        std::map<std::string, Profile> profiles;    // Store all profiles, indexed by username
        pthread_mutex_t mutex_profiles;

};

#endif