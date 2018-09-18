#include <unistd.h>
#include <poll.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include "tictoc.h"
#include "server.h"
#include "pool.h"
#include "merge_sort.h"

#define READ_BUFFFER_SIZE 4096

typedef struct {
  pool_t* join_pool;
  int socket;
} socket_read_thread_arg_t;

void* socket_read_thread(void* arg) {
  socket_read_thread_arg_t* a = arg;
  pool_t* join_pool = a->join_pool;
  int socket = a->socket;
  char buf[READ_BUFFFER_SIZE];
  size_t bytes_left_over = 0;
  while (1) {
    ssize_t bytes =
      recv(socket, buf + bytes_left_over, READ_BUFFFER_SIZE, 0);
    if (bytes > 0) {
      bytes_left_over = (bytes % sizeof(join_t));
      ssize_t bytes_to_transfer = bytes - bytes_left_over;
      write_t* write = pool_alloc_block_for_write(join_pool, bytes_to_transfer);
      expand_off_wire(
        buf + bytes_left_over,
        write->buf,
        bytes / sizeof(join_request_t)
      );
      pool_commit_write(join_pool, write);
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
  write_pool_t* write_pool = arg;
  int socket = write_pool->socket;
  pool_reader_t reader = pool_new_reader(write_pool->pool);
  void* buf;
  while (1) {
    ssize_t bytes = pool_read(write_pool->pool, &reader, &buf, -1);
    assert(bytes > 0);
    ssize_t sent = send(socket, buf, bytes, 0);
    if (sent < 0) {
      close(socket);
      return NULL;
    }
  }
}

void* expire_requests(void* arg) {
  join_t* joins;
  int n;
  int nexp;
  int* expirations = malloc(sizeof(int) * n);
  tick_timers(joins, expirations, n, &nexp, 0.01f);

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
    int rc = bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    if (rc < 0) {
      exit(1);
    }
    listen(s, 10);
    listen_poller.fd = s;
  }

  pool_new(&server.join_pool, 1024 * 1024 * 8);
  pool_reader_t reader = pool_new_reader(&server.join_pool);

  while (1) {
    TicTocTimer clock = tic();
    if (poll(&listen_poller, 1, 0)) {
      if (listen_poller.revents & POLLIN) {
        socklen_t len = sizeof(addr);
        socket_read_thread_arg_t* arg =
          malloc(sizeof(socket_read_thread_arg_t));
        arg->socket = accept(s, (struct sockaddr *)&addr, &len);
        arg->join_pool = &server.join_pool;
        pthread_t reader, writer;
        pthread_create(&reader, NULL, socket_read_thread, arg);
        assert(server.write_pool_index < 16);
        pool_t* pool = malloc(sizeof(pool_t));
        pool_new(pool, 1024 * 1024);
        server.write_pools[server.write_pool_index].pool = pool;
        server.write_pools[server.write_pool_index].socket = arg->socket;
        server.write_pool_index++;
        pthread_create(&writer, NULL, socket_write_thread, arg);
      }
    }

    join_t* new_joins;
    ssize_t bytes = pool_read(&server.join_pool, &reader, (void**)&new_joins, -1);
    int n = bytes / sizeof(join_t);
    merge_sort(new_joins, n, sizeof(join_t), sort_join_by_lobby_id_score);
    int segment_count;
    segment_t* segments = malloc(sizeof(segment_t) * server.lobby_count);
    segment(new_joins, n, segments, &segment_count);
    assign_timeouts(
      segments,
      segment_count,
      server.configs,
      server.lobby_count
    );
    join_t* joins = malloc(sizeof(join_t) * (n + server.njoins));
    int index = 0;
    merge(
      new_joins,
      server.joins,
      joins,
      n,
      server.njoins,
      sizeof(join_t),
      &index,
      sort_join_by_lobby_id_score
    );
    free(server.joins);
    n += server.njoins;
    int nexp;
    segment(joins, n, segments, &segment_count);
    //write matches
    //write expireds
    //
    double delay = 0.01 - toc(&clock);
    if (delay > 0.f) {
      usleep(delay * 1000000);
    }
  }
}
