#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "mines.h"

// Access Cell I of Board or Game BG
#define cell_idx(BG, I) ((BG)->cells[(I)])
// Access Cell (X, Y) of Board or Game BG
#define cell_xy(BG, X, Y) cell_idx((BG), (X) + (Y) * (BG)->nx)


/* Utilities */

void seed_random() {
    srandom((unsigned int) time(NULL));
}

// A random array index in [0, n)
size_t random_index(size_t n) {
    const size_t end = (RAND_MAX / n) * n;
    size_t r;
    do {
        r = (size_t) random();
    } while (r >= end);
    return r % n;
}

void fill_with_shuffled_indices(size_t *arr, size_t n) {
    // Fill with indices [0, n)
    for (size_t i = 0; i < n; i++) arr[i] = i;
    // Fisher-Yates shuffle
    for (size_t i = (n - 1); i > 0; i--) {
        const size_t j = random_index(i + 1);
        size_t temp = arr[j];
        arr[j] = arr[i];
        arr[i] = temp;
    }
}


/* Game operations */

int initialize_game(Game *game, Board *board) {
    const size_t xy = board->nx * board->ny;
    game->state = UNDECIDED;
    game->nx = board->nx;
    game->ny = board->ny;
    game->nopen = xy;
    game->board = board;
    game->cells = malloc(sizeof(Status) * xy);
    if (game->cells == NULL) return 1;
    for (size_t i = 0; i < xy; i++) {
        cell_idx(game, i) = STATUS_UNKNOWN;
    }
    return 0;
}

void clear_game(Game *game) {
    game->state = UNDECIDED;
    game->nx = 0;
    game->ny = 0;
    game->nopen = 0;
    game->board = NULL;
    free(game->cells);
    game->cells = NULL;
}

void toggle_flag(Game *game, size_t x, size_t y) {
    const Status status = cell_xy(game, x, y);
    if (status == STATUS_UNKNOWN) {
        cell_xy(game, x, y) = STATUS_FLAG;
    } else if (status == STATUS_FLAG) {
        cell_xy(game, x, y) = STATUS_UNKNOWN;
    }
}

void uncover(Game *game, size_t x, size_t y) {
    if (cell_xy(game, x, y) != STATUS_UNKNOWN) return;
    const Status status = board_cell_status(game->board, x, y);
    if (status == STATUS_MINE) {
        game->nopen = 0;
        game->state = LOST;
        _set_all_mines(game, STATUS_MINE);
    } else {
        cell_xy(game, x, y) = status;
        game->nopen--;
        // If all non-mine cells have been uncovered, the game is won
        if (game->nopen == game->board->nmines) {
            game->state = WON;
            _set_all_mines(game, STATUS_FLAG);
            return;
        }
        // If there are no mines around the uncovered position, uncover the
        // neighbouring cells too
        if (status == STATUS_ZERO) uncover_adjacent(game, x, y);
    }
}

void uncover_adjacent(Game *game, size_t x, size_t y) {
    const bool lOk = x > 0;
    const bool rOk = x < game->nx - 1;
    const bool tOk = y > 0;
    const bool bOk = y < game->ny - 1;
    if (tOk && lOk && cell_xy(game, x - 1, y - 1) <= STATUS_UNKNOWN) uncover(game, x - 1, y - 1);
    if (tOk &&        cell_xy(game, x    , y - 1) <= STATUS_UNKNOWN) uncover(game, x    , y - 1);
    if (tOk && rOk && cell_xy(game, x + 1, y - 1) <= STATUS_UNKNOWN) uncover(game, x + 1, y - 1);
    if (       lOk && cell_xy(game, x - 1, y    ) <= STATUS_UNKNOWN) uncover(game, x - 1, y    );
    if (       rOk && cell_xy(game, x + 1, y    ) <= STATUS_UNKNOWN) uncover(game, x + 1, y    );
    if (bOk && lOk && cell_xy(game, x - 1, y + 1) <= STATUS_UNKNOWN) uncover(game, x - 1, y + 1);
    if (bOk &&        cell_xy(game, x    , y + 1) <= STATUS_UNKNOWN) uncover(game, x    , y + 1);
    if (bOk && rOk && cell_xy(game, x + 1, y + 1) <= STATUS_UNKNOWN) uncover(game, x + 1, y + 1);
}

void _set_all_mines(Game *game, Status status) {
    for (size_t x = 0; x < game->nx; x++) {
        for (size_t y = 0; y < game->ny; y++) {
            if (cell_xy(game->board, x, y) == MINE) {
                cell_xy(game, x, y) = status;
            }
        }
    }
}



/* Board operations */

int initialize_board(Board *board, size_t nx, size_t ny, size_t nmines) {
    const size_t nxy = nx * ny;
    // Must have at least one free cell
    if (nxy <= nmines) return 2;
    // Initialize members
    board->nx = nx;
    board->ny = ny;
    board->nmines = nmines;
    board->cells = malloc(sizeof(Cell) * nxy);
    if (board->cells == NULL) return 1;
    // Distribute mines randomly
    size_t idx[nxy];
    fill_with_shuffled_indices(idx, nxy);
    for (size_t i = 0; i < nxy; i++) {
        cell_idx(board, idx[i]) = (i < nmines) ? MINE : EMPTY;
    }
    return 0;
}

void clear_board(Board *board) {
    board->nx = 0;
    board->ny = 0;
    board->nmines = 0;
    free(board->cells);
    board->cells = NULL;
}

void make_safe(Board *board, size_t x, size_t y) {
    if (cell_xy(board, x, y) == EMPTY) return;
    // Find an empty cell to place the mine
    const size_t nxy = board->nx * board->ny;
    size_t idx[nxy];
    fill_with_shuffled_indices(idx, nxy);
    for (size_t i = 0; i < nxy; i++) {
        if (cell_idx(board, i) == EMPTY) {
            // Place mine at new spot
            cell_idx(board, i) = MINE;
            // Remove mine from old spot
            cell_xy(board, x, y) = EMPTY;
            return;
        }
    }
}

Status board_cell_status(Board *board, size_t x, size_t y) {
    if (cell_xy(board, x, y) == MINE) return STATUS_MINE;
    // Count mines in surrounding cells
    Status count = 0;
    const bool lOk = x > 0;
    const bool rOk = x < board->nx - 1;
    const bool tOk = y > 0;
    const bool bOk = y < board->ny - 1;
    if (tOk && lOk && cell_xy(board, x - 1, y - 1) == MINE) count++;
    if (tOk &&        cell_xy(board, x    , y - 1) == MINE) count++;
    if (tOk && rOk && cell_xy(board, x + 1, y - 1) == MINE) count++;
    if (       lOk && cell_xy(board, x - 1, y    ) == MINE) count++;
    if (       rOk && cell_xy(board, x + 1, y    ) == MINE) count++;
    if (bOk && lOk && cell_xy(board, x - 1, y + 1) == MINE) count++;
    if (bOk &&        cell_xy(board, x    , y + 1) == MINE) count++;
    if (bOk && rOk && cell_xy(board, x + 1, y + 1) == MINE) count++;
    return count;
}

