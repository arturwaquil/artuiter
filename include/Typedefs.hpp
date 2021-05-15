#ifndef TYPEDEFS_HPP
#define TYPEDEFS_HPP

#include <list>
#include <string>

// General

typedef std::list<std::string> str_list;
typedef std::pair<std::string, std::string> str_pair;


// Profile.hpp

typedef std::pair<std::string, uint16_t> notif_info;
typedef std::pair<int,int> skt_pair;


// ClientUI.hpp

typedef std::pair<int,int> position;

#endif