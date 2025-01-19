#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

char board[133] =   "   |   |   \n"
                    " - | - | - \n"
                    "   |   |   \n"
                    "___|___|___\n"
                    "   |   |   \n"
                    " - | - | - \n"
                    "   |   |   \n"
                    "___|___|___\n"
                    "   |   |   \n"
                    " - | - | - \n"
                    "   |   |   \n\x00";

const int idx[9] = {13, 17, 21, 61, 65, 69, 109, 113, 117};

void print_state(char* state) {

    for(int i = 0; i < 9; i++) {
        switch (state[i]) {
            case 0:
                board[idx[i]] = '-';
                break;
            case 1:
                board[idx[i]] = 'X';
                break;
            case 2:
                board[idx[i]] = 'O';
        }
    }

    printf("%s", board);
}


void get_user_input(char* state) {
    int choice;

    get_choice:
    printf("Enter index (1-9): ");
    scanf("%d", &choice);
    choice--;

    while (getchar() != '\n') {}

    if (choice < 0 || choice > 8 || state[choice] != 0) {
        printf("Invalid index...\n");
        goto get_choice;
    }

    state[choice] = 1;
}

void get_bot_input(char* state) {
    for(int i = 0; i < 9; i++) {
        if (state[i] == 0) {
            state[i] = 2;
            return;
        }
    }
}

char game_over(char* state) {
    // return 0 if not over
    // return 1 if p1 wins
    // return 2 if p2 wins
    // return 3 if tie

    // check rows
    for(int r = 0; r < 3; r++) {
        if (state[r * 3] > 0 && state[r * 3] == state[r * 3 + 1] && state[r * 3 + 1] == state[r * 3 + 2]) {
            return state[r * 3];
        }
    }

    // check columns
    for(int c = 0; c < 3; c++) {
        if (state[c] > 0 && state[c] == state[c + 3] && state[c + 3] == state[c + 6]) {
            return state[c];
        }
    }

    // check diagonals
    if (state[0] > 0 && state[0] == state[4] && state[4] == state[8]) {
        return state[0];
    }

    if (state[2] > 0 && state[2] == state[4] && state[4] == state[6]) {
        return state[2];
    }

    // check for tie
    if (state[0] != 0 && state[1] != 0 && state[2] != 0 && state[3] != 0 && state[4] != 0 && state[5] != 0 && state[6] != 0 && state[7] != 0 && state[8] != 0) {
        return 3;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    char state[9] = {0,0,0,0,0,0,0,0,0};
    char win = 0;

    while (true) {
        print_state(state);

        get_user_input(state);
        win = game_over(state);

        if (win) {
            break;
        }
        
        get_bot_input(state);
        win = game_over(state);
        
        if (win) {
            break;
        }

    }

    print_state(state);

    switch (win) {
        case 1:
            printf("You Win!\n");
            break;
        case 2:
            printf("You Lose!\n");
            break;
        case 3:
            printf("It's a Tie!\n");
            break;
    }

}
