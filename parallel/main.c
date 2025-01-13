#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include "common/io.h"
#include "common/sumset.h"

typedef struct {
    const Sumset* a;
    const Sumset* b;
    size_t index;
} Task;

Task* tasks = NULL;
Sumset sumsets[2550];
size_t stack_size = 0;
size_t task_count = 0;
_Atomic size_t next_task_index = 0;
pthread_mutex_t best_solution_mutex = PTHREAD_MUTEX_INITIALIZER;
InputData input_data;
Solution best_solution;

static void solve(const Sumset* a, const Sumset* b)
{
    if (a->sum > b->sum)
        return solve(b, a);

    if (is_sumset_intersection_trivial(a, b)) { // s(a) ∩ s(b) = {0}.
        for (size_t i = a->last; i <= input_data.d; ++i) {
            if (!does_sumset_contain(b, i)) {
                Sumset a_with_i;
                sumset_add(&a_with_i, a, i);
                solve(&a_with_i, b);
            }
        }
    } else if ((a->sum == b->sum) && (get_sumset_intersection_size(a, b) == 2)) {
        pthread_mutex_lock(&best_solution_mutex);
        if (b->sum > best_solution.sum) {
            solution_build(&best_solution, &input_data, a, b);
        }
        pthread_mutex_unlock(&best_solution_mutex);
    }
}

static void initialize(const Sumset* a, const Sumset* b, size_t level)
{

    if (a->sum > b->sum)
        return initialize(b, a, level);
    level++;

    if (is_sumset_intersection_trivial(a, b)) { // s(a) ∩ s(b) = {0}.
        for (size_t i = a->last; i <= input_data.d; ++i) {
            if (!does_sumset_contain(b, i)) {
                if (level <= 2) {
                    sumset_add(&sumsets[stack_size++], a, i);
                    initialize(&sumsets[stack_size - 1], b, level);
                }
                else {
                    tasks[task_count++] = (Task){
                        .a = a,
                        .b = b,
                        .index = i
                    };

                }
            }
        }
    } else if ((a->sum == b->sum) && (get_sumset_intersection_size(a, b) == 2)) {
        if (b->sum > best_solution.sum) {
            solution_build(&best_solution, &input_data, a, b);
        }
    }
}

void* parallel_solver(void* arg) {
    while (1) {
        size_t task_index = atomic_fetch_add(&next_task_index, 1);
        if (task_index >= task_count) break;

        Task* task = &tasks[task_index];
        if (task->a->sum > task->b->sum) {
            const Sumset* temp = task->a;
            task->a = task->b;
            task->b = temp;
        }

        if (is_sumset_intersection_trivial(task->a, task->b) && !does_sumset_contain(task->b, task->index)){ // nwm czy potrzebne
            Sumset a_with_i;
            sumset_add(&a_with_i, task->a, task->index);
            solve(&a_with_i, task->b);
        }
    }
    return NULL;
}

int main() {
    input_data_read(&input_data);
    solution_init(&best_solution);
    tasks = (Task*)malloc(input_data.d * input_data.d * input_data.d * sizeof(Task));

    initialize(&input_data.a_start, &input_data.b_start, 0);

    pthread_t threads[input_data.t - 1];

    for (int i = 0; i < input_data.t - 1; ++i) {
        pthread_create(&threads[i], NULL, parallel_solver, NULL);
    }

    while (1) {
        size_t task_index = atomic_fetch_add(&next_task_index, 1);
        if (task_index >= task_count) break;

        Task* task = &tasks[task_index];
        if (task->a->sum > task->b->sum) {
            const Sumset* temp = task->a;
            task->a = task->b;
            task->b = temp;
        }

        if (is_sumset_intersection_trivial(task->a, task->b) && !does_sumset_contain(task->b, task->index)){ // nwm czy potrzebne
            Sumset a_with_i;
            sumset_add(&a_with_i, task->a, task->index);
            solve(&a_with_i, task->b);
        }
    }

    for (int i = 0; i < input_data.t - 1; ++i) {
        pthread_join(threads[i], NULL);
    }

    solution_print(&best_solution);
    free(tasks);
    return 0;
}

