#include <nds.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_COLS 4
#define COL_WIDTH (SCREEN_WIDTH / NUM_COLS)   // 64
#define BLOCK_WIDTH 56
#define BLOCK_HEIGHT 40
#define DEADLINE_Y  160

typedef struct {
    float y;
    bool active;   // 是否有块存在（其实 y < DEADLINE_Y 就存在，但用 active 更清晰）
} Column;

Column cols[NUM_COLS];
float speed = 2.0f;
int score = 0;
int selectedCol = 0;        // 被方向键选中的列
bool gameOver = false;

// 上屏打印分数
PrintConsole topScreen;

void resetGame() {
    score = 0;
    speed = 2.0f;
    gameOver = false;
    selectedCol = 0;
    for (int i = 0; i < NUM_COLS; i++) {
        cols[i].y = -(BLOCK_HEIGHT + (rand() % 80));   // 随机起始高度，让开场不拥挤
        cols[i].active = true;
    }
}

// 绘制下屏游戏画面（使用 framebuffer 模式）
void drawGame() {
    u16* fb = (u16*)VRAM_A;
    // 清屏为白色（16 位颜色：RGB 5,5,5，白色 = 0xFFFF）
    dmaFillHalfWords(0xFFFF, fb, SCREEN_WIDTH * SCREEN_HEIGHT * 2);

    // 画底线（红色）
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        fb[DEADLINE_Y * SCREEN_WIDTH + x] = RGB15(31, 0, 0);
    }

    // 画四个列的分隔线（可选，浅灰色）
    for (int i = 1; i < NUM_COLS; i++) {
        int lineX = i * COL_WIDTH;
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            fb[y * SCREEN_WIDTH + lineX] = RGB15(20, 20, 20);
        }
    }

    // 画黑色块
    for (int col = 0; col < NUM_COLS; col++) {
        if (!cols[col].active) continue;
        int blockY = (int)cols[col].y;
        int blockLeft = col * COL_WIDTH + (COL_WIDTH - BLOCK_WIDTH) / 2;
        int blockRight = blockLeft + BLOCK_WIDTH;
        int blockTop = blockY;
        int blockBottom = blockY + BLOCK_HEIGHT;

        // 裁剪在屏幕范围内的部分
        for (int y = blockTop; y < blockBottom; y++) {
            if (y < 0 || y >= SCREEN_HEIGHT) continue;
            for (int x = blockLeft; x < blockRight; x++) {
                if (x >= 0 && x < SCREEN_WIDTH) {
                    fb[y * SCREEN_WIDTH + x] = RGB15(0, 0, 0); // 黑色
                }
            }
        }
    }

    // 画当前选中列的高亮光标（金色边框）
    int selX = selectedCol * COL_WIDTH;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        fb[y * SCREEN_WIDTH + selX] = RGB15(31, 31, 0); // 左边界
        fb[y * SCREEN_WIDTH + selX + COL_WIDTH - 1] = RGB15(31, 31, 0); // 右边界
    }
}

// 更新上屏分数显示
void updateScoreDisplay() {
    consoleSelect(&topScreen);
    printf("\x1b[10;0HScore: %d   Speed: %.1f   \n", score, (double)speed);
}

// 消除指定列的块
void hitColumn(int col) {
    if (!cols[col].active) return;
    score++;
    speed += 0.15f;
    cols[col].y = -(BLOCK_HEIGHT + (rand() % 60));   // 新块从上方随机位置落下
    // active 保持 true
}

int main(void) {
    // 初始化随机种子
    srand(1);

    // 上屏：控制台输出分数
    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    consoleInit(0, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    consoleSelect(&topScreen);
    printf("  Don't Tap the White Tile\n\n");

    // 下屏：framebuffer 模式，直接写屏
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);

    resetGame();

    touchPosition touch;
    u32 prevKeys = 0;
    bool touchWasDown = false;

    while (1) {
        swiWaitForVBlank();
        scanKeys();
        u32 held = keysHeld();
        u32 down = keysDown();
        touchRead(&touch);

        if (gameOver) {
            // 显示结束画面
            consoleSelect(&topScreen);
            printf("\x1b[12;0H     GAME OVER        \n");
            printf("\x1b[14;0H  Press START to retry\n");
            // 下屏也写个简单的提示
            u16* fb = (u16*)VRAM_A;
            dmaFillHalfWords(0x0000, fb, SCREEN_WIDTH * SCREEN_HEIGHT * 2); // 黑屏
            if (down & KEY_START) {
                resetGame();
                updateScoreDisplay();
            }
            continue;
        }

        // --- 输入处理 ---
        bool actionTaken = false; // 每帧最多消除一个块

        // 按键：方向键移动选中列
        if (down & KEY_LEFT) {
            selectedCol--;
            if (selectedCol < 0) selectedCol = NUM_COLS - 1;
        }
        if (down & KEY_RIGHT) {
            selectedCol++;
            if (selectedCol >= NUM_COLS) selectedCol = 0;
        }

        // 按 A 键踩当前选中列的黑块
        if ((down & KEY_A) && !actionTaken) {
            hitColumn(selectedCol);
            actionTaken = true;
        }

        // 触屏检测
        if (touch.px && !actionTaken) {
            if (!touchWasDown) { // 检测触摸按下沿，避免按住连续触发
                int tx = touch.px;
                int ty = touch.py;
                int col = tx / COL_WIDTH;
                if (col >= 0 && col < NUM_COLS && cols[col].active) {
                    int by = (int)cols[col].y;
                    if (ty >= by && ty <= by + BLOCK_HEIGHT) {
                        hitColumn(col);
                        actionTaken = true;
                    }
                }
            }
            touchWasDown = true;
        } else {
            touchWasDown = false;
        }

        // --- 游戏逻辑更新 ---
        for (int i = 0; i < NUM_COLS; i++) {
            if (!cols[i].active) continue;
            cols[i].y += speed;
            // 检查失败：块底部超过底线
            if (cols[i].y + BLOCK_HEIGHT >= DEADLINE_Y) {
                gameOver = true;
                break;
            }
            // 如果块完全离开屏幕底部且没被点到（理论上块在底线以上就会失败，此处再兜底）
            if (cols[i].y > SCREEN_HEIGHT) {
                // 不应该发生，但重置到上方
                cols[i].y = -(BLOCK_HEIGHT + rand() % 40);
            }
        }

        // 绘制
        drawGame();
        updateScoreDisplay();
    }

    return 0;
}