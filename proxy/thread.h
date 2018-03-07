#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>

class Thread
{
public:
    Thread();
    virtual ~Thread();

    virtual void Run() = 0;

    bool Start();

    bool Join();

    void Exit();
private:
    ::pthread_t m_thread_id;
};

#endif

