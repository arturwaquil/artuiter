#ifndef PROFILE_HPP
#define PROFILE_HPP

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
        std::list<std::string> followers;   // list of strings or list of profile pointers?
        // TODO: add list of sent notifications (by this profile)
        // TODO: add list of pending notifications (to this profile)

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
        void new_user(std::string username, std::list<std::string> followers);

        void add_follower(std::string follower, std::string followed);

        bool trywait_semaphore(std::string username);
        void post_semaphore(std::string username);

        bool user_exists(std::string username);
        bool is_follower(std::string follower, std::string followed);

        void print_profiles();
    
    private:
        std::map<std::string, Profile> profiles;    // Store all profiles, indexed by username
        std::map<std::string, sem_t*> connections_limit_semaphore_map; // One semaphore for each user

};

#endif