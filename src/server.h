#ifndef SERVER_H
#define SERVER_H

#include <uuid/uuid.h>
#include "pool.h"

typedef struct {
  int lobby_id;
  int user_id;
  int score;
  float timeout;
} join_t;

typedef struct {
    int lobby_id;
    int user_id;
    int score;
} join_request_t;

typedef struct {
  int lobby_id;
  int min_user_count;
  int max_user_count;
  float timeout;
} lobby_conf_t;

typedef struct {
  uuid_t uuid;
  int user_id;
} match_t;

typedef struct {
  join_t* ptr;
  int n;
  int lobby_id;
} segment_t;

typedef struct {
    int socket;
    pool_t* pool;
} write_pool_t;

typedef struct {
  pool_t join_pool;
  lobby_conf_t* configs;
  write_pool_t write_pools[16];
  int write_pool_index;
  int lobby_count;
  join_t* joins;
  int njoins;
} server_t;

void get_expired(join_t* joins, int n, join_t** start, int* count);
void segment(join_t* joins, int n, segment_t* segments, int* segment_count);
void assign_timeouts(segment_t* segments, int n, lobby_conf_t* confs, int m);
int sort_join_by_lobby_id_score(void* a, void* b);
void expand_off_wire(join_request_t* requests, join_t* joins, int n);
void tick_timers(join_t* joins, int* expirations, int n, int* nexp, float delta);

#endif