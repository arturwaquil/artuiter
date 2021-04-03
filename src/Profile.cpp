#include "../include/Profile.hpp"

Profile::Profile(std::string _name)
{
    name = _name;
    followers = std::list<std::string>();
}

Profile::Profile(std::string _name, std::list<std::string> _followers)
{
    name = _name;
    followers = _followers;
}

bool search_by_username(std::map<std::string, Profile> profiles, std::string username)
{
    auto it = profiles.find(username);
    return it != profiles.end();
}

bool is_follower(std::map<std::string, Profile> profiles, std::string follower, std::string followed)
{
    for (std::string it : profiles.at(followed).followers)
    {
        if (it == follower) return true;
    }
    return false;
}