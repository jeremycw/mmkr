#ifndef ATOMIC_SEND_H
#define ATOMIC_SEND_H

#include <unistd.h>
#include <pthread/pthread.h>

ssize_t atomic_send(int fd, const void* buf, size_t len, int flags, pthread_mutex_t* mutex);

#endif