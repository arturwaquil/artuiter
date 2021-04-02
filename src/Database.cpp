#include "../include/Database.hpp"

#include <iostream>
#include <fstream>

// Save list of profiles (map) to file
void json_from_profiles(std::map<std::string, Profile> profiles)
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

// Get list of profiles (map) from file
std::map<std::string, Profile> profiles_from_json()
{
    // Read json structure from file
    std::ifstream file("/home/artur/Documents/ufrgs/20-2/sisop2/trabalho/artuiter/data/database.json");
    nlohmann::json j;
    file >> j;
    file.close();

    // Create profiles map from JSON
    std::map<std::string, Profile> profiles;
    for (auto item : j)
    {
        std::string username = item["username"].get<std::string>();
        auto followers = item["followers"].get<std::list<std::string>>();
        profiles.emplace(item["username"], Profile(username, followers));
    }

    return profiles;
}
