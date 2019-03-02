#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <curses.h>

#include "mines.h"


// (x, y)-based access to game cells:
#define cell_xy(X, Y) (game.cells[(X) + (Y) * game.nx])


/* Global program state */

Board board;
Game game;

size_t cursorX;
size_t cursorY;

WINDOW *boardWindow = NULL;
WINDOW *headerWindow = NULL;



/* Cursor movement */

void move_cursor_left(bool skipping) {
    while (cursorX > 0) {
        cursorX--;
        // When skipping, find the next non-0 cell in the direction:
        if (!skipping || cell_xy(cursorX, cursorY) != STATUS_ZERO) break;
    }
}

void move_cursor_right(bool skipping) {
    while (cursorX < game.nx - 1) {
        cursorX++;
        if (!skipping || cell_xy(cursorX, cursorY) != STATUS_ZERO) break;
    }
}

void move_cursor_up(bool skipping) {
    while (cursorY > 0) {
        cursorY--;
        if (!skipping || cell_xy(cursorX, cursorY) != STATUS_ZERO) break;
    }
}

void move_cursor_down(bool skipping) {
    while (cursorY < game.ny - 1) {
        cursorY++;
        if (!skipping || cell_xy(cursorX, cursorY) != STATUS_ZERO) break;
    }
}


// An extended action (uncover all non-flagged adjacent cells) is available for
// cells with 1-8 markers:
bool extended_action_available() {
    const Status status = cell_xy(cursorX, cursorY);
    return status > STATUS_ZERO && status != STATUS_MINE;
}



/* Interface */

void draw_layout() {
    // Clean up old windows:
    if (boardWindow != NULL) delwin(boardWindow);
    if (headerWindow != NULL) delwin(headerWindow);
    wclear(stdscr);
    // Determine available screen size:
    int termW, termH;
    getmaxyx(stdscr, termH, termW);
    // Determine required screen size:
    const int boardW = (int) (4*game.nx + 1);
    const int boardH = (int) (2*game.ny + 1);
    const int headerH = 3;
    // Terminal is not big enough to contain board. Display a message
    // containing the required size:
    if (termH < boardH + headerH || termW < boardW) {
        wattrset(stdscr, A_BOLD);
        mvwprintw(stdscr, 0, 0, "required terminal size: %d x %d\n"
                                "available terminal size: %d x %d", boardW, boardH + headerH, termW, termH);
        boardWindow = NULL;
        headerWindow = NULL;
    // Create (centered) windows for the header and game board:
    } else {
        const int offsetX = (termW - boardW) / 2;
        const int offsetY = (termH - boardH - headerH) / 2;
        boardWindow = newwin(boardH, boardW, offsetY + headerH, offsetX);
        headerWindow = newwin(headerH, boardW, offsetY, offsetX);
    }
    // Fill the background:
    wbkgd(stdscr, COLOR_PAIR(1));
    wrefresh(stdscr);
    // Board and header have to be refreshed separately
}

