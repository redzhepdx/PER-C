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

#include "nob.h"
#include "sum_tree.h"

#define EPS 1e-6
#define BETA_INC 1e-3
#define BATCH_SIZE 32
#define ELEM_COUNT 200

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
