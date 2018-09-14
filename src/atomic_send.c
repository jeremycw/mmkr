#include <pthread/pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include "atomic_send.h"

ssize_t atomic_send(int fd, const void* buf, size_t len, int flags, pthread_mutex_t* mutex) {
    pthread_mutex_lock(mutex);
    ssize_t bytes = send(fd, buf, len, flags);
    pthread_mutex_unlock(mutex);
    return bytes;
}