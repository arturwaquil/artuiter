#ifndef PROFILE_HPP
#define PROFILE_HPP

#include <list>
#include <map>
#include <string>

class Profile
{
    public:
        Profile(std::string _name);
        Profile(std::string _name, std::list<std::string> _followers);

        std::string name;
        std::list<std::string> followers;   // list of strings or list of profile pointers?
        // TODO: add list of sent notifications (by this profile)
        // TODO: add list of pending notifications (to this profile)

};

bool search_by_username(std::map<std::string, Profile> profiles, std::string username);
bool is_follower(std::map<std::string, Profile> profiles, std::string follower, std::string followed);

#endif