// _____________________________________________________________________________________________________________________________________________
// SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE
// _____________________________________________________________________________________________________________________________________________
// SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE
// _____________________________________________________________________________________________________________________________________________
// SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE...SEMAPHORE

#include "semaphore.h"


// construct the semaphore
Semaphore::Semaphore(uns64 init) :
    tokens { (int64) init }   // initialize the number of (real) tokens in the semaphore
{}



// try to acquire a token from the semaphore, return if the operation was successful
bool Semaphore::try_acquire()
{
    // obtain exclusive access to the semaphore, release it when exiting the function
    std::lock_guard<std::mutex> lock(mutex);

    // if there are no (real) tokens to be acquired, return that the token wasn't acquired
    if( tokens <= 0 ) return false;

    // take a real token from the pile
    tokens--;
    // return that the token was successfully acquired
    return true;
}

// acquire a token from the semaphore, if there are none available wait until there are
void Semaphore::acquire()
{
    // obtain exclusive access to the semaphore (that is lost when waiting on a condition, and regained when the condition is fulfilled), release it when exiting the function
    std::unique_lock<std::mutex> lock(mutex);

    // take a token -- real (if the number of tokens is greater than zero) or imaginary (if the number of tokens is equal to or less than zero)
    // if the token that is going to be taken is imaginary
    if( tokens-- <= 0 )
    {
        // release exclusive access to the semaphore
        // wait until this thread wakes up, and was previously given the permission to wake up by some other thread, otherwise continue waiting (this prevents spurious wakeups)
        // the release of exclusive access and the wait are executed together as one atomic operation
        condition.wait(lock, [&] { return wakeup; });

        // reset the wake up permission (so that other waiting threads continue waiting)
        wakeup = false;
    }
}

// return a token to the semaphore
void Semaphore::release()
{
    // obtain exclusive access to the semaphore, release it when exiting the function
    std::lock_guard<std::mutex> lock(mutex);

    // return a real token
    // if there are threads with imaginary tokens, wake one up (transfer to it the returned real token)
    if( tokens++ < 0 )
    {
        // give the permission for the thread to be woken up
        wakeup = true;
        // wake up a single thread that is currently waiting
        condition.notify_one();
    }
}
