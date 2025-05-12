/* sudoku_mpi.c - MPI-based Crook's Sudoku Solver */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sudoku_utils.h"

#define TAG_WORK 1
#define TAG_RESULT 2
#define TAG_STOP 3
#define MAX_TASKS 81

typedef struct {
    int grid[9][9];
    int solved;
} SudokuTask;

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    SudokuTask task;
    int grid[9][9];

    if (rank == 0) {
        // MASTER PROCESS
        load_sudoku("puzzle.txt", grid);

        SudokuTask tasks[MAX_TASKS];
        int task_count = generate_initial_tasks(grid, tasks);

        int task_index = 0;
        int active_workers = 0;
        SudokuTask result;

        // Distribute initial tasks
        for (int i = 1; i < size && task_index < task_count; i++) {
            MPI_Send(&tasks[task_index], sizeof(SudokuTask), MPI_BYTE, i, TAG_WORK, MPI_COMM_WORLD);
            task_index++;
            active_workers++;
        }

        while (active_workers > 0) {
            MPI_Status status;
            MPI_Recv(&result, sizeof(SudokuTask), MPI_BYTE, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD, &status);

            if (result.solved) {
                printf("Sudoku Solved by process %d:\n", status.MPI_SOURCE);
                print_grid(result.grid);

                // Abort all other workers
                for (int i = 1; i < size; i++) {
                    if (i != status.MPI_SOURCE)
                        MPI_Send(NULL, 0, MPI_BYTE, i, TAG_STOP, MPI_COMM_WORLD);
                }
                break;
            }

            if (task_index < task_count) {
                MPI_Send(&tasks[task_index], sizeof(SudokuTask), MPI_BYTE, status.MPI_SOURCE, TAG_WORK, MPI_COMM_WORLD);
                task_index++;
            } else {
                MPI_Send(NULL, 0, MPI_BYTE, status.MPI_SOURCE, TAG_STOP, MPI_COMM_WORLD);
                active_workers--;
            }
        }
    } else {
        // WORKER PROCESS
        MPI_Status status;
        while (1) {
            MPI_Recv(&task, sizeof(SudokuTask), MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == TAG_STOP) break;

            if (solve_sudoku(task.grid)) {
                task.solved = 1;
            } else {
                task.solved = 0;
            }

            MPI_Send(&task, sizeof(SudokuTask), MPI_BYTE, 0, TAG_RESULT, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
