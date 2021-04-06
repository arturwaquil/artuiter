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

    pthread_mutex_init(&mutex_followers, NULL);
    pthread_mutex_init(&mutex_sessions, NULL);
    pthread_mutex_init(&mutex_sent_notifications, NULL);
    pthread_mutex_init(&mutex_pending_notifications, NULL);
    pthread_mutex_init(&mutex_sessions_pending_notifications, NULL);
}

Profile::~Profile()
{
    pthread_mutex_destroy(&mutex_followers);
    pthread_mutex_destroy(&mutex_sessions);
    pthread_mutex_destroy(&mutex_sent_notifications);
    pthread_mutex_destroy(&mutex_pending_notifications);
    pthread_mutex_destroy(&mutex_sessions_pending_notifications);
}

bool Profile::create_session(skt_pair sockets)
{
    // Already connected twice
    if (sem_trywait(&sem_connections_limit) == -1) return false;

    pthread_mutex_lock(&mutex_sessions);
    pthread_mutex_lock(&mutex_pending_notifications);

    sessions.push_back(sockets);
    sessions_pending_notifications.emplace(sockets, std::list<std::string>());

    pthread_mutex_unlock(&mutex_pending_notifications);
    pthread_mutex_unlock(&mutex_sessions);

    return true;
}

void Profile::end_session(skt_pair sockets)
{
    pthread_mutex_lock(&mutex_sessions);
    pthread_mutex_lock(&mutex_pending_notifications);

    sem_post(&sem_connections_limit);
    sessions.remove(sockets);
    sessions_pending_notifications.erase(sockets);

    pthread_mutex_unlock(&mutex_pending_notifications);
    pthread_mutex_unlock(&mutex_sessions);
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
    
    pthread_mutex_init(&mutex_profiles, NULL);
}

ProfileManager::~ProfileManager()
{
    write_to_database();

    pthread_mutex_destroy(&mutex_profiles);
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
    pthread_mutex_lock(&mutex_profiles);
    profiles.emplace(username, Profile(username));
    pthread_mutex_unlock(&mutex_profiles);
}

void ProfileManager::new_user(std::string username, std::list<std::string> followers)
{
    pthread_mutex_lock(&mutex_profiles);
    profiles.emplace(username, Profile(username, followers));
    pthread_mutex_unlock(&mutex_profiles);
}

void ProfileManager::add_follower(std::string follower, std::string followed)
{
    pthread_mutex_lock(&mutex_profiles);

    Profile* p = &profiles.at(followed);

    pthread_mutex_lock(&p->mutex_followers);
    profiles.at(followed).followers.push_back(follower);
    pthread_mutex_unlock(&p->mutex_followers);

    pthread_mutex_unlock(&mutex_profiles);
}

void ProfileManager::send_notification(std::string message, std::string username)
{
    pthread_mutex_lock(&mutex_profiles);

    Profile* p = &profiles.at(username);

    pthread_mutex_lock(&p->mutex_followers);

    // Create new notification in username's sent_notifications list
    Notification n = Notification(username, message, p->followers.size());
    pthread_mutex_lock(&p->mutex_sent_notifications);
    p->sent_notifications.emplace(n.id, n);
    pthread_mutex_unlock(&p->mutex_sent_notifications);

    // Add notification reference to every follower's pending_notifications list
    for (std::string follower : p->followers)
    {
        Profile* f = &profiles.at(follower);
        pthread_mutex_lock(&f->mutex_pending_notifications);
        f->pending_notifications.push_back(std::make_pair(username, n.id));
        pthread_mutex_unlock(&f->mutex_pending_notifications);
    }

    pthread_mutex_unlock(&p->mutex_followers);

    pthread_mutex_unlock(&mutex_profiles);
}

std::string ProfileManager::consume_notification(std::string username, skt_pair sockets)
{
    // Update each session's pending list according to the user's pending list
    update_sessions_pending_lists(username);

    pthread_mutex_lock(&mutex_profiles);
    Profile* p = &profiles.at(username);
    pthread_mutex_lock(&p->mutex_sessions_pending_notifications);

    std::list<std::string>* session_pending_list = &p->sessions_pending_notifications.at(sockets);

    std::string ret;

    if (session_pending_list->size() == 0)
    {
        // Return "empty notification". The busy waiting is done in the server thread directly
        ret =  std::string();
    }
    else
    {
        // Consume an item from the session's pending list
        ret = session_pending_list->front();
        session_pending_list->pop_front();
    }

    pthread_mutex_unlock(&p->mutex_sessions_pending_notifications);
    pthread_mutex_unlock(&mutex_profiles);
    
    return ret;
}

void ProfileManager::update_sessions_pending_lists(std::string username)
{
    pthread_mutex_lock(&mutex_profiles);

    Profile* p = &profiles.at(username);

    pthread_mutex_lock(&p->mutex_pending_notifications);
    pthread_mutex_lock(&p->mutex_sessions);

    if (p->sessions.size() != 0)
    {
        for (auto notif_info : p->pending_notifications)
        {
            std::string author = notif_info.first;
            uint16_t id = notif_info.second;

            Profile* a = &profiles.at(author);

            pthread_mutex_lock(&a->mutex_sent_notifications);

            // Get notification from author and id
            Notification* n = &a->sent_notifications.at(id);

            // Insert notification (in fact, the string "@author: message") in each session's list
            std::string message = n->author + std::string(": ") + n->message;
            pthread_mutex_lock(&p->mutex_sessions_pending_notifications);
            for (skt_pair s : p->sessions) p->sessions_pending_notifications.at(s).push_back(message);
            pthread_mutex_unlock(&p->mutex_sessions_pending_notifications);

            // Decrement number of pending clients to send
            n->pending--;
            if (n->pending < 1) a->sent_notifications.erase(id);
            
            pthread_mutex_unlock(&a->mutex_sent_notifications);

        }
        p->pending_notifications.clear();
    }

    pthread_mutex_unlock(&p->mutex_sessions);
    pthread_mutex_unlock(&p->mutex_pending_notifications);

    pthread_mutex_unlock(&mutex_profiles);
}

bool ProfileManager::create_session(std::string username, skt_pair sockets)
{
    pthread_mutex_lock(&mutex_profiles);
    bool b = profiles.at(username).create_session(sockets);
    pthread_mutex_unlock(&mutex_profiles);
    return b;
}

void ProfileManager::end_session(std::string username, skt_pair sockets)
{
    pthread_mutex_lock(&mutex_profiles);
    profiles.at(username).end_session(sockets);
    pthread_mutex_unlock(&mutex_profiles);
}

bool ProfileManager::user_exists(std::string username)
{
    pthread_mutex_lock(&mutex_profiles);
    auto it = profiles.find(username);
    pthread_mutex_unlock(&mutex_profiles);
    return it != profiles.end();
}

bool ProfileManager::is_follower(std::string follower, std::string followed)
{
    pthread_mutex_lock(&mutex_profiles);

    Profile* p = &profiles.at(followed);

    pthread_mutex_lock(&p->mutex_followers);

    for (std::string it : p->followers)
    {
        if (it == follower)
        {
            pthread_mutex_unlock(&p->mutex_followers);
            pthread_mutex_unlock(&mutex_profiles);
            return true;
        }
    }

    pthread_mutex_unlock(&p->mutex_followers);
    pthread_mutex_unlock(&mutex_profiles);

    return false;
}

void ProfileManager::print_profiles()
{
    pthread_mutex_lock(&mutex_profiles);
    for (auto item : profiles) item.second.print_info();
    pthread_mutex_unlock(&mutex_profiles);
}