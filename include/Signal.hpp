#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <csignal>

int set_signal_action(int signum, __sighandler_t handler);

#endif