#include <stddef.h>

#include "common/io.h"
#include "common/sumset.h"
typedef struct {
    Sumset *a, *b;
    bool first;     // Flaga wskazująca, czy jest to pierwszy raz przetwarzania tej ramki
} StackFrame;

void solve_iterative(InputData* input_data, Solution* best_solution) {
    size_t stack_size = 0;
    // Rozmiar stosu 1024 został empirycznie sprawdzony i jest wystarczający dla danych wejściowych.
    StackFrame stack[1024];
    Sumset sumsetStack[1024];
    sumsetStack[0] = input_data->a_start;
    sumsetStack[1] = input_data->b_start;
    const size_t d = input_data->d;
    stack[stack_size++] = (StackFrame){
        .a = &sumsetStack[0],
        .b = &sumsetStack[1],
        .first = true
    };
    while (stack_size > 0) {

        const size_t s = stack_size - 1;    // Indeks ostatniej ramki na stosie
        // Jeśli ramka została już przetworzona, usuń ją ze stosu
        if (!stack[s].first) {
            --stack_size;
            continue;
        }
        stack[s].first = false;

        if (stack[s].a->sum > stack[s].b->sum) {
            Sumset* temp = stack[s].a;
            stack[s].a = stack[s].b;
            stack[s].b = temp;
        }

        if (is_sumset_intersection_trivial(stack[s].a, stack[s].b)) {
            for (size_t i = stack[s].a->last; i <= d; ++i) {
                if (!does_sumset_contain(stack[s].b, i)) {
                    sumset_add(&sumsetStack[stack_size + 1], stack[s].a, i);
                    stack[stack_size++] = (StackFrame){
                        .a = &sumsetStack[stack_size],
                        .b = stack[s].b,
                        .first = true
                    };
                }
            }
        } else if ((stack[s].a->sum == stack[s].b->sum) && (get_sumset_intersection_size(stack[s].a, stack[s].b) == 2) && stack[s].b->sum > best_solution->sum)
                solution_build(best_solution, input_data, stack[s].a, stack[s].b);
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
