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
Sumset* sumsets = NULL;
size_t stack_size = 0;
size_t task_count = 0;
_Atomic size_t next_task_index = 0;
InputData input_data;
Solution* best_solution = NULL;

static void solve(const Sumset* a, const Sumset* b, int thread_num)
{
    if (a->sum > b->sum)
        return solve(b, a, thread_num);

    if (is_sumset_intersection_trivial(a, b)) { // s(a) ∩ s(b) = {0}.
        for (size_t i = a->last; i <= input_data.d; ++i) {
            if (!does_sumset_contain(b, i)) {
                Sumset a_with_i;
                sumset_add(&a_with_i, a, i);
                solve(&a_with_i, b, thread_num);
            }
        }
    } else if ((a->sum == b->sum) && (get_sumset_intersection_size(a, b) == 2)) {
        if (b->sum > best_solution[thread_num].sum) {
            solution_build(&best_solution[thread_num], &input_data, a, b);
        }
    }
}

// Na początku programu dodaje trzy pierwsze poziomy rekurencji do tablicy zadań
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
        if (b->sum > best_solution[0].sum) {
            solution_build(&best_solution[0], &input_data, a, b);
        }
    }
}

void* parallel_solver(void* arg) {
    int thread_num = *(int*)arg;
    while (1) {
        size_t task_index = atomic_fetch_add(&next_task_index, 1);
        if (task_index >= task_count) break;

        Task* task = &tasks[task_index];
        if (task->a->sum > task->b->sum) {
            const Sumset* temp = task->a;
            task->a = task->b;
            task->b = temp;
        }

        if (is_sumset_intersection_trivial(task->a, task->b) && !does_sumset_contain(task->b, task->index)){
            Sumset a_with_i;
            sumset_add(&a_with_i, task->a, task->index);
            solve(&a_with_i, task->b, thread_num);
        }
    }
    return NULL;
}

int main() {
    input_data_read(&input_data);
    tasks = (Task*)malloc(input_data.d * input_data.d * input_data.d * sizeof(Task));
    sumsets = (Sumset*)malloc((input_data.d * input_data.d + input_data.d) * sizeof(Sumset));
    best_solution = (Solution*)malloc(input_data.t * sizeof(Solution));
    for (size_t i = 0; i < input_data.t; ++i) {
        solution_init(&best_solution[i]);
    }

    initialize(&input_data.a_start, &input_data.b_start, 0);

    pthread_t threads[input_data.t - 1];
    int thread_nums[input_data.t - 1]; // Tablica przechowująca numery wątków

    for (int i = 0; i < input_data.t - 1; ++i) {
        thread_nums[i] = i + 1; // Numeracja wątków od 1
        pthread_create(&threads[i], NULL, parallel_solver, &thread_nums[i]);
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
            solve(&a_with_i, task->b, 0);
        }
    }
    size_t result = 0;
    for (int i = 0; i < input_data.t - 1; ++i) {
        pthread_join(threads[i], NULL);
        if (best_solution[result].sum < best_solution[i].sum)
            result = i;
    }
    if (best_solution[result].sum < best_solution[input_data.t - 1].sum)
        result = input_data.t - 1;

    solution_print(&best_solution[result]);
    free(tasks);
    free(sumsets);
    free(best_solution);
    return 0;
}

