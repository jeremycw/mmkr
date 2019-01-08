#ifndef CORE_H
#define CORE_H

#include <uuid/uuid.h>
#include "pool.h"
#include "array.h"

typedef struct {
  int lobby_id;
  int user_id;
  int score;
  float timeout;
} join_t;

array_declare(join_t)

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
} lobby_t;

array_declare(lobby_t)

typedef struct {
  uuid_t uuid;
  int user_id;
} match_t;

typedef struct {
  array(join_t) joins;
  int lobby_id;
} segment_t;

array_declare(segment_t)

typedef struct {
  pool_t in_pool;
  pool_t out_pool;
  array(lobby_t) lobbies;
  array(join_t) joins;
} server_t;

void segment(array(join_t) joins, segment_t* segments, int* nseg);
void assign_timeouts(array(segment_t) segments, array(lobby_t) lobbies);
int sort_join_by_lobby_id_score(void* a, void* b);
void expand_off_wire(join_request_t* requests, join_t* joins, int n);
void tick_timers(array(join_t) joins, int* expirations, int* nexp, float delta);
void match(array(join_t) joins, match_t* matches, int max, int min);
int match_count(int njoins, int max, int min);
lobby_t find_lobby_config(array(lobby_t) lobbies, int lobby_id);

#endif
