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
#include "parallel_map.h"

#define READ_BUFFFER_SIZE 4096

parallel_map_declare(segment_t)
parallel_map_define(segment_t)

typedef struct {
  pool_t* pool;
  int socket;
} socket_pool_t;

void* socket_read_thread(void* arg) {
  socket_pool_t* a = arg;
  pool_t* in_pool = a->pool;
  int socket = a->socket;
  char buf[READ_BUFFFER_SIZE];
  size_t bytes_left_over = 0;
  while (1) {
    ssize_t bytes =
      recv(socket, buf + bytes_left_over, READ_BUFFFER_SIZE, 0);
    if (bytes > 0) {
      bytes_left_over = (bytes % sizeof(join_t));
      ssize_t bytes_to_transfer = bytes - bytes_left_over;
      write_t* write = pool_alloc_block_for_write(in_pool, bytes_to_transfer);
      expand_off_wire(
        (join_request_t*)buf + bytes_left_over,
        write->buf,
        bytes / sizeof(join_request_t)
      );
      pool_commit_write(in_pool, write);
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
  socket_pool_t* a = arg;
  int socket = a->socket;
  pool_t* out_pool = a->pool;
  pool_reader_t reader = pool_new_reader(out_pool);
  void* buf;
  while (1) {
    ssize_t bytes = pool_read(out_pool, &reader, &buf, -1);
    assert(bytes > 0);
    ssize_t sent = send(socket, buf, bytes, 0);
    if (sent < 0) {
      close(socket);
      return NULL;
    }
  }
}

typedef struct {
  lobby_t* lobbies;
  pool_t* out_pool;
  int nlob;
} match_map_arg_t;

void match_mapfn(segment_t* segments, int start, int n, void* arg) {
  match_map_arg_t* a = arg;
  assert(n == 1);
  segment_t* segment = &segments[start];
  lobby_t lobby = find_lobby_config(a->lobbies, a->nlob, segment->lobby_id);
  int nmat = match_count(segment->n, lobby.max_user_count, lobby.min_user_count);
  write_t* write = pool_alloc_block_for_write(a->out_pool, sizeof(match_t) * nmat);
  match(segment->ptr, segment->n, write->buf, lobby.max_user_count, lobby.min_user_count);
  pool_commit_write(a->out_pool, write);
}

int main() {
  server_t server;
  int s = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr;
  struct pollfd listen_poller;
  threadpool thpool = thpool_init(4);

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

  pool_new(&server.in_pool, 1024 * 1024 * 8);
  pool_reader_t reader = pool_new_reader(&server.in_pool);
  pool_new(&server.out_pool, 1024 * 1024 * 8);

  while (1) {
    TicTocTimer clock = tic();
    if (poll(&listen_poller, 1, 0)) {
      if (listen_poller.revents & POLLIN) {
        socklen_t len = sizeof(addr);
        socket_pool_t* arg =
          malloc(sizeof(socket_pool_t));
        int socket = accept(s, (struct sockaddr *)&addr, &len);
        arg->socket = socket;
        arg->pool = &server.in_pool;
        pthread_t reader, writer;
        pthread_create(&reader, NULL, socket_read_thread, arg);
        arg = malloc(sizeof(socket_pool_t));
        arg->socket = socket;
        arg->pool = &server.out_pool;
        pthread_create(&writer, NULL, socket_write_thread, arg);
      }
    }

    join_t* new_joins;
    ssize_t bytes = pool_read(&server.in_pool, &reader, (void**)&new_joins, -1);
    int njoins = bytes / sizeof(join_t);
    merge_sort(new_joins, njoins, sizeof(join_t), sort_join_by_lobby_id_score);
    int nseg;
    segment_t* segments = malloc(sizeof(segment_t) * server.nlob);
    segment(new_joins, njoins, segments, &nseg);
    assign_timeouts(
      segments,
      nseg,
      server.lobbies,
      server.nlob
    );
    join_t* joins = malloc(sizeof(join_t) * (njoins + server.njoins));
    int index = 0;
    merge(
      new_joins,
      server.joins,
      joins,
      njoins,
      server.njoins,
      sizeof(join_t),
      &index,
      sort_join_by_lobby_id_score
    );
    free(server.joins);
    njoins += server.njoins;
    segment(joins, njoins, segments, &nseg);
    match_map_arg_t arg;
    arg.lobbies = server.lobbies;
    arg.nlob = server.nlob;
    arg.out_pool = &server.out_pool;
    parallel_map(segment_t, thpool, nseg, segments, nseg, &arg, match_mapfn);
    //write expireds
    //
    double delay = 0.01 - toc(&clock);
    if (delay > 0.f) {
      usleep(delay * 1000000);
    }
  }
}
