#include <string.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#include "core.h"

void tick_timers(array(join_t) joins, int* expirations, int* nexp, float delta) {
  int index = 0;
  *nexp = 0;
  for (int i = 0; i < joins.n; i++) {
    joins.buf[i].timeout -= delta;
    if (joins.buf[i].timeout > 0.f) {
      joins.buf[index] = joins.buf[i];
      index++;
    } else {
      expirations[*nexp] = joins.buf[i].user_id;
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

void segment(array(join_t) joins, segment_t* segments, int* segment_count) {
  if (joins.n == 0) {
    *segment_count = 0;
    return;
  }
  segment_t* segment = segments;
  *segment_count = 1;
  segment->joins = joins;
  segment->joins.n = 0;
  segment->lobby_id = joins.buf[0].lobby_id;
  for (int i = 0; i < joins.n; i++) {
    if (segment->lobby_id == joins.buf[i].lobby_id) {
      segment->joins.n++;
    } else {
      *segment_count += 1;
      segment++;
      segment->joins.n = 1;
      segment->joins.buf = &joins.buf[i];
      segment->lobby_id = joins.buf[i].lobby_id;
    }
  }
}

lobby_t find_lobby_config(array(lobby_t) lobbies, int lobby_id) {
  for (int i = 0; i < lobbies.n; i++) {
    if (lobbies.buf[i].lobby_id == lobby_id) return lobbies.buf[i];
  }
  lobby_t not_found;
  not_found.lobby_id = -1;
  return not_found;
}

void assign_timeouts(array(segment_t) segments, array(lobby_t) lobbies) {
  for (int i = 0; i < segments.n; i++) {
    lobby_t lobby = find_lobby_config(lobbies, segments.buf[i].lobby_id);
    for (int j = 0; j < segments.buf[i].joins.n; j++) {
      segments.buf[i].joins.buf[j].timeout = lobby.timeout;
    }
  }
}

int match_count(int njoins, int max, int min) {
  int count = njoins / max * max;
  return njoins - count >= min ? count + min : count;
}

void match(array(join_t) joins, match_t* matches, int max, int min) {
  int count = 0;
  int match_count = joins.n / max;
  if (joins.n % max >= min) match_count++;
  for (int i = 0; i < match_count; i++) {
    uuid_t uuid;
    uuid_generate_time(uuid);
    join_t* subjoins = &joins.buf[count];
    match_t* submatches = &matches[count];
    int players = i == match_count-1 ? min : max;
    for (int j = 0; j < players; j++) {
      uuid_copy(submatches[j].uuid, uuid);
      submatches[j].user_id = subjoins[j].user_id;
      count++;
    }
  }
}
