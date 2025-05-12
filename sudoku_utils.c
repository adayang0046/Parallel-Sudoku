#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void load_sudoku(const char* filename, int grid[9][9]) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open puzzle file");
        exit(1);
    }
    for (int i = 0; i < 9; i++) {
        char line[10];
        fscanf(fp, "%s", line);
        for (int j = 0; j < 9; j++) {
            grid[i][j] = line[j] - '0';
        }
    }
    fclose(fp);
}

void print_grid(int grid[9][9]) {
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            printf("%d ", grid[i][j]);
        }
        printf("\n");
    }
}

int is_safe(int grid[9][9], int row, int col, int num) {
    for (int x = 0; x < 9; x++) {
        if (grid[row][x] == num || grid[x][col] == num)
            return 0;
    }
    int startRow = row - row % 3;
    int startCol = col - col % 3;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (grid[i + startRow][j + startCol] == num)
                return 0;
    return 1;
}

int solve_sudoku(int grid[9][9]) {
    int row = -1, col = -1;
    int empty = 0;
    for (int i = 0; i < 9 && !empty; i++) {
        for (int j = 0; j < 9; j++) {
            if (grid[i][j] == 0) {
                row = i;
                col = j;
                empty = 1;
                break;
            }
        }
    }
    if (!empty)
        return 1;

    for (int num = 1; num <= 9; num++) {
        if (is_safe(grid, row, col, num)) {
            grid[row][col] = num;
            if (solve_sudoku(grid))
                return 1;
            grid[row][col] = 0;
        }
    }
    return 0;
}

typedef struct {
    int grid[9][9];
    int solved;
} SudokuTask;

int generate_initial_tasks(int grid[9][9], SudokuTask tasks[]) {
    int row = -1, col = -1;
    for (int i = 0; i < 9 && row == -1; i++) {
        for (int j = 0; j < 9; j++) {
            if (grid[i][j] == 0) {
                row = i;
                col = j;
                break;
            }
        }
    }
    if (row == -1)
        return 0;

    int count = 0;
    for (int num = 1; num <= 9; num++) {
        if (is_safe(grid, row, col, num)) {
            memcpy(tasks[count].grid, grid, sizeof(int) * 81);
            tasks[count].grid[row][col] = num;
            tasks[count].solved = 0;
            count++;
        }
    }
    return count;
}
