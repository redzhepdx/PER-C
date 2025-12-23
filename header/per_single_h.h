#ifndef HEADER_PER_SH_H
#define HEADER_PER_SH_H

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define EPS 1e-6
#define BETA_INC 1e-3
#define BATCH_SIZE 32
#define ELEM_COUNT 200

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


typedef struct {
    double *items;
    size_t  count;
    size_t  capacity;
} TD_ERRORS;

typedef struct
{
    SumTree *tree;
    double   alpha;
    double   beta;
    double   max_priority;
} PER;

typedef struct {
    SumTreeSample *items;
    size_t         count;
    double        *importance_weights;
} Batch;

void free_per(PER *per) {
    if (!per)
        return;
    free_sum_tree(per->tree);
    free(per);
}

PER *create_prioritized_replay(size_t capacity, size_t elem_size, double alpha, double beta) {
    PER *per = (PER *)malloc(sizeof(PER));
    if (per == NULL) {
        return NULL;
    }

    per->tree = create_sum_tree(capacity, elem_size);

    if (!per->tree) {
        free(per);
        return NULL;
    }

    per->alpha        = alpha;
    per->beta         = beta;
    per->max_priority = 1.0;
    return per;
}

double calculate_priority(const PER *per, double td_error) {
    return pow(fabs(td_error) + EPS, per->alpha);
}

void add_to_per(PER *per, const void *item) {
    sum_tree_add(per->tree, item, per->max_priority);
}

void calculate_sampling_priorities(const Batch *batch, double *out_importance_weights, double tree_top_value, size_t total_entry_count, double beta) {
    if (total_entry_count == 0 || tree_top_value <= 0.0) {
        memset(out_importance_weights, 0, batch->count * sizeof *out_importance_weights);
        return;
    }

    double max_importance_weight = 0.0;

    for (size_t i = 0; i < batch->count; ++i) {
        if (tree_top_value <= 0.0) {
            out_importance_weights[i] = 0.0;
            continue;
        }

        double prob = batch->items[i].priority / tree_top_value;
        if (prob < 1e-12)
            prob = 1e-12;

        double w                  = pow(1.0 / ((double)total_entry_count * prob), beta);
        out_importance_weights[i] = w;

        if (w > max_importance_weight)
            max_importance_weight = w;
    }

    // Normalise once - guard against division by zero
    if (max_importance_weight <= 0.0) {
        // all weights are 0 already
        return;
    }

    for (size_t i = 0; i < batch->count; ++i) {
        out_importance_weights[i] /= max_importance_weight;
    }
}

static inline void free_batch(Batch *b) {
    free(b->items);
    free(b->importance_weights);
    b->items              = NULL;
    b->importance_weights = NULL;
}

Batch sample_from_per(PER *per, size_t batch_size) {
    assert(per->tree->num_entries >= batch_size);

    Batch batch              = {0};
    batch.items              = (SumTreeSample *)malloc(batch_size * sizeof(batch.items[0]));
    batch.importance_weights = (double *)malloc(batch_size * sizeof(double));

    if (!batch.items || !batch.importance_weights) {
        free(batch.items);
        free(batch.importance_weights);
        return (Batch){0};
    }

    batch.count = batch_size;

    double tree_top_value = per->tree->priority_tree[0];
    if (tree_top_value <= 0.0) {
        for (size_t i = 0; i < batch_size; ++i) {
            batch.items[i]              = (SumTreeSample){0};
            batch.importance_weights[i] = 0.0;
        }
        return batch;
    }

    double segment = tree_top_value / (double)batch_size;

    per->beta = fmin(1.0, per->beta + BETA_INC);

    for (size_t i = 0; i < batch_size; ++i) {
        double a = segment * (double)i;
        double b = segment * (double)(i + 1);
        double x = rand_double_range(a, b);

        // keep strictly inside [0, tree_top_value)
        if (x >= tree_top_value)
            x = nextafter(tree_top_value, 0.0);

        sum_tree_get(per->tree, x, &batch.items[i], NULL);
    }

    calculate_sampling_priorities(&batch, batch.importance_weights,
                                  tree_top_value, per->tree->num_entries, per->beta);
    return batch;
}

void update_per_priorities(PER *per, TD_ERRORS *td_errors, size_t *priority_indices) {
    assert(per && per->tree && td_errors && priority_indices);

    for (size_t idx = 0; idx < td_errors->count; ++idx) {
        double new_priority = calculate_priority(per, td_errors->items[idx]);
        sum_tree_update(per->tree, priority_indices[idx], new_priority);
        per->max_priority = fmax(per->max_priority, new_priority);
    }
}

void show_batch(Batch *batch) {
    for (size_t idx = 0; idx < batch->count; ++idx) {
        printf("%zu %f\n", batch->items[idx].d_idx, batch->importance_weights[idx]);
    }
}

#endif // HEADER_PER_SH_H