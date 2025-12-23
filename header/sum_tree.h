#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


static inline size_t min_size_t(size_t a, size_t b) { return a < b ? a : b; }
static inline size_t max_size_t(size_t a, size_t b) { return a > b ? a : b; }

int rand_int(int min, int max) {
    return min + rand() % (max - min + 1);
}

double rand_double_range(double min, double max) {
    return min + ((double)rand() / RAND_MAX) * (max - min);
}

typedef struct {
    void   *data;
    double *priority_tree;
    size_t  capacity;
    size_t  current_index;
    size_t  num_entries;
    size_t  elem_size;
} SumTree;

typedef struct {
    size_t p_idx;
    size_t d_idx;
    double priority;
} SumTreeSample;

static inline size_t sumtree_tree_size(const SumTree *t) {
    return 2 * t->capacity - 1;
}

static inline size_t sumtree_leaf_base(const SumTree *t) {
    return t->capacity - 1;
}

static inline size_t sumtree_leaf_index(const SumTree *t, size_t data_index) {
    return sumtree_leaf_base(t) + data_index;
}

static inline void *sumtree_data_ptr(SumTree *t, size_t data_index) {
    return (char *)t->data + data_index * t->elem_size;
}

SumTree *create_sum_tree(size_t capacity, size_t elem_size) {
    assert(capacity > 0);
    assert(elem_size > 0);

    // Check if the capacity is power of two
    assert((capacity & (capacity - 1)) == 0);

    SumTree *sum_tree = (SumTree *)malloc(sizeof(SumTree));

    if (sum_tree == NULL) {
        return NULL;
    }

    sum_tree->capacity      = capacity;
    sum_tree->elem_size     = elem_size;
    sum_tree->num_entries   = 0;
    sum_tree->current_index = 0;

    sum_tree->data = malloc(elem_size * capacity);
    if (sum_tree->data == NULL) {
        free(sum_tree);
        return NULL;
    }

    sum_tree->priority_tree = (double *)calloc((2 * capacity - 1), sizeof(double));
    if (sum_tree->priority_tree == NULL) {
        free(sum_tree->data);
        free(sum_tree);
        return NULL;
    }

    return sum_tree;
}

void sum_tree_update(SumTree *sum_tree, size_t tree_idx, double priority) {
    // Very unlikely but it can happen
    assert(tree_idx < sumtree_tree_size(sum_tree));

    double old_priority               = sum_tree->priority_tree[tree_idx];
    double priority_change            = priority - old_priority;
    sum_tree->priority_tree[tree_idx] = priority;

    while (tree_idx > 0) {
        tree_idx = (size_t)(tree_idx - 1) / 2;
        sum_tree->priority_tree[tree_idx] += priority_change;
    }
}

void sum_tree_add(SumTree *sum_tree, const void *item, double priority) {
    size_t elem_idx = sumtree_leaf_index(sum_tree, sum_tree->current_index);

    void *dst_data = sumtree_data_ptr(sum_tree, sum_tree->current_index);

    memcpy(dst_data, item, sum_tree->elem_size);

    sum_tree_update(sum_tree, elem_idx, priority);

    sum_tree->current_index++;
    sum_tree->current_index %= sum_tree->capacity;

    sum_tree->num_entries = min_size_t(sum_tree->num_entries + 1, sum_tree->capacity);
}

void sum_tree_get(SumTree *sum_tree, double segment, SumTreeSample *out, void *out_item) {
    assert(sum_tree);
    assert(out);

    // Check if there are elements
    double total = sum_tree->priority_tree[0];
    if (total <= 0.0) {
        *out = (SumTreeSample){0};
        return;
    }

    // Make sure that segment is not negative otherwise it will land on the first element
    if (segment < 0.0)
        segment = 0.0;

    // Make sure that segment is not larger than the sum of priorities
    if (segment >= total)
        segment = nextafter(total, 0.0);

    size_t idx       = 0;
    size_t leaf_base = sumtree_leaf_base(sum_tree);

    while (idx < leaf_base) {
        size_t left     = (idx << 1) + 1;
        double left_sum = sum_tree->priority_tree[left];

        if (segment <= left_sum)
            idx = left;
        else {
            segment -= left_sum;
            idx = left + 1;
        }
    }

    size_t data_index = idx - sumtree_leaf_base(sum_tree);

    if (out_item != NULL) {
        memcpy(out_item, sumtree_data_ptr(sum_tree, data_index), sum_tree->elem_size);
    }

    out->p_idx    = idx;
    out->d_idx    = data_index;
    out->priority = sum_tree->priority_tree[idx];
}

void sum_tree_show(SumTree *sum_tree) {
    size_t priority_tree_size = sumtree_tree_size(sum_tree);
    for (size_t level_start = 0, level_count = 1; level_start < priority_tree_size; level_start += level_count, level_count *= 2) {
        for (size_t i = 0; i < level_count && level_start + i < priority_tree_size; i++) {
            printf("%f ", sum_tree->priority_tree[level_start + i]);
        }
        printf("\n");
    }
}

void sum_data_show(SumTree *sum_tree) {
    for (size_t index = 0; index < sum_tree->capacity; ++index) {
        int   value;
        void *src = sumtree_data_ptr(sum_tree, index);
        memcpy(&value, src, sum_tree->elem_size);
        printf("%d ", value);
    }
    printf("\n");
}

void free_sum_tree(SumTree *sum_tree) {
    if (!sum_tree)
        return;
    free(sum_tree->data);
    free(sum_tree->priority_tree);
    free(sum_tree);
}
