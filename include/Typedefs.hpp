#ifndef TYPEDEFS_HPP
#define TYPEDEFS_HPP

#include <list>
#include <string>

// String-related. For conciseness only
typedef std::list<std::string> str_list;
typedef std::pair<std::string, std::string> str_pair;

// Used to keep both sockets' (cmd and ntf) info in one structure.
typedef std::pair<int,int> skt_pair;

#endif