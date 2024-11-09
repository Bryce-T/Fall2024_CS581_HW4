/*
Bryce Taylor
bktaylor2@crimson.ua.edu
CS 581
Homework #4

To compile:
mpicc -Wall -o homework4 homework4.c

To run:
mpirun -np (# processes) ./homework4 (Size of board) (Max generations) (Output file directory)
mpirun -np 8 ./homework4 5000 5000 outputs
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

    MPI_Init(&argc, &argv); // Start parallelized code

    int numRanks; // Number of ranks in total
    MPI_Comm_size(MPI_COMM_WORLD, &numRanks);
    int rank; // Rank of current process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int boardSize = atoi(argv[1]); // Board size (N)
    int maxGenerations = atoi(argv[2]); // Max # of iterations
    char* outputDir = argv[3]; // Directory to write output file to

    srand(0); // Set random seed (same value each time)

    int* counts = malloc(numRanks * sizeof(int)); // # elements sent/received by process
    int* displacements = malloc(numRanks * sizeof(int)); // Displacement of process start

    // Calculate counts
    for (int i = 0; i < numRanks; i++) {
        counts[i] = boardSize / numRanks;
        if (i < (boardSize % numRanks)) {
            counts[i] += 1;
        }
        counts[i] *= boardSize;
    }
    // Calculate displacements
    displacements[0] = 0;
    for (int i = 1; i < numRanks; i++) {
        displacements[i] = counts[i-1] + displacements[i-1];
    }

    // Allocate data for current process
    int numRows = counts[rank] / boardSize; // Number of rows used by current rank
    int totalRows = numRows + 2; // Number of rows adding in ghosts cells
    int** curBoard = malloc(totalRows * sizeof(int*)); // Process board current state
    int** newBoard = malloc(totalRows * sizeof(int*)); // Process board new state
    for (int i = 0; i < totalRows; i++) {
        curBoard[i] = malloc((boardSize + 2) * sizeof(int));
        newBoard[i] = malloc((boardSize + 2) * sizeof(int));
    }
    int* localBoardData = malloc(boardSize * numRows * sizeof(int)); // To store local process board data
    int* boardData = NULL;

    int count = counts[rank];

    // Process 0 creates the entire board and calls MPI_Scatter
    if (rank == 0) {
        // Initialize full board in 1D
        boardData = malloc(boardSize * boardSize * sizeof(int));
        for (int i = 0; i < boardSize * boardSize; i++) {
            boardData[i] = rand() % 2;
        }

        MPI_Scatterv(boardData, counts, displacements, MPI_INT, localBoardData, count, MPI_INT, 0, MPI_COMM_WORLD);
    }
    // Other process receive their part of the board
    else {
        MPI_Scatterv(NULL, counts, displacements, MPI_INT, localBoardData, count, MPI_INT, 0, MPI_COMM_WORLD);
    }

    // Initialize local boards with all 0's
    for (int i = 0; i < totalRows; i++) {
        for (int j = 0; j < boardSize + 2; j++) {
            curBoard[i][j] = 0;
            newBoard[i][j] = 0;
        }
    }

    // Fill in inside of current board with received data
    for (int i = 1; i < totalRows - 1; i++) {
        for (int j = 1; j < boardSize + 1; j++) {
            curBoard[i][j] = localBoardData[(i - 1) * boardSize + j - 1];
        }
    }

    int isChanged = 0; // Track whether there are changes from one generation to the next
    int boardsSame = 0; // Tracks if entire board is the same

    // Main algorithm loop
    for (int n = 1; n <= maxGenerations; n++) {
        isChanged = 0;
        int rank_above = rank - 1;
        if (rank_above < 0) {
            rank_above = MPI_PROC_NULL;
        }
        int rank_below = rank + 1;
        if (rank_below > numRanks - 1) {
            rank_below = MPI_PROC_NULL;
        }
        // First set of ghost cell exchanges tagged 0
        MPI_Sendrecv(curBoard[1], boardSize + 2, MPI_INT, rank_above, 0,
                     curBoard[numRows+1], boardSize + 2, MPI_INT, rank_below, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // Second set of ghost cell exchanges tagged 1
        MPI_Sendrecv(curBoard[numRows], boardSize + 2, MPI_INT, rank_below, 1,
                     curBoard[0], boardSize + 2, MPI_INT, rank_above, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        // For local board, calculate which cells will live or die
        for (int i = 1; i < numRows + 1; i++) {
            for (int j = 1; j < boardSize + 1; j++) {
                // Calculate number of alive neighbors this cell has
                int numNeighbors = 0;
                numNeighbors += curBoard[i-1][j-1] + curBoard[i-1][j] + curBoard[i-1][j+1] + \
                                curBoard[i+1][j-1] + curBoard[i+1][j] + curBoard[i+1][j+1] + \
                                curBoard[i][j-1] + curBoard[i][j+1];
                if (curBoard[i][j] == 0) { // If cell is dead
                    // 3 neighbors means it becomes alive
                    if (numNeighbors == 3) {
                        newBoard[i][j] = 1;
                    }
                    // Otherwise cell is still dead
                    else {
                        newBoard[i][j] = 0;
                    }
                }
                else if (curBoard[i][j] == 1) { // If cell is alive
                    // 0 or 1 neighbors dies
                    if (numNeighbors < 2) {
                        newBoard[i][j] = 0;
                    }
                    // 4 or more neighbors dies
                    else if (numNeighbors > 3) {
                        newBoard[i][j] = 0;
                    }
                    // 2 or 3 neighbors survives
                    else {
                        newBoard[i][j] = 1;
                    }
                }
            }
        }

        // Update current board with new board
        for (int i = 1; i < totalRows - 1; i++) {
            for (int j = 1; j <= boardSize; j++) {
                if (curBoard[i][j] != newBoard[i][j]) {
                    isChanged = 1;
                }
                curBoard[i][j] = newBoard[i][j];
            }
        }

        // If entire board is the same, exit program
        MPI_Allreduce(&isChanged, &boardsSame, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        if (boardsSame == 0) {
            if (rank == 0) {
                printf("Exiting on iteration %d due to zero changes.\n", n);
            }
            break;
        }

        // Make sure all processes on track before entering next iteration
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Collect final board state for each process
    int* finalData = malloc(numRows * boardSize * sizeof(int));
    for (int i = 1; i < totalRows - 1; i++) {
        for (int j = 1; j < boardSize + 1; j++) {
            finalData[(i - 1) * boardSize + j - 1] = curBoard[i][j];
        }
    }
    int* finalBoard = NULL;
    if (rank == 0) {
        finalBoard = malloc(boardSize * boardSize * sizeof(int));
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // Collect final board data to output
    MPI_Gatherv(finalData, numRows * boardSize, MPI_INT, finalBoard, counts, displacements, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        // Write output to a file
        char outputFileName[100];
        snprintf(outputFileName, sizeof(outputFileName), "%s/output.txt", outputDir);

        FILE *file = fopen(outputFileName, "w");
        for (int i = 0; i < boardSize * boardSize; i++) {
            fprintf(file, "%d ", finalBoard[i]);
            if ((i + 1) % boardSize == 0) {
                fprintf(file, "\n");
            }
        }
    }

    MPI_Finalize(); // End parallelized code

    return 0;
}
