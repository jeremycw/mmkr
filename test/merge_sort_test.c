#include <assert.h>
#include "../src/core.h"
#include "../src/merge_sort.h"

void test_merge_sort() {
  join_t even[4] = { {1, 1234, 1200, 30.f}, {1, 2345, 1000, 15.f}, {1, 4567, 1300, 0.f}, {2, 9876, 1000, 30.f} };
  merge_sort(even, 4, sizeof(join_t), sort_join_by_lobby_id_score);
  assert(even[0].user_id == 2345);
  assert(even[1].user_id == 1234);
  assert(even[2].user_id == 4567);
  assert(even[3].user_id == 9876);
}
