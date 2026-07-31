#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NUM_COLS 4
#define COL_WIDTH (SCREEN_WIDTH / NUM_COLS)
#define BLOCK_WIDTH 56
#define BLOCK_HEIGHT 40
#define DEADLINE_Y 160

typedef struct {
    float y;
    bool active;
} Column;

Column cols[NUM_COLS];
float speed = 2.0f;
int score = 0;
int selectedCol = 0;
bool gameOver = false;

static void resetGame(void) {
    score = 0;
    speed = 2.0f;
    gameOver = false;
    selectedCol = 0;
    for (int i = 0; i < NUM_COLS; i++) {
        cols[i].y = -(BLOCK_HEIGHT + (rand() % 80));
        cols[i].active = true;
    }
}

static void drawGame(void) {
    u16* fb = (u16*)VRAM_A;
    dmaFillHalfWords(0xFFFF, fb, SCREEN_WIDTH * SCREEN_HEIGHT * 2);

    for (int x = 0; x < SCREEN_WIDTH; x++)
        fb[DEADLINE_Y * SCREEN_WIDTH + x] = RGB15(31,0,0);

    for (int i = 1; i < NUM_COLS; i++) {
        int lineX = i * COL_WIDTH;
        for (int y = 0; y < SCREEN_HEIGHT; y++)
            fb[y * SCREEN_WIDTH + lineX] = RGB15(20,20,20);
    }

    for (int col = 0; col < NUM_COLS; col++) {
        if (!cols[col].active) continue;
        int blockY = (int)cols[col].y;
        int left = col * COL_WIDTH + (COL_WIDTH - BLOCK_WIDTH) / 2;
        int right = left + BLOCK_WIDTH;
        int top = blockY;
        int bottom = blockY + BLOCK_HEIGHT;

        for (int y = top; y < bottom; y++) {
            if (y < 0 || y >= SCREEN_HEIGHT) continue;
            for (int x = left; x < right; x++) {
                if (x >= 0 && x < SCREEN_WIDTH)
                    fb[y * SCREEN_WIDTH + x] = RGB15(0,0,0);
            }
        }
    }

    int selX = selectedCol * COL_WIDTH;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        fb[y * SCREEN_WIDTH + selX] = RGB15(31,31,0);
        fb[y * SCREEN_WIDTH + selX + COL_WIDTH - 1] = RGB15(31,31,0);
    }
}

static void updateScoreDisplay(PrintConsole* topScreen) {
    consoleSelect(topScreen);
    printf("\x1b[10;0HScore: %d   Speed: %.1f   \n", score, (double)speed);
}

static void hitColumn(int col) {
    if (!cols[col].active) return;
    score++;
    speed += 0.15f;
    cols[col].y = -(BLOCK_HEIGHT + (rand() % 60));
}

int main(void) {
    srand(1);

    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    PrintConsole topScreen;
    consoleInit(0, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    consoleSelect(&topScreen);
    printf("  Don't Tap the White Tile\n\n");

    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);

    resetGame();

    touchPosition touch;
    bool touchWasDown = false;

    while (1) {
        swiWaitForVBlank();
        scanKeys();
        u32 down = keysDown();
        touchRead(&touch);

        if (gameOver) {
            consoleSelect(&topScreen);
            printf("\x1b[12;0H     GAME OVER        \n");
            printf("\x1b[14;0H  Press START to retry\n");
            u16* fb = (u16*)VRAM_A;
            dmaFillHalfWords(0x0000, fb, SCREEN_WIDTH * SCREEN_HEIGHT * 2);
            if (down & KEY_START) {
                resetGame();
                updateScoreDisplay(&topScreen);
            }
            continue;
        }

        if (down & KEY_LEFT) {
            selectedCol--;
            if (selectedCol < 0) selectedCol = NUM_COLS - 1;
        }
        if (down & KEY_RIGHT) {
            selectedCol++;
            if (selectedCol >= NUM_COLS) selectedCol = 0;
        }
        if (down & KEY_A) {
            hitColumn(selectedCol);
        }

        if (touch.px) {
            if (!touchWasDown) {
                int tx = touch.px, ty = touch.py;
                int col = tx / COL_WIDTH;
                if (col >= 0 && col < NUM_COLS && cols[col].active) {
                    int by = (int)cols[col].y;
                    if (ty >= by && ty <= by + BLOCK_HEIGHT)
                        hitColumn(col);
                }
            }
            touchWasDown = true;
        } else {
            touchWasDown = false;
        }

        for (int i = 0; i < NUM_COLS; i++) {
            if (!cols[i].active) continue;
            cols[i].y += speed;
            if (cols[i].y + BLOCK_HEIGHT >= DEADLINE_Y) {
                gameOver = true;
                break;
            }
            if (cols[i].y > SCREEN_HEIGHT)
                cols[i].y = -(BLOCK_HEIGHT + rand() % 40);
        }

        drawGame();
        updateScoreDisplay(&topScreen);
    }
    return 0;
}
