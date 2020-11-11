#include <stdio.h>
#include <stdlib.h>
#include "BubbleBlast.h"
#include <time.h>
#include <libnet.h>

int getRandomState()
{
    return (enum BubbleStates)(rand() % bubbleStatesCount);
}

char getVisualCharForBubbleState(enum BubbleStates bubbleState)
{
    switch (bubbleState)
    {
        case stateEmpty: return ' ';
        case stateHalf: return 'O';
        case stateFull: return '0';
        case stateExploded: return 'X';
    }
}

void cleanInputBuffer()
{
    char c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int createPlayingField(int gameField[FIELD_ROWS][FIELD_COLUMNS])
{
    for (int r = 0; r < FIELD_ROWS; r++)
    {
        for (int c = 0; c < FIELD_COLUMNS; c++)
        {
            gameField[r][c] = getRandomState();
        }
    }
}

void printGameFieldToPlayer(int gameField[FIELD_ROWS][FIELD_COLUMNS])
{
    //system("cls");

    for (int c = 0; c < FIELD_COLUMNS; c++)
    {
        if (c == 0) printf("  ");
        printf("%d ", c + 1);
    }
    printf("\n");

    for (int r = 0; r < FIELD_ROWS; r++)
    {
        printf("%d ", r + 1);

        for (int c = 0; c < FIELD_COLUMNS; c++)
        {
            printf("%c ", getVisualCharForBubbleState(gameField[r][c]));
        }
        printf("\n");
    }
    printf("\n");
}

void askPlayerNextMoveCoords(int *x, int *y, int gameField[FIELD_ROWS][FIELD_COLUMNS])
{
    int xTemp = *x;
    int yTemp = *y;

    do
    {
        while (yTemp == -1)
        {
            printf("Seleziona una riga [1-%d]", FIELD_ROWS);
            scanf("%d", &yTemp);
            cleanInputBuffer();

            if (yTemp < 1 || yTemp > FIELD_ROWS)
            {
                yTemp = -1;
                printf("Non e' una riga valida!\n");
            }
            else
            {
                yTemp--;
            }
        }

        while (xTemp == -1)
        {
            printf("Seleziona una colonna [1-%d]", FIELD_COLUMNS);
            scanf("%d", &xTemp);
            cleanInputBuffer();

            if (xTemp < 1 || xTemp > FIELD_COLUMNS)
            {
                xTemp = -1;
                printf("Non e' una colonna valida!\n");
            }
            else
            {
                xTemp--;
            }
        }

        if (gameField[yTemp][xTemp] == stateExploded)
        {
            xTemp = -1;
            yTemp = -1;
            printf("Non puoi scegliere una bolla gia' esplosa!\n");
        }
    } while (gameField[yTemp][xTemp] == stateExploded);

    *x = xTemp;
    *y = yTemp;
}

struct BubbleExplosionDeltas calcBubbleExplosionDeltas(int x, int y, int explosionPropagationDirection)
{
    struct BubbleExplosionDeltas deltas;
    deltas.dx = x + (explosionPropagationDirection == directionLeft ? -1 : explosionPropagationDirection == directionRight ? 1 : 0);
    deltas.dy = y + (explosionPropagationDirection == directionUp ? -1 : explosionPropagationDirection == directionDown ? 1 : 0);
    return deltas;
}

void updateBubbleState(int gameField[FIELD_ROWS][FIELD_COLUMNS], int x, int y, int explosionPropagationDirection)
{
    if (x < 0 || y < 0 || x >= FIELD_COLUMNS || y >= FIELD_ROWS)
    {
        return;
    }

    struct BubbleExplosionDeltas deltas;

    if (gameField[y][x] == stateExploded)
    {
        if (explosionPropagationDirection != -1)
        {
            deltas = calcBubbleExplosionDeltas(x, y, explosionPropagationDirection);
            updateBubbleState(gameField, deltas.dx, deltas.dy, explosionPropagationDirection);
        }
    }
    else if (gameField[y][x] == stateFull)
    {
        gameField[y][x] = stateExploded;

        for (int i = 0; i < 4; i++)
        {
            enum BubbleExplosionDirectionPropagation directionPropagation = (enum BubbleExplosionDirectionPropagation)i;
            deltas = calcBubbleExplosionDeltas(x, y, directionPropagation);
            updateBubbleState(gameField, deltas.dx, deltas.dy, directionPropagation);
        }
    }
    else
    {
        gameField[y][x] += 1;
    }
}

char* saveMovesStatus(int movesField[FIELD_ROWS][FIELD_COLUMNS])
{
    int fieldSize = sizeof(int) * FIELD_ROWS * FIELD_COLUMNS;
    char* movesStatusRef = malloc(fieldSize);
    memcpy(movesStatusRef, movesField, fieldSize);
    return movesStatusRef;
}

void restoreCalcMovesStatus(int movesField[FIELD_ROWS][FIELD_COLUMNS], char* buffer)
{
    int dim = sizeof(int) * FIELD_ROWS * FIELD_COLUMNS;
    memcpy(movesField, buffer, dim);
    free(buffer);
}

int isGameCompleted(int gameField[FIELD_ROWS][FIELD_COLUMNS])
{
    for (int r = 0; r < FIELD_ROWS; r++)
    {
        for (int c = 0; c < FIELD_COLUMNS; c++)
        {
            if (gameField[r][c] != stateExploded)
            {
                return statusPlaying;
            }
        }
    }

    return statusWon;
}

void findBestPath(FILE *movesFile, int *numMoves, int *minMoves, int x, int y, int fieldRef[FIELD_ROWS][FIELD_COLUMNS], struct MovesStackStatus movesStatusStack[1000])
{
    if (*numMoves >= *minMoves - 1 || fieldRef[y][x] == stateExploded)
    {
        return;
    }

    char* movesStatusRef = saveMovesStatus(fieldRef);
    movesStatusStack[*numMoves].x = x;
    movesStatusStack[*numMoves].y = y;

    *numMoves += 1;

    updateBubbleState(fieldRef, x, y, noDirection);

    if(isGameCompleted(fieldRef))
    {
        if (*minMoves > *numMoves)
        {
            *minMoves = *numMoves;
        }

        fprintf(movesFile, "Finito ramo con %d mosse totali:\n", *numMoves);

        for (int i = 0; i < *numMoves; i++)
        {
            fprintf(movesFile, "%d: [%d][%d]\n", i + 1, movesStatusStack[i].y + 1, movesStatusStack[i].x + 1);
        }
        fprintf(movesFile, "\n");

        numMoves--;
        restoreCalcMovesStatus(fieldRef, movesStatusRef);

        return;
    }

    for (int r = 0; r < FIELD_ROWS; r++)
    {
        for (int c = 0; c < FIELD_COLUMNS; c++)
        {
            findBestPath(movesFile, numMoves, minMoves, c, r, fieldRef, movesStatusStack);
        }
    }

    *numMoves -= 1;
    restoreCalcMovesStatus(fieldRef, movesStatusRef);
}

int calcMinMovesForGameField(int gameField[FIELD_ROWS][FIELD_COLUMNS])
{
    int minMoves = INT_MAX;

    //Opens a file where write the moves search
    FILE *movesFile = fopen("moves.txt", "w");

    //Creates a pointer to a memory space containing a copy of the playing field
    int* fieldRef = malloc(sizeof(int) * FIELD_ROWS * FIELD_COLUMNS);
    memcpy(fieldRef, gameField, sizeof(int) * FIELD_ROWS * FIELD_COLUMNS);

    if (movesFile == NULL)
    {
        printf("Impossibile aprire il file %s\n", "moves.txt");
        getchar();
    }

    fprintf(movesFile, "Algoritmo per il calcolo del numero minimo di mosse necessario a completare la seguente griglia:\n\n");

    //---START WRITING THE PLAYING FIELD INTO moves.txt---
    for (int c = 0; c < FIELD_COLUMNS; c++)
    {
        if (c == 0) fprintf(movesFile, "   ");
        fprintf(movesFile, "%d ", c + 1);
    }
    fprintf(movesFile, "\n");

    for (int r = 0; r < FIELD_ROWS; r++)
    {
        fprintf(movesFile, "%d  ", r + 1);

        for (int c = 0; c < FIELD_COLUMNS; c++)
        {

            fprintf(movesFile, "%c ", getVisualCharForBubbleState((enum BubbleStates)*(fieldRef + (r * FIELD_ROWS) + c)));
        }
        fprintf(movesFile, "\n");
    }
    fprintf(movesFile, "\n");
    //---FINISH---

    //calc best path to win
    struct MovesStackStatus movesStatusStack[1000];
    for (int r = 0; r < FIELD_ROWS; r++)
    {
        for (int c = 0; c < FIELD_COLUMNS; c++)
        {
            fprintf(movesFile, "-------------------------------\n\nCalcolo rami possibili partendo da [%d][%d]\n\n", r + 1, c + 1);
            int numMoves = 0;
            findBestPath(movesFile, &numMoves, &minMoves, c, r, fieldRef, &movesStatusStack);
        }
    }

    fprintf(movesFile, "===============================\n\nIl ramo più efficiente impiega %d mosse per completare la griglia.", minMoves);

    fflush(movesFile);
    fclose(movesFile);
    free(fieldRef);

    return minMoves;
}

int main()
{
    //Initialize random generation system
    srand(time(0));
    int numMoves = 0, status = 0;
    int gameField[FIELD_ROWS][FIELD_COLUMNS];

    createPlayingField(gameField);
    int minMoves = calcMinMovesForGameField(gameField);

    while (status == 0)
    {
        printGameFieldToPlayer(gameField);

        printf("Mosse rimanenti: %d\n\n", minMoves - numMoves);

        int x = -1, y = -1;
        askPlayerNextMoveCoords(&x, &y, gameField);

        updateBubbleState(gameField, x, y, noDirection);
        numMoves++;

        status = isGameCompleted(gameField);
        if (status == 0 && numMoves >= minMoves)
        {
            status = statusLost;
        }
    }

    printGameFieldToPlayer(gameField);
    printf("\n%s\n", (status == 1 ? "HAI VINTO" : "HAI PERSO"));
    getchar();
}
