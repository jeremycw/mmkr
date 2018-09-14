#include <string.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#include "server.h"

void get_expired(join_t* joins, int n, join_t** start, int* count) {
  *count = 0;
  for (int i = n-1; i >= 0; i--) {
    if (joins[i].timeout <= 0.f) {
      *count += 1;
      *start = &joins[i];
    } else {
      return;
    }
  }
}

int sort_join_by_lobby_id_score(void* a, void* b) {
    join_t* ja = a;
    join_t* jb = b;
    return
      ja->timeout > 0.f
      && (
        ja->lobby_id < jb->lobby_id
        || (ja->lobby_id == jb->lobby_id && ja->score < jb->score)
      );
}

void segment(join_t* joins, int n, segment_t* segments, int* segment_count) {
  if (n == 0) {
    *segment_count = 0;
    return;
  }
  segment_t* segment = segments;
  *segment_count = 1;
  segment->ptr = joins;
  segment->n = 0;
  segment->lobby_id = joins[0].lobby_id;
  for (int i = 0; i < n; i++) {
    if (segment->lobby_id == joins[i].lobby_id) {
      segment->n++;
    } else {
      *segment_count += 1;
      segment++;
      segment->n = 1;
      segment->ptr = &joins[i];
      segment->lobby_id = joins[i].lobby_id;
    }
  }
}

lobby_conf_t find_lobby_config(lobby_conf_t* configs, int n, int lobby_id) {
  for (int i = 0; i < n; i++) {
    if (configs[i].lobby_id == lobby_id) return configs[i];
  }
  lobby_conf_t not_found;
  not_found.lobby_id = -1;
  return not_found;
}

void assign_timeouts(segment_t* segments, int n, lobby_conf_t* confs, int m) {
  for (int i = 0; i < n; i++) {
    lobby_conf_t conf = find_lobby_config(confs, m, segments[i].lobby_id);
    for (int j = 0; j < segments[i].n; j++) {
      segments[i].ptr[j].timeout = conf.timeout;
    }
  }
}

//void match(join_t* joins, int n, lobby_conf_t* configs, int m) {
//  //get max user count
//  int max_user_count = 2;
//  int min_user_count = 2;
//  pool_t pool;
//  pool_new(&pool, sizeof(match_t) * n/min_user_count + sizeof(int) * max_user_count * n/min_user_count);
//  lobby_conf_t config;
//  match_t* match = pool_alloc(&pool, sizeof(match_t) + sizeof(int) * config.max_user_count);
//  match->lobby_id = joins[0].lobby_id;
//  match->user_count = 0;
//  for (int i = 0; i < n; i++) {
//    if (joins[i].lobby_id == match->lobby_id) {
//      match->user_ids[match->user_count++] = joins[i].user_id;
//    } else {
//      //finalize match
//    }
//
//  }
//  match_t* matches;
//  int mi = 0;
//  for (int i = 0; i < n; i += 2) {
//    if (joins[i].lobby_id == joins[i+1].lobby_id) {
//      uuid_generate_time(matches[mi].uuid);
//    }
//  }
//}