void draw_board() {
    if (boardWindow == NULL) return;
    werase(boardWindow);
    for (size_t y = 0; y < game.ny; y++) {
        // Every second row in y-direction
        const size_t screenY = 1 + 2*y;
        for (size_t x = 0; x < game.nx; x++) {
            // Every fourth row in x-direction
            const size_t screenX = 2 + 4*x;
            Status cellStatus = cell_xy(x, y);
            // Output symbol:
            chtype ch;
            switch (cellStatus) {
                case STATUS_UNKNOWN:
                    ch = ACS_BULLET;
                    break;
                case STATUS_MINE:
                    ch = '#';
                    break;
                case STATUS_FLAG:
                    ch = 'X';
                    break;
                case STATUS_ZERO:
                    ch = ' ';
                    break;
                case STATUS_ONE:
                    ch = '1';
                    break;
                case STATUS_TWO:
                    ch = '2';
                    break;
                case STATUS_THREE:
                    ch = '3';
                    break;
                case STATUS_FOUR:
                    ch = '4';
                    break;
                case STATUS_FIVE:
                    ch = '5';
                    break;
                case STATUS_SIX:
                    ch = '6';
                    break;
                case STATUS_SEVEN:
                    ch = '7';
                    break;
                case STATUS_EIGHT:
                    ch = '8';
                    break;
            }
            // Apply style depending on cell status:
            wattrset(boardWindow, COLOR_PAIR(10 + cellStatus));
            mvwaddch(boardWindow, screenY, screenX, ch);
        }
    }
    // Screen position of cursor:
    const size_t x = 2 + 4*cursorX;
    const size_t y = 1 + 2*cursorY;
    // Draw inner cursor:
    wattrset(boardWindow, COLOR_PAIR(20));
    mvwaddch(boardWindow, y + 1, x + 2, ACS_LRCORNER);
    mvwaddch(boardWindow, y + 1, x - 2, ACS_LLCORNER);
    mvwaddch(boardWindow, y - 1, x + 2, ACS_URCORNER);
    mvwaddch(boardWindow, y - 1, x - 2, ACS_ULCORNER);
    // Draw outer (3x3) cursor only if the extended action is available:
    if (extended_action_available()) {
        wattrset(boardWindow, COLOR_PAIR(21));
        if (cursorX < game.nx - 1 && cursorY < game.ny - 1) mvwaddch(boardWindow, y + 3, x + 6, ACS_LRCORNER);
        if (cursorX > 0 && cursorY < game.ny - 1) mvwaddch(boardWindow, y + 3, x - 6, ACS_LLCORNER);
        if (cursorX < game.nx - 1 && cursorY > 0) mvwaddch(boardWindow, y - 3, x + 6, ACS_URCORNER);
        if (cursorX > 0 && cursorY > 0) mvwaddch(boardWindow, y - 3, x - 6, ACS_ULCORNER);
    }
    // Apply changes:
    wrefresh(boardWindow);
}

void draw_header() {
    if (headerWindow == NULL) return;
    werase(headerWindow);
    // Indicate game status with header color and smiley:
    int color;
    char title[3];
    switch (game.state) {
        case UNDECIDED:
            sprintf(title, ":|");
            color = 2;
            break;
        case WON:
            sprintf(title, ":)");
            color = 3;
            break;
        case LOST:
            sprintf(title, ":(");
            color = 4;
            break;
    }
    wbkgd(headerWindow, COLOR_PAIR(color) | A_BOLD);
    mvwprintw(headerWindow, 1, 2, title);
    // Apply changes:
    wrefresh(headerWindow);
}

void resizeHandler(int sig) {
    // Reinitialize curses:
    endwin();
    refresh();
    // Redraw everything:
    draw_layout();
    draw_board();
    draw_header();
}



/* Application */

