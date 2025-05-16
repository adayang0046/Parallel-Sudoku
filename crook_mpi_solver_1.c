#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mpi.h>

#define SIZE 9

int grid[SIZE][SIZE];
int candidates[SIZE][SIZE][SIZE + 1];

int is_valid(int g[SIZE][SIZE], int row, int col, int num) {
    for (int i = 0; i < SIZE; i++) {
        if (g[row][i] == num || g[i][col] == num)
            return 0;
    }
    int boxRow = (row / 3) * 3;
    int boxCol = (col / 3) * 3;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (g[boxRow + i][boxCol + j] == num)
                return 0;
    return 1;
}

void initialize_candidates() {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            for (int n = 1; n <= SIZE; n++) {
                candidates[i][j][n] = (grid[i][j] == 0 && is_valid(grid, i, j, n)) ? 1 : 0;
            }
        }
    }
}

int apply_naked_singles() {
    int changed = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (grid[i][j] != 0) continue;
            int count = 0, lastNum = 0;
            for (int n = 1; n <= SIZE; n++) {
                if (candidates[i][j][n]) {
                    count++;
                    lastNum = n;
                }
            }
            if (count == 1) {
                grid[i][j] = lastNum;
                changed = 1;
                initialize_candidates();
            }
        }
    }
    return changed;
}

int apply_hidden_singles() {
    int changed = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int n = 1; n <= SIZE; n++) {
            int count = 0, pos = -1;
            for (int j = 0; j < SIZE; j++) {
                if (grid[i][j] == 0 && candidates[i][j][n]) {
                    count++;
                    pos = j;
                }
            }
            if (count == 1) {
                grid[i][pos] = n;
                changed = 1;
                initialize_candidates();
            }
        }
    }
    return changed;
}

void solve_with_logic() {
    initialize_candidates();
    while (apply_naked_singles() || apply_hidden_singles());
}

int solve_fallback() {
    solve_with_logic();
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (grid[i][j] == 0) {
                for (int n = 1; n <= SIZE; n++) {
                    if (is_valid(grid, i, j, n)) {
                        grid[i][j] = n;
                        if (solve_fallback())
                            return 1;
                        grid[i][j] = 0;
                    }
                }
                return 0;
            }
        }
    }
    return 1;
}

void print_grid() {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++)
            printf("%d ", grid[i][j]);
        printf("\n");
    }
}

void load_puzzle_string(const char* puzzle, int g[SIZE][SIZE]) {
    for (int i = 0; i < SIZE * SIZE; i++) {
        g[i / SIZE][i % SIZE] = puzzle[i] - '0';
    }
}

int load_puzzle_file(const char* filename, int g[SIZE][SIZE]) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return 0;

    char buffer[200];
    int index = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        for (int i = 0; buffer[i] && index < SIZE * SIZE; i++) {
            if (isdigit(buffer[i])) {
                g[index / SIZE][index % SIZE] = buffer[i] - '0';
                index++;
            }
        }
    }
    fclose(fp);
    return (index == SIZE * SIZE);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int local_solution_found = 0;
    int global_solution_found = 0;

    //Start the timer
    double start_time = MPI_Wtime();

    if (argc < 2) {
        if (rank == 0)
            fprintf(stderr, "Usage: %s <sudoku_file.txt>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    int initial_grid[SIZE][SIZE];
    if (!load_puzzle_file(argv[1], initial_grid)) {
        if (rank == 0)
            fprintf(stderr, "Failed to load puzzle from file: %s\n", argv[1]);
        MPI_Finalize();
        return 1;
    }

    int row = -1, col = -1;
    for (int i = 0; i < SIZE && row == -1; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (initial_grid[i][j] == 0) {
                row = i;
                col = j;
                break;
            }
        }
    }

    int guess_count = 0;
    for (int n = 1; n <= SIZE && !local_solution_found; n++) {
        if (is_valid(initial_grid, row, col, n)) {
            if (guess_count % size == rank) {
                memcpy(grid, initial_grid, sizeof(grid));
                grid[row][col] = n;
                if (solve_fallback()) {
                    double end_time = MPI_Wtime();
                    local_solution_found = 1;
                    printf("[Rank %d] Solution found with guess %d at (%d,%d):\n", rank, n, row, col);
                    print_grid();
                    printf("[Rank %d] Elapsed time: %.6f seconds\n", rank, end_time - start_time);
                }
            }
            guess_count++;
        }
    }

    // Aggregate: If any rank found solution, all know
    MPI_Allreduce(&local_solution_found, &global_solution_found, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();
    if (!global_solution_found && rank == 0) {
        printf("[Rank 0] No solution found. Time elapsed: %.6f seconds\n", end_time - start_time);
    }

    MPI_Finalize();
    return 0;
}

