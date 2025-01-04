#include <stddef.h>
#include <stdlib.h>  // Dla funkcji malloc, free
#include "common/io.h"
#include "common/sumset.h"

typedef struct {
    Sumset *a, *b;
    size_t indexA, indexB;
    bool firstA, firstB;
} StackFrame;

void solve_iterative(InputData* input_data, Solution* best_solution) {
    // Rozmiar stosu: maksymalna liczba stanów DFS
    size_t stack_capacity = input_data->d * input_data->d;
    size_t stack_size = 0;
    StackFrame* stack = malloc(stack_capacity * sizeof(StackFrame));
    if (!stack) exit(1); // Błąd alokacji
    stack[stack_size++] = (StackFrame){
        .a = malloc(sizeof(Sumset)),
        .b = malloc(sizeof(Sumset)),
        .indexA = input_data->a_start.last,
        .indexB = input_data->b_start.last,
        .firstA = true,
        .firstB = true
    };
    if (!stack[0].a || !stack[0].b) exit(1); // Błąd alokacji
    *(stack[0].a) = input_data->a_start;
    *(stack[0].b) = input_data->b_start;

    while (stack_size > 0) {
        const size_t s = stack_size - 1;
        if (stack[s].a->sum > stack[s].b->sum) {
            Sumset* temp = stack[s].a;
            stack[s].a = stack[s].b;
            stack[s].b = temp;

            size_t temp_index = stack[s].indexA;
            stack[s].indexA = stack[s].indexB;
            stack[s].indexB = temp_index;

            bool tempFirstA = stack[s].firstA;
            stack[s].firstA = stack[s].firstB;
            stack[s].firstB = tempFirstA;
        }


        if (stack[s].indexA > input_data->d) {
            if (stack[s].firstA)
                free(stack[s].a);
            if (stack[s].firstB)
                free(stack[s].b);
            --stack_size;
            continue;
        }
        if (is_sumset_intersection_trivial(stack[s].a, stack[s].b)) {
            const size_t i = stack[s].indexA;
            if (!does_sumset_contain(stack[s].b, i)) {
                Sumset* a_with_i = malloc(sizeof(Sumset));
                if (!a_with_i) exit(1); // Błąd alokacji
                sumset_add(a_with_i, stack[s].a, i);
                stack[stack_size++] = (StackFrame){
                    .a = a_with_i,
                    .b = stack[s].b,
                    .indexA = i,
                    .indexB = stack[s].indexB,
                    .firstA = true,
                    .firstB = false
                };
            }
        } else if ((stack[s].a->sum == stack[s].b->sum) &&
                   (get_sumset_intersection_size(stack[s].a, stack[s].b) == 2)) {
            if (stack[s].b->sum > best_solution->sum) {
                solution_build(best_solution, input_data, stack[s].a, stack[s].b);
            }
                   }
        stack[s].indexA++;
    }
    free(stack);
}



int main()
{
    InputData input_data;
    input_data_read(&input_data);

    Solution best_solution;
    solution_init(&best_solution);

    solve_iterative(&input_data, &best_solution);
    solution_print(&best_solution);

    return 0;
}


