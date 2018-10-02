#include <assert.h>
#include <stdlib.h>
#include "../src/core.h"

void test_segment() {
  join_t joins[6];
  for (int i = 0; i < 6; i++) {
    joins[i].timeout = 30.f;
  }
  joins[0].lobby_id = 1;
  joins[1].lobby_id = 1;
  joins[2].lobby_id = 2;
  joins[3].lobby_id = 3;
  joins[4].lobby_id = 3;
  joins[5].lobby_id = 3;
  segment_t* segments = malloc(sizeof(segment_t) * 3);
  int segment_count;
  segment(joins, 6, segments, &segment_count);
  assert(segments[0].n == 2);
  assert(segments[0].ptr == &joins[0]);
  assert(segments[1].n == 1);
  assert(segments[1].ptr == &joins[2]);
  assert(segments[2].n == 3);
  assert(segments[2].ptr == &joins[3]);
}