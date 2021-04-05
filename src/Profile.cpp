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

    // TODO: (SEM)
    // sem_connections_limit = 2;
    // available_connections = 2;
}

// TODO: (SEM) had implemented a semaphore manually because an error was occurring
//      when using the sem_t. Will keep this here for a while just in case
// bool Profile::_sem_try_wait()
// {
//     // Acquire the semaphore only if not yet in the connections limit
//     if (available_connections > 0)
//     {
//         available_connections--;
//         return true;
//     }
//     else
//     {
//         return false;
//     }
// }

// void Profile::_sem_post()
// {
//     // Release the semaphore
//     if (available_connections < sem_connections_limit) available_connections++;
// }

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

void ProfileManager::send_notification(std::string message, std::string username)
{
    // Create new notification in username's sent_notifications list
    int pending = profiles.at(username).followers.size();
    Notification n = Notification(username, message, pending);
    pthread_mutex_lock(&profiles.at(username).mutex_sent_notifications);
    profiles.at(username).sent_notifications.emplace(n.id, n);
    pthread_mutex_unlock(&profiles.at(username).mutex_sent_notifications);

    // Add notification reference to every follower's pending_notifications list
    // TODO: need to synchronize??
    for (std::string follower : profiles.at(username).followers)
    {
        // pthread_mutex_lock(&profiles.at(follower).mutex_pending_notifications);
        profiles.at(follower).pending_notifications.push_back(std::make_pair(username, n.id));
        // pthread_mutex_unlock(&profiles.at(follower).mutex_pending_notifications);
    }
}

std::string ProfileManager::consume_notification(std::string username)
{
    // TODO: deal with multiple sessions of the same user

    // Return "empty notification". The busy waiting is done in the server thread directly
    if (profiles.at(username).pending_notifications.size() == 0) return std::string();

    // Retrieve first notification info from username's pending list
    std::pair<std::string, uint16_t> notif_info = profiles.at(username).pending_notifications.front();
    profiles.at(username).pending_notifications.pop_front();

    std::string author = notif_info.first;
    uint16_t id = notif_info.second;

    // Get notification from author and id
    Notification* n = &profiles.at(author).sent_notifications.at(id);

    // Decrement number of pending clients to send
    // TODO: should this be atomic?
    n->pending--;
    
    return n->author + ": " + n->message;
}

bool ProfileManager::trywait_semaphore(std::string username)
{
    return sem_trywait(&profiles.at(username).sem_connections_limit) != -1;
    // TODO: (SEM)
    // return profiles.at(username)._sem_try_wait();
}

void ProfileManager::post_semaphore(std::string username)
{
    sem_post(&profiles.at(username).sem_connections_limit);
    // TODO: (SEM)
    // profiles.at(username)._sem_post();
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