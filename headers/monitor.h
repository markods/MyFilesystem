// _____________________________________________________________________________________________________________________________________________
// MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...M
// _____________________________________________________________________________________________________________________________________________
// MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...M
// _____________________________________________________________________________________________________________________________________________
// MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...MONITOR...M

#pragma once
#include <mutex>
#include "!global.h"


// class used to lock a scope (from the point of class creation to the end of the scope)
// its methods are reentrant
class Monitor
{
private:
    std::recursive_mutex& m;

public:
    // save the given recursive mutex and lock it
    Monitor(std::recursive_mutex& mutex) : m{mutex} { m.lock(); }
    // destroy the monitor and unlock the recursive mutex
    ~Monitor() { m.unlock(); }
};

// a handy notation for using this monitor
#define MONITORED(mutex) Monitor dummy_monitor{mutex};
