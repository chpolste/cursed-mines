#ifndef HEADER_MINES
#define HEADER_MINES

#include <stdlib.h>


/* Utilities */

void seed_random();


/* Board-related */

typedef enum Cell {
    MINE,
    EMPTY
} Cell;

typedef struct Board {
    size_t nx;
    size_t ny;
    size_t nmines;
    Cell *cells;
} Board;

int initialize_board(Board*, size_t, size_t, size_t);
void clear_board(Board*);
void make_safe(Board*, size_t, size_t);
int board_cell_status(Board*, size_t, size_t);


/* Game-related */

typedef enum State {
    UNDECIDED,
    LOST,
    WON
} State;

typedef enum Status {
    STATUS_FLAG    = -2,
    STATUS_UNKNOWN = -1,
    STATUS_ZERO    =  0,
    STATUS_ONE     =  1,
    STATUS_TWO     =  2,
    STATUS_THREE   =  3,
    STATUS_FOUR    =  4,
    STATUS_FIVE    =  5,
    STATUS_SIX     =  6,
    STATUS_SEVEN   =  7,
    STATUS_EIGHT   =  8,
    STATUS_MINE    =  9
} Status;

typedef struct Game {
    State state;
    size_t nx;
    size_t ny;
    size_t nopen;
    Board *board;
    Status *cells;
} Game;

int initialize_game(Game*, Board*);
void clear_game(Game*);
void toggle_flag(Game*, size_t, size_t);
void uncover(Game*, size_t, size_t);
void uncover_adjacent(Game*, size_t, size_t);
void _set_all_mines(Game*, Status);


#endif

