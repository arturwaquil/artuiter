#include "../include/Signal.hpp"

int set_signal_action(int signum, __sighandler_t handler)
{
    struct sigaction sigactionSigIntHandler;
    sigactionSigIntHandler.sa_handler = (__sighandler_t) handler;
    sigemptyset(&sigactionSigIntHandler.sa_mask);
    sigactionSigIntHandler.sa_flags = 0;
    return sigaction(signum, &sigactionSigIntHandler, nullptr);
}