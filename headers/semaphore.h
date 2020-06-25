// _____________________________________________________________________________________________________________________________________________
// SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE
// _____________________________________________________________________________________________________________________________________________
// SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE
// _____________________________________________________________________________________________________________________________________________
// SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE

#pragma once
#include "!global.h"
#include <mutex>
#include <condition_variable>


// an implementation of a counting semaphore (missing in C++17)
// all the methods in this class are thread safe, except the destructor
// +   the destructor is not thread-safe on purpose, since it should be called after there are no threads waiting on the semaphore (it is not the responsibility of the semaphore to ensure that there are no threads waiting)
class Semaphore
{
private:
    static const int64 one { 1LL };      // the default number of real tokens in the pile
    int64 tokens { one };                // the number of tokens in the pile
    bool wakeup { false };               // bool that tells if a thread that was sleeping should be woken up

    std::mutex mutex;                    // mutex used for exclusive access to the semaphore
    std::condition_variable condition;   // condition used for notifying that there is a token available


public:
    // construct the semaphore
    explicit Semaphore(uns64 init = one);

    // try to acquire a token from the semaphore, return if the operation was successful
    bool try_acquire();
    // acquire a token from the semaphore, if there are none available wait until there are
    void acquire();
    // return a token to the semaphore
    void release();
};
