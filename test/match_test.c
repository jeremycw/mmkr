#include <assert.h>
#include <string.h>
#include "../src/core.h"

void test_match() {
  join_t buffer[8];
  array(join_t) joins;
  joins.buf = buffer;
  joins.n = 8;
  for (int i = 0; i < joins.n; i++) {
    joins.buf[i].lobby_id = 1;
    joins.buf[i].score = 1000 + i*100;
    joins.buf[i].user_id = i + 1;
    joins.buf[i].timeout = 15.f;
  }
  match_t matches[8];
  int min = 2;
  int max = 3;
  match(joins, matches, max, min);
  assert(matches[0].user_id == 1);
  char uuid1[37];
  char uuid2[37];
  uuid_unparse(matches[0].uuid, uuid1);
  uuid_unparse(matches[1].uuid, uuid2);
  assert(uuid_compare(matches[0].uuid, matches[1].uuid) == 0);
}
