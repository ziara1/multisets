#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include "common/io.h"
#include "common/sumset.h"

typedef struct Node {
    void* data;
    struct Node* next;
} Node;

typedef struct {
    int tag;
    Node* head;
} Stack;

typedef struct {
    Sumset a;
    Sumset b;
} Task;

typedef struct {
    InputData* input_data;
    Solution* best_solution;
    _Atomic int active_threads;
} SharedData;


static _Atomic Stack task_stack = {0, NULL};

void push(_Atomic Stack* stack, void* data) {
    Stack next, prev;
    Node* new_node = malloc(sizeof(Node));
    if (!new_node)
        exit(1);
    new_node->data = data;
    prev = atomic_load(stack);
    do {
        new_node->next = prev.head;
        next.head = new_node;
        next.tag = prev.tag + 1;
    } while (!atomic_compare_exchange_weak(stack, &prev, next));
}

void* pop(_Atomic Stack* stack) {
    void* data;
    Stack next, prev;
    prev = atomic_load(stack);
    do {
        /*if (!prev.head)
            return NULL; // Empty stack*/
        next.head = prev.head->next;
        next.tag = prev.tag + 1;
    } while (!atomic_compare_exchange_weak(stack, &prev, next));

    data = prev.head->data;
    free(prev.head);
    return data;
}

static pthread_mutex_t best_solution_mutex = PTHREAD_MUTEX_INITIALIZER;

static void solve(const Sumset* a, const Sumset* b, size_t d, SharedData* shared_data)
{
    if (a->sum > b->sum)
        return solve(b, a, d, shared_data);

    if (is_sumset_intersection_trivial(a, b)) { // s(a) ∩ s(b) = {0}.
        for (size_t i = a->last; i <= d; ++i) {
            if (!does_sumset_contain(b, i)) {
                Sumset a_with_i;
                sumset_add(&a_with_i, a, i);
                solve(&a_with_i, b, d, shared_data);
            }
        }
    } else if ((a->sum == b->sum) && (get_sumset_intersection_size(a, b) == 2)) {
        pthread_mutex_lock(&best_solution_mutex);
        if (b->sum > shared_data->best_solution->sum) {
            solution_build(shared_data->best_solution, shared_data->input_data, a, b);
        }
        pthread_mutex_unlock(&best_solution_mutex);
    }
}

void* parallel_solver(void* arg) {
    SharedData* shared_data = (SharedData*)arg;
    InputData* input_data = shared_data->input_data;

    while (atomic_load(&shared_data->active_threads) > 0) {
        Task* task = pop(&task_stack);
        if (!task) {

            sched_yield();
            continue;
        }

        const Sumset a = task->a;
        const Sumset b = task->b;
        free(task);

        solve(&a, &b, input_data->d, shared_data);

        Stack loaded_stack = atomic_load(&task_stack);
        if (!loaded_stack.head) {
            atomic_fetch_sub(&shared_data->active_threads, 1);
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
        .input_data = &input_data,
        .best_solution = &best_solution,
        .active_threads = input_data.t
    };

    Task* initial_task = malloc(sizeof(Task));
    if (!initial_task)
        exit(1);

    initial_task->a = input_data.a_start;
    initial_task->b = input_data.b_start;
    push(&task_stack, initial_task);

    pthread_t threads[input_data.t];
    for (int i = 0; i < input_data.t; ++i) {
        pthread_create(&threads[i], NULL, parallel_solver, &shared_data);
    }
    for (int i = 0; i < input_data.t; ++i) {
        pthread_join(threads[i], NULL);
    }

    solution_print(&best_solution);

    return 0;
}

