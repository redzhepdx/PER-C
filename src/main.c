#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX

// #include "../header/per.h"
#include "../header/per_single_h.h"
#include "../header/nob.h"

void test_sum_tree() {
    srand((unsigned int)time(NULL));

    SumTree *sm = create_sum_tree(16, sizeof(int));
    printf("%zu\n", sm->capacity);

    for (size_t i = 0; i < 30; ++i) {
        int    value    = rand_int(0, 100);
        double priority = (double)rand_int(1, 10) / 10.0;
        printf("Adding %d with priority %f\n", value, priority);
        printf("=================================\n");
        sum_tree_add(sm, &value, priority);
        sum_tree_show(sm);
        printf("---------------------------------\n");
        sum_data_show(sm);
    }

    getchar();
    fflush(stdin);

    SumTreeSample res;
    int           value;
    sum_tree_get(sm, 3.5, &res, &value);
    printf("Retrieved Results :\nidx -> %zu\npriority -> %f\nvalue -> %d\n", res.d_idx, res.priority, value);

    free_sum_tree(sm);
}

void test_prioritized_replay() {
    srand((unsigned int)time(NULL));

    PER *per = create_prioritized_replay(256, sizeof(int), 0.6, 0.4);
    printf("PER has been created!");

    for (size_t i = 0; i < ELEM_COUNT; ++i) {
        int value = rand_int(0, 100);
        printf("Adding :%d\n", value);
        add_to_per(per, &value);
    }

    sum_tree_show(per->tree);
    printf("----------------------------------------------------------------\n");
    sum_data_show(per->tree);
    printf("================================================================\n");

    Batch sampled_batch = sample_from_per(per, BATCH_SIZE);
    show_batch(&sampled_batch);
    printf("****************************************************************\n");

    // Fake td errors
    TD_ERRORS td_errors       = {0};
    size_t   *sampled_indices = (size_t *)malloc(sizeof(size_t) * BATCH_SIZE);
    for (size_t i = 0; i < BATCH_SIZE; ++i) {
        double fake_td = rand_double_range(-1.0, 1.0);
        da_append(&td_errors, fake_td);
        sampled_indices[i] = sampled_batch.items[i].p_idx;
    }

    printf("Fake TDs are created\n");

    update_per_priorities(per, &td_errors, sampled_indices);

    free(sampled_indices);
    free_per(per);
    da_free(td_errors);
    free_batch(&sampled_batch);
}

int main(void) {
    // test_sum_tree();
    test_prioritized_replay();
    return 0;
}