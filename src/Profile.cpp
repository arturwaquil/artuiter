#include "../include/Profile.hpp"

#include <iostream>
#include <fstream>

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

    // Init the counting-semaphores map to control the number of connections of each user (and keep it <= 2)
    for (std::pair<const std::string, Profile> item : profiles)
    {
        std::string username = std::string(item.first);

        // Create new semaphore allowing two connections
        sem_t* semaphore = (sem_t*) malloc(sizeof(semaphore));
        sem_init(semaphore, 0, 2);

        connections_limit_semaphore_map.emplace(username, semaphore);
    }
}

ProfileManager::~ProfileManager()
{
    // Free memory from malloc-ed semaphores
    for (std::pair<const std::string, sem_t *> item : connections_limit_semaphore_map)
    {
        sem_t* semaphore = item.second;
        sem_destroy(semaphore);
    }

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
        profiles.emplace(username, Profile(username, followers));
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
    new_user(username, std::list<std::string>());
}

void ProfileManager::new_user(std::string username, std::list<std::string> followers)
{
    // Add to profiles map
    profiles.emplace(username, Profile(username, followers));

    // Add to semaphores map
    sem_t* semaphore = (sem_t*) malloc(sizeof(semaphore));
    sem_init(semaphore, 0, 2);
    connections_limit_semaphore_map.emplace(username, semaphore);
}

void ProfileManager::add_follower(std::string follower, std::string followed)
{
    profiles.at(followed).followers.push_back(follower);
}

bool ProfileManager::trywait_semaphore(std::string username)
{
    return sem_trywait(connections_limit_semaphore_map[username]) != -1;
}

void ProfileManager::post_semaphore(std::string username)
{
    sem_post(connections_limit_semaphore_map[username]);
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