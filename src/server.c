#include <string.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#include "server.h"

#define LOBBY_LT a1[a1i].lobby_id < a2[a2i].lobby_id
#define LOBBY_EQ a1[a1i].lobby_id == a2[a2i].lobby_id
#define SCORE_LT a1[a1i].score < a2[a2i].score
#define NOT_EXPIRED !(a1[a1i].timeout <= 0.f)
#define A1_NOT_EMPTY !(a1i == size)
#define A2_EMPTY a2i == a2size

void merge_sort(join_t* in, int n) {
    int size = 1;
    join_t* array = in;
    join_t* aux = malloc(sizeof(join_t) * n);
    while (size < n) {
        int index = 0;
        for (int base = 0; base < n; base += size * 2) {
            int a1i = 0;
            int a2i = 0;
            join_t* a1 = &array[base];
            join_t* a2 = &array[base + size];
            int a2size = base + size * 2 > n ? n % size : size;
            while (a1i != size || a2i != a2size) {
                if (A2_EMPTY || (A1_NOT_EMPTY && NOT_EXPIRED && (LOBBY_LT || (LOBBY_EQ && SCORE_LT)))) {
                    aux[index] = a1[a1i++];
                } else {
                    aux[index] = a2[a2i++];
                }
                index++;
            }
        }
        size *= 2;
        join_t* tmp = array;
        array = aux;
        aux = tmp;
    }
    if (array != in) {
        memcpy(in, array, sizeof(join_t) * n);
        free(array);
    } else {
        free(aux);
    }
}

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
//    //get max user count
//    int max_user_count = 2;
//    int min_user_count = 2;
//    pool_t pool;
//    pool_new(&pool, sizeof(match_t) * n/min_user_count + sizeof(int) * max_user_count * n/min_user_count);
//    lobby_conf_t config;
//    match_t* match = pool_alloc(&pool, sizeof(match_t) + sizeof(int) * config.max_user_count);
//    match->lobby_id = joins[0].lobby_id;
//    match->user_count = 0;
//    for (int i = 0; i < n; i++) {
//        if (joins[i].lobby_id == match->lobby_id) {
//            match->user_ids[match->user_count++] = joins[i].user_id;
//        } else {
//            //finalize match
//        }
//
//    }
//    match_t* matches;
//    int mi = 0;
//    for (int i = 0; i < n; i += 2) {
//        if (joins[i].lobby_id == joins[i+1].lobby_id) {
//            uuid_generate_time(matches[mi].uuid);
//        }
//    }
//}