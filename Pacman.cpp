#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <utility>
#include <algorithm>

struct Entity
{
    int x;
    int y;
    int dx;
    int dy;
    char icon;
    WORD color;
};

static HANDLE gConsole = GetStdHandle(STD_OUTPUT_HANDLE);

const int kMapOffsetY = 2;
const int kFrameDelayMs = 80;

std::vector<std::string> maze =
{
    "######################################################################",
    "||.. .........................................................      ||",
    "||.. %%%%%%%%%%%%%%%%         ...    %%%%%%%%%%%%%%  |%|..          ||",
    "||..        |%|   |%|      |%|...    |%|         |%| |%|..          ||",
    "||..        |%|   |%|      |%|...    |%|         |%| |%|..          ||",
    "||..        %%%%%%%%%  . . |%|...    %%%%%%%%%%%%%%     ..          ||",
    "||..        |%|        . . |%|...   ............... |%| ..          ||",
    "||..        %%%%%%%%%%%. . |%|...   %%%%%%%%%%%%    |%| ..          ||",
    "||..                |%|.            |%|......       |%| ..          ||",
    "||..      ......... |%|.            |%|......|%|        ..          ||",
    "||..|%|   |%|%%%|%|.|%|. |%|           ......|%|        ..|%|       ||",
    "||..|%|   |%|   |%|..    %%%%%%%%%%%%  ......|%|         .|%|.      ||",
    "||..|%|   |%|   |%|..          ...|%|     %%%%%%        . |%|.      ||",
    "||..|%|             .          ...|%|               |%| ..|%|.      ||",
    "||..|%|   %%%%%%%%%%%%%%       ...|%|%%%%%%%%%%     |%| ..|%|%%%%%  ||",
    "||...............................................   |%| ..........  ||",
    "||   ............................................          .......  ||",
    "||   ............................................          .......  ||",
    "||..|%|  |%|   |%|..           ...|%|     %%%%%%    |%| ..|%|.      ||",
    "||..|%|            .           ...|%|               |%| ..|%|.      ||",
    "||..|%|  %%%%%%%%%%%%%%        ...|%|%%%%%%%%%%     |%| ..|%|%%%%%  ||",
    "||...............................................   |%| ..........  ||",
    "||  .............................................          .......  ||",
    "######################################################################"
};

Entity player = { 15, 8, 0, 0, 'P', FOREGROUND_GREEN | FOREGROUND_INTENSITY };
Entity ghost1 = { 3, 1, 1, 0, 'G', FOREGROUND_RED | FOREGROUND_INTENSITY };
Entity ghost2 = { 3, 2, 0, 1, 'G', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY };

int score = 0;
int lives = 3;
int pelletsRemaining = 0;
bool running = true;
bool won = false;

void gotoxy(short x, short y)
{
    COORD coord = { x, y };
    SetConsoleCursorPosition(gConsole, coord);
}

void setColor(WORD color)
{
    SetConsoleTextAttribute(gConsole, color);
}

void hideCursor()
{
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 100;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(gConsole, &cursorInfo);
}

bool isInside(int x, int y)
{
    return y >= 0 && y < static_cast<int>(maze.size()) &&
           x >= 0 && x < static_cast<int>(maze[y].size());
}

bool isWallChar(char c)
{
    return c == '#' || c == '%' || c == '|';
}

bool isWalkable(int x, int y)
{
    if (!isInside(x, y))
    {
        return false;
    }

    return !isWallChar(maze[y][x]);
}

void drawCharAt(int x, int y, char ch, WORD color)
{
    gotoxy(static_cast<short>(x), static_cast<short>(y + kMapOffsetY));
    setColor(color);
    std::cout << ch;
}

