#include "thread.h"

Thread::Thread() {
    m_thread_id = 0;
}

Thread::~Thread() {
}

static void* ThreadEntry(void* arg) {
    Thread* thread = reinterpret_cast<Thread*>(arg);

    thread->Run();

    return NULL;
}

bool Thread::Start() {
    if (::pthread_create(&m_thread_id, NULL, ThreadEntry, this) != 0) {
        return false;
    }

    return true;
}

bool Thread::Join() {
    return (::pthread_join(m_thread_id, NULL) == 0);
}


void Thread::Exit() {
    ::pthread_exit(NULL);
}

