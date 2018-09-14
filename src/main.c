#include <unistd.h>
#include <poll.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include "tictoc.h"
#include "server.h"
#include "pool.h"

#define READ_BUFFFER_SIZE 4096

typedef struct {
    pool_t* join_pool;
    int socket;
} socket_read_thread_arg_t;

void socket_read_thread(void* arg) {
    socket_read_thread_arg_t* a = arg;
    pool_t* join_pool = a->join_pool;
    int socket = a->socket;
    char buf[READ_BUFFFER_SIZE];
    size_t bytes_left_over = 0;
    while (1) {
        ssize_t bytes = recv(socket, buf + bytes_left_over, READ_BUFFFER_SIZE, 0);
        if (bytes > 0) {
            bytes_left_over = (bytes % sizeof(join_t));
            size_t bytes_to_transfer = bytes - bytes_left_over;
            pool_write(join_pool, buf, bytes_to_transfer);
            memcpy(buf, buf + bytes_to_transfer, bytes_left_over);
        } else if (bytes == 0) {
            close(socket);
            free(arg);
            return NULL;
        } else {
            exit(1); //non blocking error
        }
    }
}

void* socket_write_thread(void* arg) {
    pool_t* write_pool;
    int socket;
    void* buf;
    size_t bytes;
    while (1) {
        pool_swap(write_pool, &buf, &bytes, POOL_WAIT_FOR_DATA);
        ssize_t sent = send(socket, buf, bytes, 0);
        if (sent < 0) {
            close(socket);
            return NULL;
        }
    }
}

int main() {
    server_t server;
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    struct pollfd listen_poller;

    //bind & listen
    {
        int flags = fcntl(s, F_GETFL, 0);
        fcntl(s, F_SETFL, flags | O_NONBLOCK);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(7654);
        int rc = bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));;
        if (rc < 0) {
            exit(1);
        }
        listen(s, 10);
        listen_poller.fd = s;
    }

    pool_new(&server.join_pool, 1024 * 1024 * 8);

    while (1) {
        TicTocTimer clock = tic();
        if (poll(&listen_poller, 1, 0)) {
            if (listen_poller.revents & POLLIN) {
                socklen_t len = sizeof(addr);
                socket_read_thread_arg_t* arg = malloc(sizeof(socket_read_thread_arg_t));
                arg->socket = accept(s, (struct sockaddr *)&addr, &len);
                arg->join_pool = &server.join_pool;
                pthread_t thread;
                pthread_create(&thread, NULL, socket_read_thread, arg);
            }
        }

        join_t* new_joins;
        size_t bytes;
        pool_swap(&server.join_pool, (void**)&new_joins, &bytes, POOL_NO_FLAGS);
        int n = bytes / sizeof(join_t);
        merge_sort(new_joins, n);
        int segment_count;
        segment_t* segments = malloc(sizeof(segment_t) * server.lobby_count);
        segment(new_joins, n, segments, &segment_count);
        assign_timeouts(segments, segment_count, server.configs, server.lobby_count);
        //merge waiting and new joins
        //match
        //write matches
        //write expireds
        //
        double delay = 0.01 - toc(&clock);
        if (delay > 0.f) {
            usleep(delay * 1000000);
        }
    }
}
