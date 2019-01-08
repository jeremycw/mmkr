#include <assert.h>
#include <stdlib.h>
#include "../src/core.h"

void test_segment() {
  array(join_t) joins;
  join_t buffer[6];
  joins.buf = buffer;
  joins.n = 6;
  for (int i = 0; i < joins.n; i++) {
    joins.buf[i].timeout = 30.f;
  }
  joins.buf[0].lobby_id = 1;
  joins.buf[1].lobby_id = 1;
  joins.buf[2].lobby_id = 2;
  joins.buf[3].lobby_id = 3;
  joins.buf[4].lobby_id = 3;
  joins.buf[5].lobby_id = 3;
  segment_t* segments = malloc(sizeof(segment_t) * 3);
  int segment_count;
  segment(joins, segments, &segment_count);
  assert(segments[0].joins.n == 2);
  assert(segments[0].joins.buf == &joins.buf[0]);
  assert(segments[1].joins.n == 1);
  assert(segments[1].joins.buf == &joins.buf[2]);
  assert(segments[2].joins.n == 3);
  assert(segments[2].joins.buf == &joins.buf[3]);
}
