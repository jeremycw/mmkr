#include <string.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#include "server.h"

void tick_timers(join_t* joins, int* expirations, int n, int* nexp, float delta) {
  int index = 0;
  *nexp = 0;
  for (int i = 0; i < n; i++) {
    joins[i].timeout -= delta;
    if (joins[i].timeout > 0.f) {
      joins[index] = joins[i];
      index++;
    } else {
      expirations[*nexp] = joins[i].user_id;
      *nexp += 1;
    }
  }
}

void expand_off_wire(join_request_t* requests, join_t* joins, int n) {
  for (int i = 0; i < n; i++) {
    joins[i].lobby_id = requests[i].lobby_id;
    joins[i].score = requests[i].score;
    joins[i].timeout = 30.f;
  }
}

int sort_join_by_lobby_id_score(void* a, void* b) {
  join_t* ja = a;
  join_t* jb = b;
  return
    ja->lobby_id < jb->lobby_id
    || (ja->lobby_id == jb->lobby_id && ja->score < jb->score);
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

lobby_t find_lobby_config(lobby_t* configs, int n, int lobby_id) {
  for (int i = 0; i < n; i++) {
    if (configs[i].lobby_id == lobby_id) return configs[i];
  }
  lobby_t not_found;
  not_found.lobby_id = -1;
  return not_found;
}

void assign_timeouts(segment_t* segments, int n, lobby_t* confs, int m) {
  for (int i = 0; i < n; i++) {
    lobby_t conf = find_lobby_config(confs, m, segments[i].lobby_id);
    for (int j = 0; j < segments[i].n; j++) {
      segments[i].ptr[j].timeout = conf.timeout;
    }
  }
}

int match_count(int njoins, int max, int min) {
  int count = njoins / max * max;
  return njoins - count >= min ? count + min : count;
}

void match(join_t* joins, int n, match_t* matches, int max, int min) {
  int count = 0;
  int match_count = n / max;
  if (n % max >= min) match_count++;
  for (int i = 0; i < match_count; i++) {
    uuid_t uuid;
    uuid_generate_time(uuid);
    join_t* subjoins = &joins[count];
    match_t* submatches = &matches[count];
    int players = i == match_count-1 ? min : max;
    for (int j = 0; j < players; j++) {
      uuid_copy(submatches[j].uuid, uuid);
      submatches[j].user_id = subjoins[j].user_id;
      count++;
    }
  }
}