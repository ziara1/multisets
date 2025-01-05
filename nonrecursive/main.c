#include <stddef.h>
#include "common/io.h"
#include "common/sumset.h"

typedef struct {
    Sumset *a, *b;
    size_t indexA, indexB;
    size_t onStack;
    bool trivial, elif;
} StackFrame;

void solve_iterative(InputData* input_data, Solution* best_solution) {
    size_t stack_size = 0;
    size_t sumsetStack_size = 2;
    StackFrame stack[512];
    Sumset sumsetStack[512];
    sumsetStack[0] = input_data->a_start;
    sumsetStack[1] = input_data->b_start;
    stack[stack_size++] = (StackFrame){
        .a = &sumsetStack[0],
        .b = &sumsetStack[1],
        .indexA = input_data->a_start.last,
        .indexB = input_data->b_start.last,
        .onStack = 2,
        .trivial = false,
        .elif = false
    };

    while (stack_size > 0) {
        const size_t s = stack_size - 1;
        if (stack[s].a->sum > stack[s].b->sum) {
            Sumset* temp = stack[s].a;
            stack[s].a = stack[s].b;
            stack[s].b = temp;
            size_t temp_index = stack[s].indexA;
            stack[s].indexA = stack[s].indexB;
            stack[s].indexB = temp_index;
        }

        if (stack[s].indexA > input_data->d) {
            --stack_size;
            sumsetStack_size -= stack[s].onStack;
            continue;
        }
        if (stack[s].trivial || is_sumset_intersection_trivial(stack[s].a, stack[s].b)) {
            stack[s].trivial = true;
            const size_t i = stack[s].indexA;
            if (!does_sumset_contain(stack[s].b, i)) {
                sumset_add(&sumsetStack[sumsetStack_size++], stack[s].a, i);
                stack[stack_size++] = (StackFrame){
                    .a = &sumsetStack[sumsetStack_size - 1],
                    .b = stack[s].b,
                    .indexA = i,
                    .indexB = stack[s].indexB,
                    .onStack = 1,
                    .trivial = false,
                    .elif = false
                };
            }
        } else if (stack[s].elif || ((stack[s].a->sum == stack[s].b->sum) && (get_sumset_intersection_size(stack[s].a, stack[s].b) == 2))) {
            stack[s].elif = true;
            if (stack[s].b->sum > best_solution->sum) {
                solution_build(best_solution, input_data, stack[s].a, stack[s].b);
            }
        }
        stack[s].indexA++;
    }
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