int main(int argc, char* argv[]) {

    int err;

    // Show help if not called with the expected number of arguments
    if (argc != 4) {
        fprintf(stderr, "usage: cursed-mines <width> <height> <mines>\n"
                        "\n"
                        "hlkj or arrow keys to move\n"
                        "HLKJ to skip 0 cells\n"
                        "\n"
                        "space to uncover a cell\n"
                        "space on a 1-8 cell uncovers all non-flagged adjacent cells\n"
                        "f, m  to toggle flag\n"
                        "q, ^D to quit\n"
                        "\n"
                        "The first action is always safe\n"
                        "Press any key to quit after the game has ended\n");
        return -1;
    }

    // Parse board size and number of mines
    size_t sizeX = (size_t) strtoul(argv[1], NULL, 10);
    size_t sizeY = (size_t) strtoul(argv[2], NULL, 10);
    size_t mines = (size_t) strtoul(argv[3], NULL, 10);

    // Initialize the random number generator
    seed_random();

    err = initialize_board(&board, sizeX, sizeY, mines);
    if (err) {
        fprintf(stderr, "error during board initialization (error code %d)\n", err);
        return err;
    }
    err = initialize_game(&game, &board);
    if (err) {
        fprintf(stderr, "error during game initialization (error code %d)\n", err);
        return err;
    }

    cursorX = 0;
    cursorY = 0;

    // Start curses:
    initscr();
    // Disable buffering of input:
    cbreak();
    // Suppress automatic input echo:
    noecho();
    // Hide the terminal cursor:
    curs_set(0);
    // Enable capture of arrow keys:
    keypad(stdscr, TRUE);
    // Enable colors:
    start_color();
    use_default_colors();
    // Background color pair:
    init_pair( 1, COLOR_WHITE, COLOR_BLACK); // background
    // Header color pairs:
    init_pair( 2, COLOR_WHITE, COLOR_CYAN); // normal header
    init_pair( 3, COLOR_WHITE, COLOR_GREEN); // win header
    init_pair( 4, COLOR_WHITE, COLOR_RED); // lose header
    // Cell color pairs:
    init_pair( 8, COLOR_YELLOW, -1); // flag
    init_pair( 9, COLOR_BLACK, -1); // unknown
    init_pair(10, COLOR_WHITE, -1); // 0
    init_pair(11, COLOR_BLUE, -1); // 1
    init_pair(12, COLOR_CYAN, -1); // 2
    init_pair(13, COLOR_GREEN, -1); // 3
    init_pair(14, COLOR_BLACK, -1); // 4
    init_pair(15, COLOR_BLACK, -1); // 5
    init_pair(16, COLOR_BLACK, -1); // 6
    init_pair(17, COLOR_BLACK, -1); // 7
    init_pair(18, COLOR_BLACK, -1); // 8
    init_pair(19, COLOR_RED, -1); // mine
    init_pair(20, COLOR_BLACK, -1); // inner cursor
    init_pair(21, COLOR_WHITE, -1); // outer cursor

    // Initialize interface:
    draw_layout();
    draw_header();

    // Attach resizeHandler to window change signal:
    struct sigaction resizeAction;
    resizeAction.sa_handler = resizeHandler;
    sigemptyset(&resizeAction.sa_mask);
    resizeAction.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &resizeAction, NULL);

    // Game loop:
    while (true) {
        // Update game board:
        draw_board();
        // Obtain user input:
        const int key = getch();
        // Exit program on q or ^D
        if (key == 'q' || key == ('d' & 0x1F)) break;
        // Actions
        switch (key) {
            case 'k':
            case 'K':
            case KEY_UP:
                move_cursor_up(key == 'K');
                break;
            case 'j':
            case 'J':
            case KEY_DOWN:
                move_cursor_down(key == 'J');
                break;
            case 'h':
            case 'H':
            case KEY_LEFT:
                move_cursor_left(key == 'H');
                break;
            case 'l':
            case 'L':
            case KEY_RIGHT:
                move_cursor_right(key == 'L');
                break;
            case 'f':
            case 'm':
                toggle_flag(&game, cursorX, cursorY);
                break;
            case ' ':
                // The first move is always safe
                if (game.nopen == game.nx * game.ny) {
                    make_safe(&board, cursorX, cursorY);
                }
                // Normal or extended action?
                if (extended_action_available()) {
                    uncover_adjacent(&game, cursorX, cursorY);
                } else {
                    uncover(&game, cursorX, cursorY);
                }
                break;
        }
        
        // Game has ended:
        if (game.state != UNDECIDED) {
            draw_board();
            draw_header();
            getch(); // any key to quit
            break;
        }

    }

    // Clear resize handler
    resizeAction.sa_handler = SIG_DFL;
    sigaction(SIGWINCH, &resizeAction, NULL);

    // End curses
    if (boardWindow != NULL) delwin(boardWindow);
    if (headerWindow != NULL) delwin(headerWindow);
    endwin();

    // Clean up
    clear_game(&game);
    clear_board(&board);

    return 0;

}

