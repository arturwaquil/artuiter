#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "Profile.hpp"

#include <nlohmann/json.hpp>

void json_from_profiles(std::map<std::string, Profile> profiles);
std::map<std::string, Profile> profiles_from_json();

#endif