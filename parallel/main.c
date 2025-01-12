#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include "common/io.h"
#include "common/sumset.h"

typedef struct {
    const Sumset* a;
    const Sumset* b;
    size_t index;
} Task;

typedef struct {
    Task tasks[2500];
    Sumset sumsets[2550];
    size_t stack_size;
    size_t task_count;
    _Atomic size_t next_task_index;
    pthread_mutex_t best_solution_mutex;
    pthread_mutex_t init_mutex;
    pthread_cond_t init_cond;
    InputData* input_data;
    Solution* best_solution;
    int init_done;
} SharedData;

static void solve(const Sumset* a, const Sumset* b, SharedData* shared_data)
{
    if (a->sum > b->sum)
        return solve(b, a, shared_data);

    if (is_sumset_intersection_trivial(a, b)) { // s(a) ∩ s(b) = {0}.
        for (size_t i = a->last; i <= shared_data->input_data->d; ++i) {
            if (!does_sumset_contain(b, i)) {
                Sumset a_with_i;
                sumset_add(&a_with_i, a, i);
                solve(&a_with_i, b, shared_data);
            }
        }
    } else if ((a->sum == b->sum) && (get_sumset_intersection_size(a, b) == 2)) {
        pthread_mutex_lock(&shared_data->best_solution_mutex);
        if (b->sum > shared_data->best_solution->sum) {
            solution_build(shared_data->best_solution, shared_data->input_data, a, b);
        }
        pthread_mutex_unlock(&shared_data->best_solution_mutex);
    }
}

static void initialize(const Sumset* a, const Sumset* b, SharedData* shared_data, size_t level)
{
    if (a->sum > b->sum)
        return initialize(b, a, shared_data, level);
    level++;

    if (is_sumset_intersection_trivial(a, b)) { // s(a) ∩ s(b) = {0}.
        for (size_t i = a->last; i <= shared_data->input_data->d; ++i) {
            if (!does_sumset_contain(b, i)) {
                if (level == 1) {
                    sumset_add(&shared_data->sumsets[shared_data->stack_size++], a, i);
                    initialize(&shared_data->sumsets[shared_data->stack_size - 1], b, shared_data, level);
                }
                else {
                    shared_data->tasks[shared_data->task_count++] = (Task){
                        .a = a,
                        .b = b,
                        .index = i
                    };
                }
            }
        }
    } else if ((a->sum == b->sum) && (get_sumset_intersection_size(a, b) == 2)) {
        if (b->sum > shared_data->best_solution->sum) {
            solution_build(shared_data->best_solution, shared_data->input_data, a, b);
        }
    }
}

void* task_initializer(void* arg) {
    SharedData* shared_data = (SharedData*)arg;
    initialize(&shared_data->input_data->a_start, &shared_data->input_data->b_start, shared_data, 0);

    pthread_mutex_lock(&shared_data->init_mutex);
    shared_data->init_done = 1;
    pthread_cond_signal(&shared_data->init_cond);
    pthread_mutex_unlock(&shared_data->init_mutex);

    while (1) {
        size_t task_index = atomic_fetch_add(&shared_data->next_task_index, 1);
        if (task_index >= shared_data->task_count) break;

        Task* task = &shared_data->tasks[task_index];
        if (task->a->sum > task->b->sum) {
            const Sumset* temp = task->a;
            task->a = task->b;
            task->b = temp;
        }

        if (is_sumset_intersection_trivial(task->a, task->b) && !does_sumset_contain(task->b, task->index)){ // nwm czy potrzebne
            Sumset a_with_i;
            sumset_add(&a_with_i, task->a, task->index);
            solve(&a_with_i, task->b, shared_data);
        }
    }

    return NULL;
}

void* parallel_solver(void* arg) {
    SharedData* shared_data = (SharedData*)arg;

    while (1) {
        size_t task_index = atomic_fetch_add(&shared_data->next_task_index, 1);
        if (task_index >= shared_data->task_count) break;

        Task* task = &shared_data->tasks[task_index];
        if (task->a->sum > task->b->sum) {
            const Sumset* temp = task->a;
            task->a = task->b;
            task->b = temp;
        }

        if (is_sumset_intersection_trivial(task->a, task->b) && !does_sumset_contain(task->b, task->index)){
            Sumset a_with_i;
            sumset_add(&a_with_i, task->a, task->index);
            solve(&a_with_i, task->b, shared_data);
        }
    }
    return NULL;
}

int main() {
    InputData input_data;
    input_data_read(&input_data);
    Solution best_solution;
    solution_init(&best_solution);

    SharedData shared_data = {
        .task_count = 0,
        .next_task_index = 0,
        .best_solution_mutex = PTHREAD_MUTEX_INITIALIZER,
        .init_mutex = PTHREAD_MUTEX_INITIALIZER,
        .init_cond = PTHREAD_COND_INITIALIZER,
        .input_data = &input_data,
        .best_solution = &best_solution,
        .init_done = 0,
        .stack_size = 0,
    };

    pthread_t threads[input_data.t];

    pthread_create(&threads[0], NULL, task_initializer, &shared_data);

    pthread_mutex_lock(&shared_data.init_mutex);
    while (!shared_data.init_done) {
        pthread_cond_wait(&shared_data.init_cond, &shared_data.init_mutex);
    }
    pthread_mutex_unlock(&shared_data.init_mutex);
    for (int i = 1; i < input_data.t; ++i) {
        pthread_create(&threads[i], NULL, parallel_solver, &shared_data);
    }
    for (int i = 0; i < input_data.t; ++i) {
        pthread_join(threads[i], NULL);
    }
    solution_print(&best_solution);
    return 0;
}

