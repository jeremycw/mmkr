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
} lobby_t;

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
  pool_t in_pool;
  pool_t out_pool;
  lobby_t* lobbies;
  join_t* joins;
  int nlob;
  int njoins;
} server_t;

void segment(join_t* joins, int njoins, segment_t* segments, int* nseg);
void assign_timeouts(segment_t* segments, int n, lobby_t* lobbies, int m);
int sort_join_by_lobby_id_score(void* a, void* b);
void expand_off_wire(join_request_t* requests, join_t* joins, int n);
void tick_timers(join_t* joins, int* expirations, int n, int* nexp, float delta);
void match(join_t* joins, int n, match_t* matches, int max, int min);
int match_count(int njoins, int max, int min);
lobby_t find_lobby_config(lobby_t* configs, int n, int lobby_id);

#endif