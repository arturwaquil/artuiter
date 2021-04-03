#include "../include/Profile.hpp"

#include <iostream>
#include <fstream>

Profile::Profile(std::string _name)
{
    Profile(_name, std::list<std::string>());
}

Profile::Profile(std::string _name, std::list<std::string> _followers)
{
    name = _name;
    followers = _followers;
    sem_init(&sem_connections_limit, 0, 2);
}

void Profile::print_info()
{
    std::cout << "Username: " << name << ". ";
    std::cout << "Followers: ";
    for (std::string user : followers) std::cout << user << " ";
    std::cout << std::endl;
}

ProfileManager::ProfileManager()
{
    read_from_database();
}

ProfileManager::~ProfileManager()
{
    write_to_database();
}

void ProfileManager::read_from_database()
{
    // Read JSON structure from file
    std::ifstream file("/home/artur/Documents/ufrgs/20-2/sisop2/trabalho/artuiter/data/database.json");
    nlohmann::json j;
    file >> j;
    file.close();

    // Fill profiles map with info from JSON
    for (auto item : j)
    {
        std::string username = item["username"].get<std::string>();
        auto followers = item["followers"].get<std::list<std::string>>();
        new_user(username, followers);
    }
}

void ProfileManager::write_to_database()
{
    nlohmann::json j;

    // Convert from list of Profiles to JSON
    for (auto item : profiles)
    {
        std::string username = item.first;
        Profile profile = item.second;

        j[username] = {
            {"username", username},
            {"followers", profile.followers}
        };
    }

    // Save to file
    std::ofstream file("/home/artur/Documents/ufrgs/20-2/sisop2/trabalho/artuiter/data/database.json");
    file << j;
}

void ProfileManager::new_user(std::string username)
{
    profiles.emplace(username, Profile(username));
}

void ProfileManager::new_user(std::string username, std::list<std::string> followers)
{
    profiles.emplace(username, Profile(username, followers));
}

void ProfileManager::add_follower(std::string follower, std::string followed)
{
    profiles.at(followed).followers.push_back(follower);
}

bool ProfileManager::trywait_semaphore(std::string username)
{
    return sem_trywait(&profiles.at(username).sem_connections_limit) != -1;
}

void ProfileManager::post_semaphore(std::string username)
{
    sem_post(&profiles.at(username).sem_connections_limit);
}

bool ProfileManager::user_exists(std::string username)
{
    auto it = profiles.find(username);
    return it != profiles.end();
}

bool ProfileManager::is_follower(std::string follower, std::string followed)
{
    for (std::string it : profiles.at(followed).followers)
    {
        if (it == follower) return true;
    }
    return false;
}

void ProfileManager::print_profiles()
{
    for (auto item : profiles) item.second.print_info();
}