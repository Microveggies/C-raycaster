#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
#include <cmath>
using namespace std;

#include <stdio.h>
#include <Windows.h>

int nScreenWidth = 120;         // Console Screen Size X (columns)
int nScreenHeight = 40;         // Console Screen Size Y (rows)
int nMapWidth = 16;             // World Dimensions
int nMapHeight = 16;

float fPlayerX = 14.7f;         // Player Start Position
float fPlayerY = 5.09f;
float fPlayerA = 0.0f;          // Player Start Rotation
float fFOV = 3.14159f / 4.0f;   // Field of View
float fDepth = 16.0f;           // Maximum rendering distance
float fSpeed = 5.0f;            // Walking Speed

// Textures (8x8 characters)
const int nTexWidth = 8;
const int nTexHeight = 8;

// Wall texture (brick pattern)
string texWall[8] = {
    "########",
    "#......#",
    "#......#",
    "#......#",
    "#......#",
    "#......#",
    "#......#",
    "########"
};

// Water texture (wavy pattern)
string texWater[8] = {
    "~~~~~~~~",
    "~~~~~~~~",
    "~~~~~~~~",
    "~~~~~~~~",
    "~~~~~~~",
    "~~~~~~~~",
    "~~~~~~~~",
    "~~~~~~~~"
};

int main()
{
    // Create Screen Buffer
    wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;

    // Create Map of world space
    // # = wall block, . = space, ~ = water
    wstring map;
    map += L"#########.......";
    map += L"#...............";
    map += L"#.......########";
    map += L"#......~~~~~~~.#";
    map += L"#......##......#";
    map += L"#......##......#";
    map += L"#......~~~~~~~.#";
    map += L"###............#";
    map += L"##.............#";
    map += L"#......####..###";
    map += L"#......#.......#";
    map += L"#......#.......#";
    map += L"#..............#";
    map += L"#......#########";
    map += L"#..............#";
    map += L"################";

    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();

    while (1)
    {
        // Timing
        tp2 = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();

        // Controls - Rotation
        if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
            fPlayerA -= (fSpeed * 0.75f) * fElapsedTime;
        if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
            fPlayerA += (fSpeed * 0.75f) * fElapsedTime;

        // Controls - Movement
        if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
        {
            fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
            fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
            if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
            {
                fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
                fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
            }
        }
        if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
        {
            fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
            fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
            if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
            {
                fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
                fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
            }
        }

        // Raycasting
        for (int x = 0; x < nScreenWidth; x++)
        {
            float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;
            float fStepSize = 0.1f; // Increment size for ray casting
            float fDistanceToWall = 0.0f;

            bool bHitWall = false;   // True if ray hits a wall
            bool bBoundary = false;  // True if ray hits a boundary between two blocks

            float fEyeX = sinf(fRayAngle); // Unit vector for ray in player space
            float fEyeY = cosf(fRayAngle);

            int nTestX = 0, nTestY = 0;
            wchar_t cBlockType = ' '; // Character to identify block type

            // Raycasting - Detecting walls and textures
            while (!bHitWall && fDistanceToWall < fDepth)
            {
                fDistanceToWall += fStepSize;
                nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
                nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

                // Ray out of bounds
                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
                {
                    bHitWall = true;
                    fDistanceToWall = fDepth;
                }
                else
                {
                    // Ray hits a block
                    wchar_t cTestBlock = map[nTestX * nMapWidth + nTestY];
                    if (cTestBlock == '#') // Wall block
                    {
                        bHitWall = true;
                        cBlockType = cTestBlock; // Store block type for texture
                    }
                }
            }

            // Calculate distance to ceiling and floor
            int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
            int nFloor = nScreenHeight - nCeiling;

            // Wall Rendering
            for (int y = 0; y < nScreenHeight; y++)
            {
                if (y <= nCeiling)
                    screen[y * nScreenWidth + x] = ' '; // Ceiling
                else if (y > nCeiling && y <= nFloor)
                {
                    // Texture Sampling for Walls
                    float fSampleY = ((float)(y - nCeiling) / (float)(nFloor - nCeiling));
                    int nTexY = (int)(fSampleY * (float)nTexHeight);

                    // Ensure texture coordinates are within bounds
                    int nTexX = (int)(fDistanceToWall * (float)nTexWidth) % nTexWidth;

                    if (nTexX >= 0 && nTexX < nTexWidth && nTexY >= 0 && nTexY < nTexHeight)
                    {
                        // Safe access of the wall texture
                        char texturePixel = texWall[nTexY][nTexX];  // Line 180 - now validated
                        screen[y * nScreenWidth + x] = texturePixel;
                    }
                    else
                    {
                        // Default to a blank space or error handling
                        screen[y * nScreenWidth + x] = ' ';
                    }
                }
                else
                {
                    // --- Floor-Casting Logic (to render water) ---
                    // Distance from the player to the current row on the screen
                    float fFloorDist = (float)nScreenHeight / (2.0f * (float)y - (float)nScreenHeight);

                    // Calculate real-world floor coordinates from the player's perspective
                    float fFloorX = fPlayerX + fFloorDist * fEyeX;
                    float fFloorY = fPlayerY + fFloorDist * fEyeY;

                    // Map coordinates (to check the map for water '~' or other floor tiles)
                    int nCellX = (int)fFloorX % nMapWidth;
                    int nCellY = (int)fFloorY % nMapHeight;

                    // Ensure map coordinates are valid and within bounds
                    if (nCellX >= 0 && nCellX < nMapWidth && nCellY >= 0 && nCellY < nMapHeight)
                    {
                        // Check if the floor tile is water
                        if (map[nCellX * nMapWidth + nCellY] == '~')
                        {
                            // Sample the water texture using fractional part of fFloorX and fFloorY
                            int nTexX = (int)(fFloorX * nTexWidth) % nTexWidth;
                            int nTexY = (int)(fFloorY * nTexHeight) % nTexHeight;

                            // Ensure texture coordinates are valid
                            if (nTexX >= 0 && nTexX < nTexWidth && nTexY >= 0 && nTexY < nTexHeight)
                            {
                                // Render the water texture
                                char texturePixel = texWater[nTexY][nTexX];
                                screen[y * nScreenWidth + x] = texturePixel;
                            }
                        }
                        else
                        {
                            // Render non-water floor (shaded based on distance)
                            float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
                            short nShade = ' ';
                            if (b < 0.25)      nShade = '#';
                            else if (b < 0.5)  nShade = 'x';
                            else if (b < 0.75) nShade = '.';
                            else if (b < 0.9)  nShade = '-';
                            else               nShade = ' ';
                            screen[y * nScreenWidth + x] = nShade;
                        }
                    }
                }
            }
        }

        // Display stats
        swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

        // Display map and player position
        for (int nx = 0; nx < nMapWidth; nx++)
            for (int ny = 0; ny < nMapWidth; ny++)
                screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
        screen[((int)fPlayerX + 1) * nScreenWidth + (int)fPlayerY] = 'P';

        // Write the screen buffer
        screen[nScreenWidth * nScreenHeight - 1] = '\0';
        WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten);
    }

    return 0;
}
