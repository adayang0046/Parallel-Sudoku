#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>  // add this

#define N 9

// Print the Sudoku board
void printBoard(int board[N][N]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++)
            printf("%d ", board[i][j]);
        printf("\n");
    }
}

// Check if it's safe to place num in board[row][col]
bool isSafe(int board[N][N], int row, int col, int num) {
    for (int x = 0; x < N; x++)
        if (board[row][x] == num || board[x][col] == num)
            return false;

    int startRow = row - row % 3, startCol = col - col % 3;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (board[i + startRow][j + startCol] == num)
                return false;

    return true;
}

// Recursive solver
bool solveSudoku(int board[N][N]) {
    int row, col;
    bool empty = false;

    for (row = 0; row < N; row++) {
        for (col = 0; col < N; col++) {
            if (board[row][col] == 0) {
                empty = true;
                break;
            }
        }
        if (empty) break;
    }

    if (!empty)
        return true;

    for (int num = 1; num <= 9; num++) {
        if (isSafe(board, row, col, num)) {
            board[row][col] = num;
            if (solveSudoku(board))
                return true;
            board[row][col] = 0; // backtrack
        }
    }

    return false;
}

// Read board from puzzle.txt
bool readPuzzleFromFile(const char *filename, int board[N][N]) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open puzzle.txt");
        return false;
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (fscanf(file, "%d", &board[i][j]) != 1) {
                fclose(file);
                fprintf(stderr, "Invalid format in puzzle.txt\n");
                return false;
            }
        }
    }

    fclose(file);
    return true;
}



int main() {
    int board[N][N];

    if (!readPuzzleFromFile("puzzle.txt", board)) {
        return 1;
    }

    struct timeval start, end;
    gettimeofday(&start, NULL);

    if (solveSudoku(board)) {
        gettimeofday(&end, NULL);

        long seconds = end.tv_sec - start.tv_sec;
        long microseconds = end.tv_usec - start.tv_usec;
        double elapsed = seconds + microseconds * 1e-6;

        printf("Sudoku Solved:\n");
        printBoard(board);
        printf("\nTime taken: %.6f seconds\n", elapsed);
    } else {
        printf("No solution exists.\n");
    }

    return 0;
}

