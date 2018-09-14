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
  int min_user_count;
  int max_user_count;
  float timeout;
} lobby_conf_t;

typedef struct {
  uuid_t uuid;
  int lobby_id;
  int user_count;
  int user_ids[0];
} match_t;

typedef struct {
  join_t* ptr;
  int n;
  int lobby_id;
} segment_t;

typedef struct {
  pool_t join_pool;
  lobby_conf_t* configs;
  int lobby_count;
} server_t;

void merge_sort(join_t* in, int n);
void get_expired(join_t* joins, int n, join_t** start, int* count);
void segment(join_t* joins, int n, segment_t* segments, int* segment_count);
void assign_timeouts(segment_t* segments, int n, lobby_conf_t* confs, int m);

#endif