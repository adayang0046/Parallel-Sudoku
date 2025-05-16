#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define SIZE 9
#define TAG_PUZZLE 1
#define TAG_DONE   2

typedef struct {
    int grid[SIZE][SIZE];
} Puzzle;

void read_puzzle(const char *filename, Puzzle *p) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening puzzle file");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            fscanf(fp, "%d", &p->grid[i][j]);
    fclose(fp);
}

void print_puzzle(const Puzzle *p) {
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j)
            printf("%d ", p->grid[i][j]);
        printf("\n");
    }
}

int is_valid(const Puzzle *p, int row, int col, int num) {
    for (int i = 0; i < SIZE; ++i) {
        if (p->grid[row][i] == num || p->grid[i][col] == num)
            return 0;
    }
    int startRow = row / 3 * 3;
    int startCol = col / 3 * 3;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            if (p->grid[startRow + i][startCol + j] == num)
                return 0;
    return 1;
}

int is_solved(const Puzzle *p) {
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            if (p->grid[i][j] == 0)
                return 0;
    return 1;
}

int solve(Puzzle *p) {
    for (int row = 0; row < SIZE; ++row) {
        for (int col = 0; col < SIZE; ++col) {
            if (p->grid[row][col] == 0) {
                for (int num = 1; num <= 9; ++num) {
                    if (is_valid(p, row, col, num)) {
                        p->grid[row][col] = num;
                        if (solve(p)) return 1;
                        p->grid[row][col] = 0;
                    }
                }
                return 0; // no valid number
            }
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    int rank, size;
    Puzzle puzzle;
    double start_time, end_time;


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        read_puzzle("puzzle.txt", &puzzle);
        start_time = MPI_Wtime(); 

        if (is_solved(&puzzle)) {
            printf("Already solved:\n");
            print_puzzle(&puzzle);
            end_time = MPI_Wtime();
            printf("Runtime: %.6f seconds\n", end_time - start_time);
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        // Find first empty cell
        int empty_row = -1, empty_col = -1;
        for (int i = 0; i < SIZE && empty_row == -1; ++i) {
            for (int j = 0; j < SIZE; ++j) {
                if (puzzle.grid[i][j] == 0) {
                    empty_row = i;
                    empty_col = j;
                    break;
                }
            }
        }

        int sent = 0;
        for (int num = 1; num <= 9 && sent < size - 1; ++num) {
            if (is_valid(&puzzle, empty_row, empty_col, num)) {
                Puzzle new_p = puzzle;
                new_p.grid[empty_row][empty_col] = num;
                MPI_Send(&new_p, sizeof(Puzzle), MPI_BYTE, ++sent, TAG_PUZZLE, MPI_COMM_WORLD);
            }
        }

        Puzzle result;
        MPI_Status status;
        MPI_Recv(&result, sizeof(Puzzle), MPI_BYTE, MPI_ANY_SOURCE, TAG_DONE, MPI_COMM_WORLD, &status);
        end_time = MPI_Wtime();  // Stop timer
        printf("Runtime: %.6f seconds\n", end_time - start_time);
        printf("Solved by worker %d:\n", status.MPI_SOURCE);
        print_puzzle(&result);

        // Stop other workers
        for (int i = 1; i < size; ++i)
            if (i != status.MPI_SOURCE)
                MPI_Send(NULL, 0, MPI_BYTE, i, TAG_DONE, MPI_COMM_WORLD);
    } else {
        MPI_Status status;
        MPI_Recv(&puzzle, sizeof(Puzzle), MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == TAG_PUZZLE) {
            if (solve(&puzzle)) {
                MPI_Send(&puzzle, sizeof(Puzzle), MPI_BYTE, 0, TAG_DONE, MPI_COMM_WORLD);
            }
        }
    }

    MPI_Finalize();
    return 0;
}