void drawMapCell(int x, int y)
{
    char c = maze[y][x];
    WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

    if (isWallChar(c))
    {
        color = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    }
    else if (c == '.')
    {
        color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // Yellow-ish
    }
    else
    {
        color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    drawCharAt(x, y, c, color);
}

void drawEntity(const Entity& e)
{
    drawCharAt(e.x, e.y, e.icon, e.color);
}

void eraseEntity(const Entity& e)
{
    drawMapCell(e.x, e.y);
}

void drawHUD()
{
    setColor(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    gotoxy(0, 0);
    std::cout << "PACMAN++  Score: " << score
              << "  Lives: " << lives
              << "  Pellets Left: " << pelletsRemaining
              << "                    ";

    setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    gotoxy(0, 1);
    std::cout << "Controls: Arrow Keys / WASD. Avoid ghosts. Eat all dots to win.         ";
}

void drawMaze()
{
    for (int y = 0; y < static_cast<int>(maze.size()); y++)
    {
        for (int x = 0; x < static_cast<int>(maze[y].size()); x++)
        {
            drawMapCell(x, y);
        }
    }
}

void countPellets()
{
    pelletsRemaining = 0;
    for (const std::string& row : maze)
    {
        for (char c : row)
        {
            if (c == '.')
            {
                pelletsRemaining++;
            }
        }
    }
}

void collectPelletIfAny()
{
    if (maze[player.y][player.x] == '.')
    {
        maze[player.y][player.x] = ' ';
        score += 10;
        pelletsRemaining--;
        drawMapCell(player.x, player.y);
        drawEntity(player);
    }
}

bool moveEntity(Entity& e, int dx, int dy)
{
    int nx = e.x + dx;
    int ny = e.y + dy;

    if (!isWalkable(nx, ny))
    {
        return false;
    }

    eraseEntity(e);
    e.x = nx;
    e.y = ny;
    drawEntity(e);
    return true;
}

void resetPositions()
{
    eraseEntity(player);
    eraseEntity(ghost1);
    eraseEntity(ghost2);

    player.x = 15;
    player.y = 8;
    player.dx = 0;
    player.dy = 0;

    ghost1.x = 3;
    ghost1.y = 1;
    ghost1.dx = 1;
    ghost1.dy = 0;

    ghost2.x = 3;
    ghost2.y = 2;
    ghost2.dx = 0;
    ghost2.dy = 1;

    drawEntity(player);
    drawEntity(ghost1);
    drawEntity(ghost2);

    collectPelletIfAny();
}

bool isCollision()
{
    return (player.x == ghost1.x && player.y == ghost1.y) ||
           (player.x == ghost2.x && player.y == ghost2.y);
}

void handlePlayerInput()
{
    int dx = 0;
    int dy = 0;

    if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) || (GetAsyncKeyState('D') & 0x8000))
    {
        dx = 1;
    }
    else if ((GetAsyncKeyState(VK_LEFT) & 0x8000) || (GetAsyncKeyState('A') & 0x8000))
    {
        dx = -1;
    }
    else if ((GetAsyncKeyState(VK_UP) & 0x8000) || (GetAsyncKeyState('W') & 0x8000))
    {
        dy = -1;
    }
    else if ((GetAsyncKeyState(VK_DOWN) & 0x8000) || (GetAsyncKeyState('S') & 0x8000))
    {
        dy = 1;
    }

    if (dx != 0 || dy != 0)
    {
        moveEntity(player, dx, dy);
        collectPelletIfAny();
    }
}

std::pair<int, int> chooseGhostDirection(const Entity& ghost)
{
    std::vector<std::pair<int, int>> dirs = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
    std::vector<std::pair<int, int>> validDirs;

    for (const std::pair<int, int>& d : dirs)
    {
        int nx = ghost.x + d.first;
        int ny = ghost.y + d.second;
        if (isWalkable(nx, ny))
        {
            validDirs.push_back(d);
        }
    }

    if (validDirs.empty())
    {
        return { 0, 0 };
    }

    // Mostly chase player, sometimes move random for unpredictability.
    bool chaseMode = (std::rand() % 100) < 70;

    if (chaseMode)
    {
        int bestDist = 1e9;
        std::pair<int, int> best = validDirs[0];

        for (const std::pair<int, int>& d : validDirs)
        {
            int nx = ghost.x + d.first;
            int ny = ghost.y + d.second;
            int dist = std::abs(player.x - nx) + std::abs(player.y - ny);

            if (dist < bestDist)
            {
                bestDist = dist;
                best = d;
            }
        }

        return best;
    }

    return validDirs[std::rand() % validDirs.size()];
}

void moveGhost(Entity& ghost)
{
    std::pair<int, int> d = chooseGhostDirection(ghost);
    ghost.dx = d.first;
    ghost.dy = d.second;
    moveEntity(ghost, ghost.dx, ghost.dy);
}

void printEndMessage()
{
    int endY = static_cast<int>(maze.size()) + kMapOffsetY + 1;
    gotoxy(0, static_cast<short>(endY));

    if (won)
    {
        setColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        std::cout << "You won! Final Score: " << score << "                    ";
    }
    else
    {
        setColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        std::cout << "Game Over! Final Score: " << score << "                  ";
    }

    setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    gotoxy(0, static_cast<short>(endY + 1));
}

int main()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    SetConsoleTitleA("Pacman++");
    system("cls");
    hideCursor();

    countPellets();
    drawHUD();
    drawMaze();
    drawEntity(player);
    drawEntity(ghost1);
    drawEntity(ghost2);
    collectPelletIfAny();
    drawHUD();

    while (running)
    {
        handlePlayerInput();

        if (isCollision())
        {
            lives--;
            drawHUD();
            if (lives <= 0)
            {
                running = false;
                break;
            }
            resetPositions();
            Sleep(350);
        }

        moveGhost(ghost1);
        moveGhost(ghost2);

        if (isCollision())
        {
            lives--;
            drawHUD();
            if (lives <= 0)
            {
                running = false;
                break;
            }
            resetPositions();
            Sleep(350);
        }

        if (pelletsRemaining <= 0)
        {
            won = true;
            running = false;
            break;
        }

        drawHUD();
        Sleep(kFrameDelayMs);
    }

    printEndMessage();
    return 0;
}
