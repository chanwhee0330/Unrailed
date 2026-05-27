#include <windows.h>
#include <mmsystem.h>
#include <gdiplus.h>
#include <atlimage.h>
#include <tchar.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cwchar>
#include <cstdlib>
#include <ctime>
#include <cmath>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winmm.lib")

using namespace Gdiplus;

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define MAP_WIDTH 90
#define MAP_HEIGHT 60
#define TILE_SIZE 32
#define BASE_WIDTH_TILES 10
#define BASE_HEIGHT_TILES 10
#define BASE_DOOR_HEIGHT_TILES 4
#define BASE_START_RAIL_TILES 11
#define RAIL_CRAFT_TIME 1.0f
#define OBSTACLE_CRAFT_TIME 1.5f
#define OBSTACLE_WOOD_COST 2
#define OBSTACLE_STONE_COST 2
#define PLAYER_COLLISION_LEFT 14
#define PLAYER_COLLISION_RIGHT 50
#define PLAYER_COLLISION_TOP 17
#define PLAYER_COLLISION_BOTTOM 42

#define MAX_HEAT 100.0f
#define HEAT_RATE 0.07f
#define TIMER_ID 1
#define TARGET_FRAME_MS 16
#define MAX_FRAME_DELTA 0.05f
#define marginX 16
#define marginY 39

#define START_BTN_LEFT   440
#define START_BTN_RIGHT  840
#define START_BTN_TOP    548
#define START_BTN_BOTTOM 650

struct Vec2 {
    float x, y;
};

struct Camera {
    float x, y;
};

enum PlayerDir { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_UP };
enum RailDir { RAIL_HORIZONTAL, RAIL_VERTICAL, RAIL_TURN_RD, RAIL_TURN_LD, RAIL_TURN_RU, RAIL_TURN_LU };
enum ResourceType { RESOURCE_TREE, RESOURCE_ROCK };
enum GameState { STATE_START, STATE_PLAYING };

struct MapData {
    int tiles[MAP_HEIGHT][MAP_WIDTH];
    Bitmap* image;
    std::vector<unsigned char> solidBits;
    int pixelW, pixelH;
    std::vector<unsigned char> waterBits;
};

struct Player {
    int id;
    Vec2 pos;
    float speed;
    float size;
    PlayerDir dir;
    bool isMoving;
    CImage idleSheet;
    CImage walkSheet;
    int frame;
    int wood;
    int stone;
    int railCount;
    int obstacleCount;
    float railCraftProgress;
    float obstacleCraftProgress;
    DWORD lastFrameTime;
    DWORD frameDelay;
    bool hasBucket;
    bool bucketFull;
};

struct RailData {
    int tileX, tileY;
    RailDir dir;
    int owner;
};

struct Rail {
    std::vector<RailData> rails;
    Bitmap* railImage;
    Bitmap* turnImage;
};

struct Obstacle {
    int tileX, tileY;
    int owner;
};

struct Train {
    Vec2 pos;
    float speed;
    float heat;
    float dirX, dirY;
    bool finished;
    ULONGLONG lastTime;
    Bitmap* image;
};
struct Bucket {
    Vec2 pos;
    bool pickedUp;
};
struct Resource {
    ResourceType type;
    Vec2 pos;
    Vec2 spawnPos;
    float size;
    float harvestProgress;
    bool active;
    ULONGLONG respawnStartTime;
    ULONGLONG respawnDelay;
};

struct BaseArea {
    int x, y, w, h;
    bool doorOnRight;
};

struct GameData {
    HWND hWnd;
    Player p1, p2;
    MapData map;
    Rail rail;
    Train train1, train2;
    Camera cam1, cam2;
    std::vector<Resource> resources;
    std::vector<Obstacle> obstacles;
    bool gameOver;
    int winner;
    bool rKeyPrev;
    bool twoKeyPrev;
    bool f2KeyPrev;
    bool f3KeyPrev;
    bool infiniteRailMode;
    bool infiniteResourceMode;
    HDC memDC;
    HBITMAP memBitmap;
    HBITMAP oldBitmap;
    ULONGLONG lastUpdateTime;
    RailDir selectedDir1;
    RailDir selectedDir2;
    Bucket bucket1; // p1용
    Bucket bucket2; // p2용
    bool fKeyPrev;
    bool threeKeyPrev;
    bool qKeyPrev;
    bool zeroKeyPrev;
    GameState gameState;
    Bitmap* startScreenImage;
};
Bitmap* g_emptyBucket = nullptr;
Bitmap* g_fullBucket = nullptr;
HFONT g_inventoryFont = nullptr;
SolidBrush* g_overHeatBrush = nullptr;
HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Unrailed";
ULONG_PTR g_gdiplusToken;
GameData game;

void BuildCollisionCache(MapData* map);
void InitMap(MapData* map);
void ReleaseMap(MapData* map);
void LoadMapCsv(MapData* map, const std::wstring& csvPath);
void DrawMap(MapData* map, Graphics* g, Camera cam, int viewW, int viewH);
BaseArea GetBaseArea(int index);
RECT GetPlayerRectAt(Player* p, Vec2 pos);
bool RectsOverlap(RECT a, RECT b);
bool IsPlayerInsideBase(Player* p, BaseArea base);
bool IsBlockedByBaseWall(Player* p, Vec2 nextPos);
bool IsBlockedByObstacle(Player* p, Vec2 nextPos);
bool IsRectInsideAnyBase(RECT rc);
RECT GetRailCraftStationRect(BaseArea base);
RECT GetObstacleCraftStationRect(BaseArea base);
bool IsPlayerTouchingRailCraftStation(Player* p);
bool IsPlayerTouchingObstacleCraftStation(Player* p);
void UpdateRailCraft(Player* p, float deltaTime);
void UpdateObstacleCraft(Player* p, float deltaTime);
void DrawRailCraftStations(Graphics* g, Player* viewer);
void DrawObstacleCraftStations(Graphics* g, Player* viewer);
void DrawBase(Graphics* g, BaseArea base, bool viewerInside);
void DrawBases(Graphics* g, Player* viewer);
bool IsSolid(MapData* map, float x, float y);
void InitPlayer(Player* p, int id, float x, float y);
int GetPlayerFrameCount(Player* p);
int GetPlayerDirectionRow(Player* p);
RECT GetPlayerRect(Player* p);
void GetPlacementTile(Player* p, int* tileX, int* tileY);
void UpdatePlayer(Player* p, bool up, bool down, bool left, bool right, MapData* map, float deltaTime);
void DrawPlayer(Player* p, HDC hdc, Camera cam, int offsetY);
void DrawInventory(Player* p, HDC hdc, Camera cam, int offsetY);
void InitRail(Rail* rail);
void ReleaseRail(Rail* rail);
bool HasHorizontal(Rail* rail, int tileX, int tileY);
bool HasVertical(Rail* rail, int tileX, int tileY);
bool HasRail(Rail* rail, int tileX, int tileY);
bool HasObstacle(int tileX, int tileY);
bool CanPlaceRailAt(Rail* rail, int tileX, int tileY, RailDir dir);
bool CanPlaceObstacleAt(int tileX, int tileY);
bool PlaceObstacle(int tileX, int tileY, int owner);
void UpdateRailNeighbors(Rail* rail, int tileX, int tileY);
bool PlaceRail(Rail* rail, int tileX, int tileY, RailDir dir, int owner);
void DrawOneRailImage(Rail* rail, Graphics* g, float x, float y, RailDir dir, bool preview);
void DrawRailPreview(Rail* rail, Graphics* g, int x, int y, RailDir dir);
void DrawObstaclePreview(Graphics* g, int x, int y);
void DrawPlacementPreview(Graphics* g, Player* player, RailDir railDir, Color tileColor);
void DrawRails(Rail* rail, Graphics* g, Camera cam, int viewW, int viewH);
void DrawObstacles(Graphics* g, Camera cam, int viewW, int viewH);
void InitTrain(Train* train, float x, float y, int direction, const wchar_t* imagePath);
void ReleaseTrain(Train* train);
bool IsTrainOverheated(Train* train);
void UpdateTrainDirection(Train* train, RailDir rd);
void UpdateTrain(Train* train, Rail* rail, float deltaTime);
void DrawHeatBar(Train* train, Graphics* g);
void DrawTrain(Train* train, Graphics* g, Camera cam, int viewW, int viewH);
void InitResource(Resource* resource, ResourceType type, float x, float y);
RECT GetResourceRect(Resource* resource);
bool ResourceIntersectsPlayer(Resource* resource, Player* player);
bool HasRailOnResourceSpawn(Resource* resource, Rail* rail);
void StartResourceRespawn(Resource* resource);
void HarvestResource(Resource* resource, Player* player, float deltaTime);
void UpdateResource(Resource* resource, Player* p1, Player* p2, Rail* rail, float deltaTime);
void DrawResource(Resource* resource, Graphics* g);
bool CanPlaceResourceAt(float x, float y);
bool FindRandomResourcePosition(float* outX, float* outY, int preferredSide = -1);
void CreateResources();
void DrawVictoryScreen(Graphics* g);
bool IsWater(MapData* map, float x, float y) {
    if (x < 0 || y < 0 || x >= map->pixelW || y >= map->pixelH) return false;
    if (map->waterBits.empty()) return false;
    int index = (int)y * map->pixelW + (int)x;
    return (map->waterBits[index / 8] & (1 << (index % 8))) != 0;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hDC;

    switch (iMessage) {
    case WM_CREATE:
    {
        // init game
        game.hWnd = hWnd;
        game.gameState = STATE_START;
        game.startScreenImage = new Bitmap(L"Image\\scene\\gamestart.png");
        PlaySound(L"Sound\\start sound.wav", NULL, SND_FILENAME | SND_LOOP | SND_ASYNC);
        game.gameOver = false;
        game.winner = 0;
        game.rKeyPrev = false;
        game.twoKeyPrev = false;
        game.f2KeyPrev = false;
        game.f3KeyPrev = false;
        game.infiniteRailMode = false;
        game.infiniteResourceMode = false;
        game.selectedDir1 = RAIL_HORIZONTAL;
        game.selectedDir2 = RAIL_HORIZONTAL;
        game.bucket1 = { {400, 400}, false };  // 위치는 조정
        game.bucket2 = { {2400, 1500}, false }; // 위치는 조정
        game.fKeyPrev = false;
        game.threeKeyPrev = false;
        game.qKeyPrev = false;
        game.zeroKeyPrev = false;
        game.obstacles.clear();
        g_inventoryFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        g_overHeatBrush = new SolidBrush(Color(120, 255, 0, 0));

        InitPlayer(&game.p1, 1, 300, 400);
        InitPlayer(&game.p2, 2, 2500, 1500);
        InitMap(&game.map);
        LoadMapCsv(&game.map, L"Image\\Map\\unTiled map_Tile Layer 1.csv");
        InitRail(&game.rail);
        g_emptyBucket = new Bitmap(L"Image\\train\\bucket.png");  // 경로 채워줘
        g_fullBucket = new Bitmap(L"Image\\train\\waterbucket.png");  // 경로 채워줘
        HDC hdc = GetDC(hWnd);
        game.memDC = CreateCompatibleDC(hdc);
        game.memBitmap = CreateCompatibleBitmap(hdc, SCREEN_WIDTH, SCREEN_HEIGHT);
        game.oldBitmap = (HBITMAP)SelectObject(game.memDC, game.memBitmap);
        ReleaseDC(hWnd, hdc);

        InitTrain(&game.train1, 64, 424, 1, L"Image\\train\\locomoto.png");
        InitTrain(&game.train2, 2720, 1512, -1, L"Image\\train\\locomoto2.png");

        game.cam1 = { 0, 0 };
        game.cam2 = { 0, 0 };
        game.lastUpdateTime = GetTickCount64();

        // start rail
        int t1X = (int)(game.train1.pos.x / TILE_SIZE);
        int t1Y = (int)((game.train1.pos.y + 24) / TILE_SIZE);
        for (int i = 0; i < BASE_START_RAIL_TILES; i++) PlaceRail(&game.rail, t1X + i, t1Y, RAIL_HORIZONTAL, 1);

        int t2X = (int)(game.train2.pos.x / TILE_SIZE);
        int t2Y = (int)((game.train2.pos.y + 24) / TILE_SIZE);
        for (int i = 0; i < BASE_START_RAIL_TILES; i++) PlaceRail(&game.rail, t2X + 2 - i, t2Y, RAIL_HORIZONTAL, 2);

        CreateResources();
        SetTimer(hWnd, TIMER_ID, TARGET_FRAME_MS, NULL);
        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == TIMER_ID) {
            if (game.gameState == STATE_START) {
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }

            if (!game.gameOver) {
                // time
                ULONGLONG now = GetTickCount64();
                float deltaTime = (now - game.lastUpdateTime) / 1000.0f;
                game.lastUpdateTime = now;
                if (deltaTime > MAX_FRAME_DELTA) deltaTime = MAX_FRAME_DELTA;

                // player move
                UpdatePlayer(&game.p1, GetAsyncKeyState('W'), GetAsyncKeyState('S'), GetAsyncKeyState('A'), GetAsyncKeyState('D'), &game.map, deltaTime);
                UpdatePlayer(&game.p2, GetAsyncKeyState(VK_UP), GetAsyncKeyState(VK_DOWN), GetAsyncKeyState(VK_LEFT), GetAsyncKeyState(VK_RIGHT), &game.map, deltaTime);

                // resource update
                for (int i = 0; i < (int)game.resources.size(); i++) {
                    UpdateResource(&game.resources[i], &game.p1, &game.p2, &game.rail, deltaTime);
                }
                UpdateRailCraft(&game.p1, deltaTime);
                UpdateRailCraft(&game.p2, deltaTime);
                UpdateObstacleCraft(&game.p1, deltaTime);
                UpdateObstacleCraft(&game.p2, deltaTime);

                // rail direction change
                bool rKey = GetAsyncKeyState('R') & 0x8000;
                if (rKey && !game.rKeyPrev) {
                    game.selectedDir1 = (game.selectedDir1 == RAIL_HORIZONTAL) ? RAIL_VERTICAL : RAIL_HORIZONTAL;
                }
                game.rKeyPrev = rKey;

                bool twoKey = (GetAsyncKeyState(VK_NUMPAD2) & 0x8000) || (GetAsyncKeyState('2') & 0x8000);
                if (twoKey && !game.twoKeyPrev) {
                    game.selectedDir2 = (game.selectedDir2 == RAIL_HORIZONTAL) ? RAIL_VERTICAL : RAIL_HORIZONTAL;
                }
                game.twoKeyPrev = twoKey;

                bool f2Key = GetAsyncKeyState(VK_F2) & 0x8000;
                if (f2Key && !game.f2KeyPrev) {
                    game.infiniteRailMode = !game.infiniteRailMode;
                }
                game.f2KeyPrev = f2Key;

                bool f3Key = GetAsyncKeyState(VK_F3) & 0x8000;
                if (f3Key && !game.f3KeyPrev) {
                    game.infiniteResourceMode = !game.infiniteResourceMode;
                }
                game.f3KeyPrev = f3Key;

                // rail place
                if (GetAsyncKeyState('E') & 0x8000) {
                    int tileX, tileY;
                    GetPlacementTile(&game.p1, &tileX, &tileY);
                    if ((game.infiniteRailMode || game.infiniteResourceMode || game.p1.railCount > 0) &&
                        PlaceRail(&game.rail, tileX, tileY, game.selectedDir1, 1) &&
                        !game.infiniteRailMode &&
                        !game.infiniteResourceMode) {
                        game.p1.railCount--;
                    }
                }

                if ((GetAsyncKeyState(VK_NUMPAD1) & 0x8000) || (GetAsyncKeyState('1') & 0x8000)) {
                    int tileX, tileY;
                    GetPlacementTile(&game.p2, &tileX, &tileY);
                    if ((game.infiniteRailMode || game.infiniteResourceMode || game.p2.railCount > 0) &&
                        PlaceRail(&game.rail, tileX, tileY, game.selectedDir2, 2) &&
                        !game.infiniteRailMode &&
                        !game.infiniteResourceMode) {
                        game.p2.railCount--;
                    }
                }

                bool qKey = GetAsyncKeyState('Q') & 0x8000;
                if (qKey && !game.qKeyPrev && (game.infiniteResourceMode || game.p1.obstacleCount > 0)) {
                    int tileX, tileY;
                    GetPlacementTile(&game.p1, &tileX, &tileY);
                    if (PlaceObstacle(tileX, tileY, 1) && !game.infiniteResourceMode) game.p1.obstacleCount--;
                }
                game.qKeyPrev = qKey;

                bool zeroKey = (GetAsyncKeyState('0') & 0x8000) || (GetAsyncKeyState(VK_NUMPAD0) & 0x8000);
                if (zeroKey && !game.zeroKeyPrev && (game.infiniteResourceMode || game.p2.obstacleCount > 0)) {
                    int tileX, tileY;
                    GetPlacementTile(&game.p2, &tileX, &tileY);
                    if (PlaceObstacle(tileX, tileY, 2) && !game.infiniteResourceMode) game.p2.obstacleCount--;
                }
                game.zeroKeyPrev = zeroKey;

                // Player1
                // Player1 - F키 줍기/놓기
                bool fKey = GetAsyncKeyState('F') & 0x8000;
                if (fKey && !game.fKeyPrev) {
                    if (!game.p1.hasBucket && !game.bucket1.pickedUp) {
                        RECT pr = GetPlayerRect(&game.p1);
                        RECT br = { (LONG)game.bucket1.pos.x, (LONG)game.bucket1.pos.y,
                                    (LONG)(game.bucket1.pos.x + 64), (LONG)(game.bucket1.pos.y + 64) };
                        if (RectsOverlap(pr, br)) {
                            game.bucket1.pickedUp = true;
                            game.p1.hasBucket = true;
                        }
                    }
                    else if (game.p1.hasBucket) {
                        game.p1.hasBucket = false;
                        game.p1.bucketFull = false;
                        game.bucket1.pickedUp = false;
                        game.bucket1.pos = game.p1.pos;
                    }
                }
                game.fKeyPrev = fKey;


               
                // Player1 물 채우기
                if (game.p1.hasBucket && !game.p1.bucketFull) {
                    RECT pr = GetPlayerRect(&game.p1);
                    int margin = 10; // 물가 감지 여유
                    if (IsWater(&game.map, (float)(pr.left - margin), (float)pr.top) ||
                        IsWater(&game.map, (float)(pr.right + margin), (float)pr.top) ||
                        IsWater(&game.map, (float)pr.left, (float)(pr.top - margin)) ||
                        IsWater(&game.map, (float)pr.left, (float)(pr.bottom + margin)) ||
                        IsWater(&game.map, (float)(pr.right + margin), (float)pr.bottom) ||
                        IsWater(&game.map, (float)pr.right, (float)(pr.bottom + margin)))
                        game.p1.bucketFull = true;
                }

                
                if (game.p1.hasBucket && game.p1.bucketFull) {
                    RECT pr = GetPlayerRect(&game.p1);
                    RECT tr = { (LONG)game.train1.pos.x, (LONG)game.train1.pos.y,
                                (LONG)(game.train1.pos.x + 96), (LONG)(game.train1.pos.y + 48) };
                    if (RectsOverlap(pr, tr)) {
                        game.train1.heat -= 90.0f;
                        if (game.train1.heat < 0) game.train1.heat = 0;
                        game.p1.bucketFull = false;
                    }
                }

                // Player2
               // Player2 - 3키 줍기/놓기
                bool threeKey = (GetAsyncKeyState('3') & 0x8000) || (GetAsyncKeyState(VK_NUMPAD3) & 0x8000);
                if (threeKey && !game.threeKeyPrev) {
                    if (!game.p2.hasBucket && !game.bucket2.pickedUp) {
                        RECT pr = GetPlayerRect(&game.p2);
                        RECT br = { (LONG)game.bucket2.pos.x, (LONG)game.bucket2.pos.y,
                                    (LONG)(game.bucket2.pos.x + 64), (LONG)(game.bucket2.pos.y + 64) };
                        if (RectsOverlap(pr, br)) {
                            game.bucket2.pickedUp = true;
                            game.p2.hasBucket = true;
                        }
                    }
                    else if (game.p2.hasBucket) {
                        game.p2.hasBucket = false;
                        game.p2.bucketFull = false;
                        game.bucket2.pickedUp = false;
                        game.bucket2.pos = game.p2.pos;
                    }
                }
                game.threeKeyPrev = threeKey;
                // Player2 물 채우기
                if (game.p2.hasBucket && !game.p2.bucketFull) {
                    RECT pr = GetPlayerRect(&game.p2);
                    int margin = 10;
                    if (IsWater(&game.map, (float)(pr.left - margin), (float)pr.top) ||
                        IsWater(&game.map, (float)(pr.right + margin), (float)pr.top) ||
                        IsWater(&game.map, (float)pr.left, (float)(pr.top - margin)) ||
                        IsWater(&game.map, (float)pr.left, (float)(pr.bottom + margin)) ||
                        IsWater(&game.map, (float)(pr.right + margin), (float)pr.bottom) ||
                        IsWater(&game.map, (float)pr.right, (float)(pr.bottom + margin)))
                        game.p2.bucketFull = true;
                }
                if (game.p2.hasBucket && game.p2.bucketFull) {
                    RECT pr = GetPlayerRect(&game.p2);
                    RECT tr = { (LONG)game.train2.pos.x, (LONG)game.train2.pos.y,
                                (LONG)(game.train2.pos.x + 96), (LONG)(game.train2.pos.y + 48) };
                    if (RectsOverlap(pr, tr)) {
                        game.train2.heat -= 90.0f;
                        if (game.train2.heat < 0) game.train2.heat = 0;
                        game.p2.bucketFull = false;
                    }
                }
                // train move
                UpdateTrain(&game.train1, &game.rail, deltaTime);
                UpdateTrain(&game.train2, &game.rail, deltaTime);

                // win check
                if (game.train1.finished && !game.train2.finished) { game.gameOver = true; game.winner = 1; }
                else if (game.train2.finished && !game.train1.finished) { game.gameOver = true; game.winner = 2; }
                else if (game.train1.finished && game.train2.finished) { game.gameOver = true; game.winner = 0; }

                // camera follow
                int halfH = SCREEN_HEIGHT / 2;
                game.cam1.x = game.p1.pos.x - SCREEN_WIDTH / 2;
                game.cam1.y = game.p1.pos.y - (halfH / 2);
                if (game.cam1.x < 0) game.cam1.x = 0;
                if (game.cam1.y < 0) game.cam1.y = 0;
                if (game.cam1.x > MAP_WIDTH * TILE_SIZE - SCREEN_WIDTH) game.cam1.x = (float)(MAP_WIDTH * TILE_SIZE - SCREEN_WIDTH);
                if (game.cam1.y > MAP_HEIGHT * TILE_SIZE - halfH) game.cam1.y = (float)(MAP_HEIGHT * TILE_SIZE - halfH);

                game.cam2.x = game.p2.pos.x - SCREEN_WIDTH / 2;
                game.cam2.y = game.p2.pos.y - (halfH / 2);
                if (game.cam2.x < 0) game.cam2.x = 0;
                if (game.cam2.y < 0) game.cam2.y = 0;
                if (game.cam2.x > MAP_WIDTH * TILE_SIZE - SCREEN_WIDTH) game.cam2.x = (float)(MAP_WIDTH * TILE_SIZE - SCREEN_WIDTH);
                if (game.cam2.y > MAP_HEIGHT * TILE_SIZE - halfH) game.cam2.y = (float)(MAP_HEIGHT * TILE_SIZE - halfH);

            }

            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_PAINT:
    {
        hDC = BeginPaint(hWnd, &ps);

        Graphics g(game.memDC);

        if (game.gameState == STATE_START) {
            g.Clear(Color(255, 0, 0, 0));
            if (game.startScreenImage && game.startScreenImage->GetLastStatus() == Ok)
                g.DrawImage(game.startScreenImage, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

            BitBlt(hDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, game.memDC, 0, 0, SRCCOPY);
            EndPaint(hWnd, &ps);
            return 0;
        }

        // draw to back buffer
        int halfH = SCREEN_HEIGHT / 2;

        // top screen
        Graphics g1(game.memDC);
        g1.SetClip(Rect(0, 0, SCREEN_WIDTH, halfH));
        g1.Clear(Color(255, 255, 255));
        g1.TranslateTransform(-game.cam1.x, -game.cam1.y);
        DrawMap(&game.map, &g1, game.cam1, SCREEN_WIDTH, halfH);

        DrawPlacementPreview(&g1, &game.p1, game.selectedDir1, Color(120, 255, 255, 0));

        for (int i = 0; i < (int)game.resources.size(); i++) DrawResource(&game.resources[i], &g1);
        DrawRails(&game.rail, &g1, game.cam1, SCREEN_WIDTH, halfH);
        DrawObstacles(&g1, game.cam1, SCREEN_WIDTH, halfH);
        DrawTrain(&game.train1, &g1, game.cam1, SCREEN_WIDTH, halfH);
        DrawTrain(&game.train2, &g1, game.cam1, SCREEN_WIDTH, halfH);
        // 바닥에 있는 양동이
        if (!game.bucket1.pickedUp)
            g1.DrawImage(g_emptyBucket, game.bucket1.pos.x, game.bucket1.pos.y, 64.0f, 64.0f);

        // 플레이어가 든 양동이
        if (game.p1.hasBucket) {
            Bitmap* img = game.p1.bucketFull ? g_fullBucket : g_emptyBucket;
            g1.DrawImage(img, game.p1.pos.x + 20, game.p1.pos.y + 10, 48.0f, 48.0f);
        }
        DrawBases(&g1, &game.p1);
        DrawRailCraftStations(&g1, &game.p1);
        DrawObstacleCraftStations(&g1, &game.p1);

        g1.Flush();
        SaveDC(game.memDC);
        IntersectClipRect(game.memDC, 0, 0, SCREEN_WIDTH, halfH);
        DrawPlayer(&game.p1, game.memDC, game.cam1, 0);
        DrawPlayer(&game.p2, game.memDC, game.cam1, 0);
        DrawInventory(&game.p1, game.memDC, game.cam1, 0);
        DrawInventory(&game.p2, game.memDC, game.cam1, 0);
        RestoreDC(game.memDC, -1);

        // bottom screen
        Graphics g2(game.memDC);
        g2.SetClip(Rect(0, halfH, SCREEN_WIDTH, halfH));
        g2.TranslateTransform(0, (REAL)halfH);
        g2.TranslateTransform(-game.cam2.x, -game.cam2.y);
        DrawMap(&game.map, &g2, game.cam2, SCREEN_WIDTH, halfH);

        DrawPlacementPreview(&g2, &game.p2, game.selectedDir2, Color(120, 0, 255, 255));

        for (int i = 0; i < (int)game.resources.size(); i++) DrawResource(&game.resources[i], &g2);
        DrawRails(&game.rail, &g2, game.cam2, SCREEN_WIDTH, halfH);
        DrawObstacles(&g2, game.cam2, SCREEN_WIDTH, halfH);
        DrawTrain(&game.train1, &g2, game.cam2, SCREEN_WIDTH, halfH);
        DrawTrain(&game.train2, &g2, game.cam2, SCREEN_WIDTH, halfH);
        if (!game.bucket2.pickedUp)
            g2.DrawImage(g_emptyBucket, game.bucket2.pos.x, game.bucket2.pos.y, 64.0f, 64.0f);
        if (game.p2.hasBucket) {
            Bitmap* img = game.p2.bucketFull ? g_fullBucket : g_emptyBucket;
            g2.DrawImage(img, game.p2.pos.x + 20, game.p2.pos.y + 10, 48.0f, 48.0f);
        }
        DrawBases(&g2, &game.p2);
        DrawRailCraftStations(&g2, &game.p2);
        DrawObstacleCraftStations(&g2, &game.p2);

        g2.Flush();
        SaveDC(game.memDC);
        IntersectClipRect(game.memDC, 0, halfH, SCREEN_WIDTH, SCREEN_HEIGHT);
        DrawPlayer(&game.p1, game.memDC, game.cam2, halfH);
        DrawPlayer(&game.p2, game.memDC, game.cam2, halfH);
        DrawInventory(&game.p1, game.memDC, game.cam2, halfH);
        DrawInventory(&game.p2, game.memDC, game.cam2, halfH);
        RestoreDC(game.memDC, -1);

        // game over screen
        if (game.gameOver) {
            Graphics vg(game.memDC);
            DrawVictoryScreen(&vg);
        }

        Pen pen(Color(255, 0, 0, 0), 5);
        g.DrawLine(&pen, 0, halfH, SCREEN_WIDTH, halfH);
        BitBlt(hDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, game.memDC, 0, 0, SRCCOPY);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_KEYDOWN:
        return 0;

    case WM_LBUTTONDOWN:
    {
        if (game.gameState == STATE_START) {
            int mx = LOWORD(lParam);
            int my = HIWORD(lParam);
            if (mx >= START_BTN_LEFT && mx <= START_BTN_RIGHT &&
                my >= START_BTN_TOP  && my <= START_BTN_BOTTOM) {
                PlaySound(NULL, NULL, 0);
                game.gameState = STATE_PLAYING;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_DESTROY:
        // release images and dc
        if (game.startScreenImage) { delete game.startScreenImage; game.startScreenImage = nullptr; }
        ReleaseTrain(&game.train1);
        ReleaseTrain(&game.train2);
        ReleaseRail(&game.rail);
        ReleaseMap(&game.map);

        SelectObject(game.memDC, game.oldBitmap);
        DeleteObject(game.memBitmap);
        DeleteDC(game.memDC);
        delete g_emptyBucket;
        delete g_fullBucket;
        if (g_inventoryFont) {
            DeleteObject(g_inventoryFont);
            g_inventoryFont = nullptr;
        }
        delete g_overHeatBrush;
        g_overHeatBrush = nullptr;
        KillTimer(hWnd, TIMER_ID);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, iMessage, wParam, lParam);
}


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpszCmdParam, _In_ int nCmdShow) {
    HWND hWnd;
    MSG Message;
    WNDCLASSEX WndClass;
    g_hInst = hInstance;
    srand((unsigned int)time(NULL));
    timeBeginPeriod(1);

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    WndClass.cbSize = sizeof(WndClass);
    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc = (WNDPROC)WndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = hInstance;
    WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    WndClass.hCursor = LoadCursor(NULL, IDI_APPLICATION);
    WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    WndClass.lpszMenuName = NULL;
    WndClass.lpszClassName = lpszClass;
    WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassEx(&WndClass);

    hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW,
        0, 0, SCREEN_WIDTH + marginX, SCREEN_HEIGHT + marginY,
        NULL, (HMENU)NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    while (GetMessage(&Message, 0, 0, 0)) {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }

    GdiplusShutdown(g_gdiplusToken);
    timeEndPeriod(1);
    return (int)Message.wParam;
}

void BuildCollisionCache(MapData* map) {
    if (!map->image || map->image->GetLastStatus() != Ok) return;

    map->pixelW = map->image->GetWidth();
    map->pixelH = map->image->GetHeight();
    int pixelCount = map->pixelW * map->pixelH;
    map->solidBits.assign((pixelCount + 7) / 8, 0);
    map->waterBits.assign((pixelCount + 7) / 8, 0);  // ← 여기

    Rect rect(0, 0, map->pixelW, map->pixelH);
    BitmapData data;
    if (map->image->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &data) != Ok) return;

    for (int y = 0; y < map->pixelH; y++) {
        BYTE* row = (BYTE*)data.Scan0 + y * data.Stride;
        for (int x = 0; x < map->pixelW; x++) {
            BYTE b = row[x * 4 + 0];
            BYTE g = row[x * 4 + 1];
            BYTE r = row[x * 4 + 2];

            int index = y * map->pixelW + x;

            bool isGreen = (g > 80 && g > r && g > b);
            if (!isGreen)
                map->solidBits[index / 8] |= (1 << (index % 8));

            bool isBlue = (b > 100 && b > r && b > g);  // ← 루프 안으로
            if (isBlue)
                map->waterBits[index / 8] |= (1 << (index % 8));
        }
    }

    map->image->UnlockBits(&data);
}

void InitMap(MapData* map) {
    map->image = nullptr;
    map->pixelW = 0;
    map->pixelH = 0;

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            map->tiles[y][x] = 0;
        }
    }

    map->image = new Bitmap(L"Image\\Map\\UnrailedMap.png");
    BuildCollisionCache(map);
}

void ReleaseMap(MapData* map) {
    delete map->image;
    map->image = nullptr;
    map->solidBits.clear();
    map->waterBits.clear();
}

void LoadMapCsv(MapData* map, const std::wstring& csvPath) {
    std::ifstream file(csvPath);
    if (!file.is_open()) return;

    std::string line;
    int y = 0;
    while (std::getline(file, line) && y < MAP_HEIGHT) {
        std::stringstream ss(line);
        std::string cell;
        int x = 0;
        while (std::getline(ss, cell, ',') && x < MAP_WIDTH) {
            map->tiles[y][x] = std::stoi(cell);
            x++;
        }
        y++;
    }
}

void DrawMap(MapData* map, Graphics* g, Camera cam, int viewW, int viewH) {
    if (!map->image || map->image->GetLastStatus() != Ok) return;

    int srcX = (cam.x < 0) ? 0 : (int)cam.x;
    int srcY = (cam.y < 0) ? 0 : (int)cam.y;
    int srcW = viewW;
    int srcH = viewH;

    if (srcX + srcW > (int)map->image->GetWidth()) srcW = (int)map->image->GetWidth() - srcX;
    if (srcY + srcH > (int)map->image->GetHeight()) srcH = (int)map->image->GetHeight() - srcY;
    if (srcW <= 0 || srcH <= 0) return;

    Rect dest(srcX, srcY, srcW, srcH);
    g->DrawImage(map->image, dest, srcX, srcY, srcW, srcH, UnitPixel);
}

BaseArea GetBaseArea(int index) {
    BaseArea base;
    base.w = BASE_WIDTH_TILES * TILE_SIZE;
    base.h = BASE_HEIGHT_TILES * TILE_SIZE;

    if (index == 0) {
        base.x = 0;
        base.y = 9 * TILE_SIZE;
        base.doorOnRight = true;
    }
    else {
        base.x = (MAP_WIDTH - BASE_WIDTH_TILES) * TILE_SIZE;
        base.y = 43 * TILE_SIZE;
        base.doorOnRight = false;
    }

    return base;
}

RECT GetPlayerRectAt(Player* p, Vec2 pos) {
    RECT rc;
    rc.left = (LONG)(pos.x + PLAYER_COLLISION_LEFT);
    rc.top = (LONG)(pos.y + PLAYER_COLLISION_TOP);
    rc.right = (LONG)(pos.x + PLAYER_COLLISION_RIGHT);
    rc.bottom = (LONG)(pos.y + PLAYER_COLLISION_BOTTOM);
    return rc;
}

bool RectsOverlap(RECT a, RECT b) {
    RECT hit;
    return IntersectRect(&hit, &a, &b) != 0;
}

bool IsPlayerInsideBase(Player* p, BaseArea base) {
    RECT playerRect = GetPlayerRect(p);
    RECT baseRect = { base.x, base.y, base.x + base.w, base.y + base.h };
    return RectsOverlap(playerRect, baseRect);
}

bool IsBlockedByBaseWall(Player* p, Vec2 nextPos) {
    RECT playerRect = GetPlayerRectAt(p, nextPos);
    int wall = TILE_SIZE;
    int doorH = BASE_DOOR_HEIGHT_TILES * TILE_SIZE;

    for (int i = 0; i < 2; i++) {
        BaseArea base = GetBaseArea(i);
        int doorY = base.y + (base.h - doorH) / 2;
        RECT walls[5];

        walls[0] = { base.x, base.y, base.x + base.w, base.y + wall };
        walls[1] = { base.x, base.y + base.h - wall, base.x + base.w, base.y + base.h };

        if (base.doorOnRight) {
            walls[2] = { base.x, base.y, base.x + wall, base.y + base.h };
            walls[3] = { base.x + base.w - wall, base.y, base.x + base.w, doorY };
            walls[4] = { base.x + base.w - wall, doorY + doorH, base.x + base.w, base.y + base.h };
        }
        else {
            walls[2] = { base.x + base.w - wall, base.y, base.x + base.w, base.y + base.h };
            walls[3] = { base.x, base.y, base.x + wall, doorY };
            walls[4] = { base.x, doorY + doorH, base.x + wall, base.y + base.h };
        }

        for (int wallIndex = 0; wallIndex < 5; wallIndex++) {
            if (RectsOverlap(playerRect, walls[wallIndex])) return true;
        }
    }

    return false;
}

bool IsBlockedByObstacle(Player* p, Vec2 nextPos) {
    RECT playerRect = GetPlayerRectAt(p, nextPos);

    for (int i = 0; i < (int)game.obstacles.size(); i++) {
        RECT obstacleRect = {
            game.obstacles[i].tileX * TILE_SIZE,
            game.obstacles[i].tileY * TILE_SIZE,
            game.obstacles[i].tileX * TILE_SIZE + TILE_SIZE,
            game.obstacles[i].tileY * TILE_SIZE + TILE_SIZE
        };

        if (RectsOverlap(playerRect, obstacleRect)) return true;
    }

    return false;
}

bool IsRectInsideAnyBase(RECT rc) {
    for (int i = 0; i < 2; i++) {
        BaseArea base = GetBaseArea(i);
        RECT baseRect = { base.x, base.y, base.x + base.w, base.y + base.h };
        if (RectsOverlap(rc, baseRect)) return true;
    }

    return false;
}

RECT GetRailCraftStationRect(BaseArea base) {
    int stationX = base.doorOnRight ? base.x + 2 * TILE_SIZE : base.x + base.w - 3 * TILE_SIZE;
    int stationY = base.y + 2 * TILE_SIZE;
    return { stationX, stationY, stationX + TILE_SIZE, stationY + TILE_SIZE };
}

RECT GetObstacleCraftStationRect(BaseArea base) {
    int stationX = base.doorOnRight ? base.x + 2 * TILE_SIZE : base.x + base.w - 3 * TILE_SIZE;
    int stationY = base.y + 4 * TILE_SIZE;
    return { stationX, stationY, stationX + TILE_SIZE, stationY + TILE_SIZE };
}

bool IsPlayerTouchingRailCraftStation(Player* p) {
    RECT playerRect = GetPlayerRect(p);

    for (int i = 0; i < 2; i++) {
        RECT stationRect = GetRailCraftStationRect(GetBaseArea(i));
        if (RectsOverlap(playerRect, stationRect)) return true;
    }

    return false;
}

bool IsPlayerTouchingObstacleCraftStation(Player* p) {
    RECT playerRect = GetPlayerRect(p);

    for (int i = 0; i < 2; i++) {
        RECT stationRect = GetObstacleCraftStationRect(GetBaseArea(i));
        if (RectsOverlap(playerRect, stationRect)) return true;
    }

    return false;
}

void UpdateRailCraft(Player* p, float deltaTime) {
    if (!IsPlayerTouchingRailCraftStation(p) ||
        (!game.infiniteResourceMode && (p->wood <= 0 || p->stone <= 0))) {
        p->railCraftProgress = 0.0f;
        return;
    }

    p->railCraftProgress += deltaTime;

    if (p->railCraftProgress >= RAIL_CRAFT_TIME) {
        if (!game.infiniteResourceMode) {
            p->wood--;
            p->stone--;
        }
        p->railCount++;
        p->railCraftProgress = 0.0f;
        if (p->hasBucket) {
            if (p->id == 1) {
                game.p1.hasBucket = false;
                game.p1.bucketFull = false;
                game.bucket1.pickedUp = false;
                game.bucket1.pos = game.p1.pos;
            }
            else {
                game.p2.hasBucket = false;
                game.p2.bucketFull = false;
                game.bucket2.pickedUp = false;
                game.bucket2.pos = game.p2.pos;
            }
        }
    }
}

void UpdateObstacleCraft(Player* p, float deltaTime) {
    if (!IsPlayerTouchingObstacleCraftStation(p) ||
        (!game.infiniteResourceMode &&
            (p->wood < OBSTACLE_WOOD_COST ||
             p->stone < OBSTACLE_STONE_COST))) {
        p->obstacleCraftProgress = 0.0f;
        return;
    }

    p->obstacleCraftProgress += deltaTime;

    if (p->obstacleCraftProgress >= OBSTACLE_CRAFT_TIME) {
        if (!game.infiniteResourceMode) {
            p->wood -= OBSTACLE_WOOD_COST;
            p->stone -= OBSTACLE_STONE_COST;
        }
        p->obstacleCount++;
        p->obstacleCraftProgress = 0.0f;
    }
}

void DrawRailCraftStations(Graphics* g, Player* viewer) {
    for (int i = 0; i < 2; i++) {
        BaseArea base = GetBaseArea(i);
        if (!IsPlayerInsideBase(viewer, base)) continue;

        RECT rc = GetRailCraftStationRect(base);
        SolidBrush tableBrush(Color(255, 130, 82, 42));
        Pen tableEdge(Color(255, 70, 40, 18), 2.0f);
        SolidBrush railBrush(Color(255, 55, 55, 60));

        g->FillRectangle(&tableBrush, rc.left, rc.top, TILE_SIZE, TILE_SIZE);
        g->DrawRectangle(&tableEdge, rc.left, rc.top, TILE_SIZE, TILE_SIZE);
        g->FillRectangle(&railBrush, rc.left + 6, rc.top + 9, TILE_SIZE - 12, 4);
        g->FillRectangle(&railBrush, rc.left + 6, rc.top + 19, TILE_SIZE - 12, 4);

        if (RectsOverlap(GetPlayerRect(viewer), rc) && viewer->railCraftProgress > 0.0f) {
            float ratio = viewer->railCraftProgress / RAIL_CRAFT_TIME;
            if (ratio > 1.0f) ratio = 1.0f;

            SolidBrush bgBrush(Color(255, 0, 0, 0));
            SolidBrush fillBrush(Color(255, 255, 210, 40));
            g->FillRectangle(&bgBrush, (float)rc.left, (float)(rc.top - 10), (float)TILE_SIZE, 6.0f);
            g->FillRectangle(&fillBrush, (float)rc.left, (float)(rc.top - 10), (float)TILE_SIZE * ratio, 6.0f);
        }
    }
}

void DrawObstacleCraftStations(Graphics* g, Player* viewer) {
    for (int i = 0; i < 2; i++) {
        BaseArea base = GetBaseArea(i);
        if (!IsPlayerInsideBase(viewer, base)) continue;

        RECT rc = GetObstacleCraftStationRect(base);
        SolidBrush tableBrush(Color(255, 105, 70, 42));
        Pen tableEdge(Color(255, 55, 35, 22), 2.0f);
        SolidBrush stoneBrush(Color(255, 95, 98, 105));
        SolidBrush woodBrush(Color(255, 130, 78, 34));

        g->FillRectangle(&tableBrush, rc.left, rc.top, TILE_SIZE, TILE_SIZE);
        g->DrawRectangle(&tableEdge, rc.left, rc.top, TILE_SIZE, TILE_SIZE);
        g->FillRectangle(&woodBrush, rc.left + 7, rc.top + 18, 18, 8);
        g->FillEllipse(&stoneBrush, rc.left + 16, rc.top + 7, 11, 11);
        g->FillEllipse(&stoneBrush, rc.left + 6, rc.top + 8, 12, 12);

        if (RectsOverlap(GetPlayerRect(viewer), rc) && viewer->obstacleCraftProgress > 0.0f) {
            float ratio = viewer->obstacleCraftProgress / OBSTACLE_CRAFT_TIME;
            if (ratio > 1.0f) ratio = 1.0f;

            SolidBrush bgBrush(Color(255, 0, 0, 0));
            SolidBrush fillBrush(Color(255, 255, 120, 70));
            g->FillRectangle(&bgBrush, (float)rc.left, (float)(rc.top - 10), (float)TILE_SIZE, 6.0f);
            g->FillRectangle(&fillBrush, (float)rc.left, (float)(rc.top - 10), (float)TILE_SIZE * ratio, 6.0f);
        }
    }
}

void DrawBase(Graphics* g, BaseArea base, bool viewerInside) {
    int wall = TILE_SIZE;
    int doorH = BASE_DOOR_HEIGHT_TILES * TILE_SIZE;
    int doorY = base.y + (base.h - doorH) / 2;

    SolidBrush cover(viewerInside ? Color(55, 80, 180, 255) : Color(235, 20, 24, 30));
    SolidBrush wallBrush(Color(255, 70, 48, 32));
    Pen edgePen(Color(255, 20, 18, 16), 2.0f);
    SolidBrush doorBrush(Color(150, 255, 210, 70));

    g->FillRectangle(&cover, base.x, base.y, base.w, base.h);
    g->DrawRectangle(&edgePen, base.x, base.y, base.w, base.h);

    g->FillRectangle(&wallBrush, base.x, base.y, base.w, wall);
    g->FillRectangle(&wallBrush, base.x, base.y + base.h - wall, base.w, wall);

    if (base.doorOnRight) {
        g->FillRectangle(&wallBrush, base.x, base.y, wall, base.h);
        g->FillRectangle(&wallBrush, base.x + base.w - wall, base.y, wall, doorY - base.y);
        g->FillRectangle(&wallBrush, base.x + base.w - wall, doorY + doorH, wall, base.y + base.h - doorY - doorH);
        g->FillRectangle(&doorBrush, base.x + base.w - wall, doorY, wall, doorH);
    }
    else {
        g->FillRectangle(&wallBrush, base.x + base.w - wall, base.y, wall, base.h);
        g->FillRectangle(&wallBrush, base.x, base.y, wall, doorY - base.y);
        g->FillRectangle(&wallBrush, base.x, doorY + doorH, wall, base.y + base.h - doorY - doorH);
        g->FillRectangle(&doorBrush, base.x, doorY, wall, doorH);
    }
}

void DrawBases(Graphics* g, Player* viewer) {
    for (int i = 0; i < 2; i++) {
        BaseArea base = GetBaseArea(i);
        DrawBase(g, base, IsPlayerInsideBase(viewer, base));
    }
}

bool IsSolid(MapData* map, float x, float y) {
    if (x < 0 || y < 0 || x >= map->pixelW || y >= map->pixelH) return true;
    if (map->solidBits.empty()) return false;

    int px = (int)x;
    int py = (int)y;
    int index = py * map->pixelW + px;
    return (map->solidBits[index / 8] & (1 << (index % 8))) != 0;
}

void InitPlayer(Player* p, int id, float x, float y) {
    p->id = id;
    p->pos = { x, y };
    p->speed = 300.0f;
    p->size = 64.0f;
    p->dir = DIR_DOWN;
    p->isMoving = false;
    p->frame = 0;
    p->wood = 0;
    p->stone = 0;
    p->railCount = 0;
    p->obstacleCount = 0;
    p->railCraftProgress = 0.0f;
    p->obstacleCraftProgress = 0.0f;
    p->lastFrameTime = GetTickCount();
    p->frameDelay = 120;
    p->hasBucket = false;
    p->bucketFull = false;
    if (id == 1) {
        p->idleSheet.Load(L"Image\\Player\\Player1\\Slime1_Idle_with_shadow.png");
        p->walkSheet.Load(L"Image\\Player\\Player1\\Slime1_Walk_with_shadow.png");
    }
    else {
        p->idleSheet.Load(L"Image\\Player\\Player2\\Slime3_Idle_with_shadow.png");
        p->walkSheet.Load(L"Image\\Player\\Player2\\Slime3_Walk_with_shadow.png");
    }
}

int GetPlayerFrameCount(Player* p) {
    return p->isMoving ? 8 : 6;
}

int GetPlayerDirectionRow(Player* p) {
    if (p->dir == DIR_DOWN) return 0;
    if (p->dir == DIR_UP) return 1;
    if (p->dir == DIR_LEFT) return 2;
    if (p->dir == DIR_RIGHT) return 3;
    return 0;
}

RECT GetPlayerRect(Player* p) {
    return GetPlayerRectAt(p, p->pos);
}

void GetPlacementTile(Player* p, int* tileX, int* tileY) {
    *tileX = (int)((p->pos.x + p->size / 2.0f) / TILE_SIZE);
    *tileY = (int)((p->pos.y + p->size / 2.0f) / TILE_SIZE);

    if (p->dir == DIR_LEFT) (*tileX)--;
    else if (p->dir == DIR_RIGHT) (*tileX)++;
    else if (p->dir == DIR_UP) (*tileY)--;
    else if (p->dir == DIR_DOWN) (*tileY)++;
}

void UpdatePlayer(Player* p, bool up, bool down, bool left, bool right, MapData* map, float deltaTime) {
    Vec2 nextPos = p->pos;
    p->isMoving = false;
    float moveAmount = p->speed * deltaTime;
    float moveX = 0.0f;
    float moveY = 0.0f;

    if (up) { moveY -= 1.0f; p->dir = DIR_UP; }
    if (down) { moveY += 1.0f; p->dir = DIR_DOWN; }
    if (left) { moveX -= 1.0f; p->dir = DIR_LEFT; }
    if (right) { moveX += 1.0f; p->dir = DIR_RIGHT; }

    if (moveX != 0.0f || moveY != 0.0f) {
        float length = sqrtf(moveX * moveX + moveY * moveY);
        nextPos.x += (moveX / length) * moveAmount;
        nextPos.y += (moveY / length) * moveAmount;
        p->isMoving = true;
    }

    bool collision = false;
    RECT nextRect = GetPlayerRectAt(p, nextPos);
    if (IsSolid(map, (float)nextRect.left, (float)nextRect.top) ||
        IsSolid(map, (float)(nextRect.right - 1), (float)nextRect.top) ||
        IsSolid(map, (float)nextRect.left, (float)(nextRect.bottom - 1)) ||
        IsSolid(map, (float)(nextRect.right - 1), (float)(nextRect.bottom - 1)) ||
        IsBlockedByBaseWall(p, nextPos) ||
        IsBlockedByObstacle(p, nextPos)) {
        collision = true;
    }

    if (!collision) p->pos = nextPos;

    if (p->pos.x < 0) p->pos.x = 0;
    if (p->pos.y < 0) p->pos.y = 0;
    if (p->pos.x > MAP_WIDTH * TILE_SIZE - p->size) p->pos.x = MAP_WIDTH * TILE_SIZE - p->size;
    if (p->pos.y > MAP_HEIGHT * TILE_SIZE - p->size) p->pos.y = MAP_HEIGHT * TILE_SIZE - p->size;

    DWORD now = GetTickCount();
    if (now - p->lastFrameTime >= p->frameDelay) {
        p->frame = (p->frame + 1) % GetPlayerFrameCount(p);
        p->lastFrameTime = now;
    }
}

void DrawPlayer(Player* p, HDC hdc, Camera cam, int offsetY) {
    CImage* sheet = p->isMoving ? &p->walkSheet : &p->idleSheet;
    if (sheet->IsNull()) return;

    int frameCount = GetPlayerFrameCount(p);
    int frameW = sheet->GetWidth() / frameCount;
    int frameH = sheet->GetHeight() / 4;
    int sx = (p->frame % frameCount) * frameW;
    int sy = GetPlayerDirectionRow(p) * frameH;
    int dx = (int)(p->pos.x - cam.x);
    int dy = (int)(p->pos.y - cam.y) + offsetY;

    sheet->Draw(hdc, dx, dy, (int)p->size, (int)p->size, sx, sy, frameW, frameH);
}

void DrawInventory(Player* p, HDC hdc, Camera cam, int offsetY) {
    int x = (int)(p->pos.x - cam.x);
    int y = (int)(p->pos.y - cam.y) + offsetY;

    wchar_t text[128];
    if (game.infiniteResourceMode) {
        swprintf_s(text, L"나무:∞ / 돌:∞ / 레일:∞ / 장애물:∞");
    }
    else if (game.infiniteRailMode) {
        swprintf_s(text, L"나무:%d / 돌:%d / 레일:∞ / 장애물:%d", p->wood, p->stone, p->obstacleCount);
    }
    else if (p->railCount > 0 || p->obstacleCount > 0) {
        swprintf_s(text, L"나무:%d / 돌:%d / 레일:%d / 장애물:%d", p->wood, p->stone, p->railCount, p->obstacleCount);
    }
    else {
        swprintf_s(text, L"나무:%d / 돌:%d", p->wood, p->stone);
    }

    int oldBkMode = SetBkMode(hdc, TRANSPARENT);
    COLORREF oldColor = SetTextColor(hdc, RGB(0, 0, 0));
    HFONT oldFont = nullptr;
    if (g_inventoryFont) {
        oldFont = (HFONT)SelectObject(hdc, g_inventoryFont);
    }

    TextOutW(hdc, x, y, text, (int)wcslen(text));

    if (oldFont) SelectObject(hdc, oldFont);
    SetTextColor(hdc, oldColor);
    SetBkMode(hdc, oldBkMode);
}

void InitRail(Rail* rail) {
    rail->railImage = new Bitmap(L"Image\\train\\rail.png");
    rail->turnImage = new Bitmap(L"Image\\train\\turn rail.png");
}

void ReleaseRail(Rail* rail) {
    delete rail->railImage;
    delete rail->turnImage;
    rail->rails.clear();
}

bool HasHorizontal(Rail* rail, int tileX, int tileY) {
    for (int i = 0; i < (int)rail->rails.size(); i++) {
        RailData r = rail->rails[i];
        if (r.tileX == tileX && r.tileY == tileY && r.dir == RAIL_HORIZONTAL) return true;
    }
    return false;
}

bool HasVertical(Rail* rail, int tileX, int tileY) {
    for (int i = 0; i < (int)rail->rails.size(); i++) {
        RailData r = rail->rails[i];
        if (r.tileX == tileX && r.tileY == tileY && r.dir == RAIL_VERTICAL) return true;
    }
    return false;
}

bool HasRail(Rail* rail, int tileX, int tileY) {
    for (int i = 0; i < (int)rail->rails.size(); i++) {
        if (rail->rails[i].tileX == tileX && rail->rails[i].tileY == tileY) return true;
    }
    return false;
}

bool HasObstacle(int tileX, int tileY) {
    for (int i = 0; i < (int)game.obstacles.size(); i++) {
        if (game.obstacles[i].tileX == tileX && game.obstacles[i].tileY == tileY) return true;
    }
    return false;
}

bool CanPlaceRailAt(Rail* rail, int tileX, int tileY, RailDir dir) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) return false;
    if (HasRail(rail, tileX, tileY) || HasObstacle(tileX, tileY)) return false;

    if (dir == RAIL_HORIZONTAL) {
        if (HasVertical(rail, tileX, tileY - 1) || HasVertical(rail, tileX, tileY + 1)) return false;
    }
    if (dir == RAIL_VERTICAL) {
        if (HasHorizontal(rail, tileX - 1, tileY) || HasHorizontal(rail, tileX + 1, tileY)) return false;
    }

    return true;
}

bool CanPlaceObstacleAt(int tileX, int tileY) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) return false;
    if (HasRail(&game.rail, tileX, tileY) || HasObstacle(tileX, tileY)) return false;

    float x = (float)(tileX * TILE_SIZE);
    float y = (float)(tileY * TILE_SIZE);
    if (IsSolid(&game.map, x + 4.0f, y + 4.0f) ||
        IsSolid(&game.map, x + TILE_SIZE - 4.0f, y + 4.0f) ||
        IsSolid(&game.map, x + 4.0f, y + TILE_SIZE - 4.0f) ||
        IsSolid(&game.map, x + TILE_SIZE - 4.0f, y + TILE_SIZE - 4.0f)) {
        return false;
    }

    return true;
}

bool PlaceObstacle(int tileX, int tileY, int owner) {
    if (!CanPlaceObstacleAt(tileX, tileY)) return false;

    Obstacle obstacle;
    obstacle.tileX = tileX;
    obstacle.tileY = tileY;
    obstacle.owner = owner;
    game.obstacles.push_back(obstacle);
    return true;
}

RailDir GetRailDir(Rail* rail, int tileX, int tileY) {
    for (int i = 0; i < (int)rail->rails.size(); i++) {
        if (rail->rails[i].tileX == tileX && rail->rails[i].tileY == tileY) return rail->rails[i].dir;
    }
    return RAIL_HORIZONTAL;
}

RailDir AutoDetectRailDir(Rail* rail, int tileX, int tileY, RailDir baseDir) {
    bool left = HasRail(rail, tileX - 1, tileY);
    bool right = HasRail(rail, tileX + 1, tileY);
    bool up = HasRail(rail, tileX, tileY - 1);
    bool down = HasRail(rail, tileX, tileY + 1);

    if (baseDir == RAIL_VERTICAL) {
        if (right && (up || down)) return RAIL_TURN_RD;
        if (left && (up || down)) return RAIL_TURN_LD;
        return RAIL_VERTICAL;
    }
    if (down && (left || right)) return baseDir == RAIL_HORIZONTAL && right ? RAIL_TURN_RD : RAIL_TURN_LD;
    if (up && (left || right)) return baseDir == RAIL_HORIZONTAL && right ? RAIL_TURN_RU : RAIL_TURN_LU;
    return baseDir;
}

void UpdateRailNeighbors(Rail* rail, int tileX, int tileY) {
    int dx[] = { -1, 1, 0, 0 };
    int dy[] = { 0, 0, -1, 1 };

    for (int i = 0; i < 4; i++) {
        int nx = tileX + dx[i];
        int ny = tileY + dy[i];
        for (int j = 0; j < (int)rail->rails.size(); j++) {
            if (rail->rails[j].tileX == nx && rail->rails[j].tileY == ny) {
                rail->rails[j].dir = AutoDetectRailDir(rail, nx, ny, rail->rails[j].dir);
            }
        }
    }
}

bool PlaceRail(Rail* rail, int tileX, int tileY, RailDir dir, int owner) {
    if (!CanPlaceRailAt(rail, tileX, tileY, dir)) return false;

    RailData data;
    data.tileX = tileX;
    data.tileY = tileY;
    data.dir = AutoDetectRailDir(rail, tileX, tileY, dir);
    data.owner = owner;
    rail->rails.push_back(data);
    UpdateRailNeighbors(rail, tileX, tileY);
    return true;
}

void DrawOneRailImage(Rail* rail, Graphics* g, float x, float y, RailDir dir, bool preview) {
    GraphicsState state = g->Save();
    g->TranslateTransform(x + TILE_SIZE / 2.0f, y + TILE_SIZE / 2.0f);

    bool isTurn = (dir == RAIL_TURN_RD || dir == RAIL_TURN_LD || dir == RAIL_TURN_RU || dir == RAIL_TURN_LU);
    Bitmap* img = isTurn ? rail->turnImage : rail->railImage;

    if (dir == RAIL_VERTICAL) g->RotateTransform(90);
    else if (dir == RAIL_TURN_LD) g->RotateTransform(90);
    else if (dir == RAIL_TURN_LU) g->RotateTransform(180);
    else if (dir == RAIL_TURN_RU) g->RotateTransform(270);

    if (preview) {
        ColorMatrix cm = { 1,0,0,0,0, 0,1,0,0,0, 0,0,1,0,0, 0,0,0,0.5f,0, 0,0,0,0,1 };
        ImageAttributes ia;
        ia.SetColorMatrix(&cm);
        g->DrawImage(img, RectF(-TILE_SIZE / 2.0f, -TILE_SIZE / 2.0f, (float)TILE_SIZE, (float)TILE_SIZE),
            0, 0, (float)img->GetWidth(), (float)img->GetHeight(), UnitPixel, &ia);
    }
    else {
        g->DrawImage(img, -TILE_SIZE / 2.0f, -TILE_SIZE / 2.0f, (float)TILE_SIZE, (float)TILE_SIZE);
    }

    g->Restore(state);
}

void DrawRailPreview(Rail* rail, Graphics* g, int x, int y, RailDir dir) {
    DrawOneRailImage(rail, g, (float)x, (float)y, dir, true);
}

void DrawObstaclePreview(Graphics* g, int x, int y) {
    SolidBrush body(Color(150, 92, 66, 46));
    SolidBrush highlight(Color(150, 140, 96, 58));
    Pen edge(Color(180, 40, 28, 20), 2.0f);

    g->FillRectangle(&body, (float)x, (float)y, (float)TILE_SIZE, (float)TILE_SIZE);
    g->FillRectangle(&highlight, (float)x + 5.0f, (float)y + 7.0f, (float)TILE_SIZE - 10.0f, 6.0f);
    g->FillRectangle(&highlight, (float)x + 5.0f, (float)y + 20.0f, (float)TILE_SIZE - 10.0f, 5.0f);
    g->DrawRectangle(&edge, (float)x, (float)y, (float)TILE_SIZE, (float)TILE_SIZE);
}

void DrawPlacementPreview(Graphics* g, Player* player, RailDir railDir, Color tileColor) {
    int tileX, tileY;
    GetPlacementTile(player, &tileX, &tileY);
    int preX = tileX * TILE_SIZE;
    int preY = tileY * TILE_SIZE;

    bool hasRailItem = game.infiniteRailMode || game.infiniteResourceMode || player->railCount > 0;
    bool hasObstacleItem = game.infiniteResourceMode || player->obstacleCount > 0;
    bool canPlaceRail = hasRailItem && CanPlaceRailAt(&game.rail, tileX, tileY, railDir);
    bool canPlaceObstacle = hasObstacleItem && CanPlaceObstacleAt(tileX, tileY);

    if (!canPlaceRail && !canPlaceObstacle) return;

    bool showRail = canPlaceRail;
    if (canPlaceRail && canPlaceObstacle) {
        showRail = ((GetTickCount64() / 500) % 2) == 0;
    }

    SolidBrush previewBrush(showRail ? tileColor : Color(120, 255, 130, 70));
    g->FillRectangle(&previewBrush, (float)preX, (float)preY, (float)TILE_SIZE, (float)TILE_SIZE);

    if (showRail) DrawRailPreview(&game.rail, g, preX, preY, railDir);
    else DrawObstaclePreview(g, preX, preY);
}

void DrawRails(Rail* rail, Graphics* g, Camera cam, int viewW, int viewH) {
    const float left = cam.x - TILE_SIZE;
    const float top = cam.y - TILE_SIZE;
    const float right = cam.x + viewW + TILE_SIZE;
    const float bottom = cam.y + viewH + TILE_SIZE;

    for (int i = 0; i < (int)rail->rails.size(); i++) {
        float x = (float)(rail->rails[i].tileX * TILE_SIZE);
        float y = (float)(rail->rails[i].tileY * TILE_SIZE);

        if (x > right || x + TILE_SIZE < left || y > bottom || y + TILE_SIZE < top) continue;
        DrawOneRailImage(rail, g, x, y, rail->rails[i].dir, false);
    }
}

void DrawObstacles(Graphics* g, Camera cam, int viewW, int viewH) {
    const float left = cam.x - TILE_SIZE;
    const float top = cam.y - TILE_SIZE;
    const float right = cam.x + viewW + TILE_SIZE;
    const float bottom = cam.y + viewH + TILE_SIZE;

    SolidBrush body(Color(255, 92, 66, 46));
    SolidBrush highlight(Color(255, 140, 96, 58));
    Pen edge(Color(255, 40, 28, 20), 2.0f);

    for (int i = 0; i < (int)game.obstacles.size(); i++) {
        float x = (float)(game.obstacles[i].tileX * TILE_SIZE);
        float y = (float)(game.obstacles[i].tileY * TILE_SIZE);

        if (x > right || x + TILE_SIZE < left || y > bottom || y + TILE_SIZE < top) continue;

        g->FillRectangle(&body, x, y, (float)TILE_SIZE, (float)TILE_SIZE);
        g->FillRectangle(&highlight, x + 5.0f, y + 7.0f, (float)TILE_SIZE - 10.0f, 6.0f);
        g->FillRectangle(&highlight, x + 5.0f, y + 20.0f, (float)TILE_SIZE - 10.0f, 5.0f);
        g->DrawRectangle(&edge, x, y, (float)TILE_SIZE, (float)TILE_SIZE);
    }
}

void InitTrain(Train* train, float x, float y, int direction, const wchar_t* imagePath) {
    train->pos = { x, y };
    train->speed = 5.0f;
    train->heat = 0.0f;
    train->dirX = (float)direction;
    train->dirY = 0.0f;
    train->finished = false;
    train->lastTime = GetTickCount64();
    train->image = new Bitmap(imagePath);
}

void ReleaseTrain(Train* train) {
    delete train->image;
    train->image = nullptr;
}

bool IsTrainOverheated(Train* train) {
    return train->heat >= MAX_HEAT;
}

void UpdateTrainDirection(Train* train, RailDir rd) {
    if (rd == RAIL_HORIZONTAL) train->dirY = 0;
    else if (rd == RAIL_VERTICAL) train->dirX = 0;
    else if (rd == RAIL_TURN_RD) {
        if (train->dirX < 0) { train->dirX = 0; train->dirY = 1; }
        else if (train->dirY < 0) { train->dirX = 1; train->dirY = 0; }
    }
    else if (rd == RAIL_TURN_LD) {
        if (train->dirX > 0) { train->dirX = 0; train->dirY = 1; }
        else if (train->dirY < 0) { train->dirX = -1; train->dirY = 0; }
    }
    else if (rd == RAIL_TURN_RU) {
        if (train->dirX < 0) { train->dirX = 0; train->dirY = -1; }
        else if (train->dirY > 0) { train->dirX = 1; train->dirY = 0; }
    }
    else if (rd == RAIL_TURN_LU) {
        if (train->dirX > 0) { train->dirX = 0; train->dirY = -1; }
        else if (train->dirY > 0) { train->dirX = -1; train->dirY = 0; }
    }
}

void UpdateTrain(Train* train, Rail* rail, float deltaTime) {
    if (game.gameOver) return;
    if (train->finished) return;

    if (IsTrainOverheated(train)) {
        train->heat = MAX_HEAT;
        return;
    }

    int tileX = (int)((train->pos.x + 48) / TILE_SIZE);
    int tileY = (int)((train->pos.y + 24) / TILE_SIZE);

    if (HasObstacle(tileX, tileY)) {
        train->finished = true;
        game.gameOver = true;
        game.winner = (train == &game.train1) ? 1 : 2;
        return;
    }

    if (HasRail(rail, tileX, tileY)) {
        UpdateTrainDirection(train, GetRailDir(rail, tileX, tileY));
        train->pos.x += train->speed * train->dirX * deltaTime;
        train->pos.y += train->speed * train->dirY * deltaTime;
    }
    else {
        train->finished = true;
    }

    train->heat += HEAT_RATE * deltaTime * 60.0f;
    if (train->heat > MAX_HEAT) train->heat = MAX_HEAT;
}

void DrawHeatBar(Train* train, Graphics* g) {
    float ratio = train->heat / MAX_HEAT;
    SolidBrush bgBrush(Color(255, 0, 0, 0));
    g->FillRectangle(&bgBrush, train->pos.x, train->pos.y - 14.0f, 96.0f, 8.0f);

    BYTE r = (BYTE)(255 * ratio);
    BYTE green = (BYTE)(255 * (1.0f - ratio));
    SolidBrush heatBrush(Color(255, r, green, 0));
    g->FillRectangle(&heatBrush, train->pos.x, train->pos.y - 14.0f, 96.0f * ratio, 8.0f);
}

void DrawTrain(Train* train, Graphics* g, Camera cam, int viewW, int viewH) {
    const float padding = 120.0f;
    if (train->pos.x > cam.x + viewW + padding ||
        train->pos.x + 96.0f < cam.x - padding ||
        train->pos.y > cam.y + viewH + padding ||
        train->pos.y + 48.0f < cam.y - padding) {
        return;
    }

    g->DrawImage(train->image, train->pos.x, train->pos.y, 96.0f, 48.0f);

    if (IsTrainOverheated(train) && g_overHeatBrush) {
        g->FillRectangle(g_overHeatBrush, train->pos.x, train->pos.y, 96.0f, 48.0f);
    }

    DrawHeatBar(train, g);
}

void InitResource(Resource* resource, ResourceType type, float x, float y) {
    resource->type = type;
    resource->pos = { x, y };
    resource->spawnPos = { x, y };
    resource->size = 64.0f;
    resource->harvestProgress = 0.0f;
    resource->active = true;
    resource->respawnStartTime = 0;
    resource->respawnDelay = 10000;
}

RECT GetResourceRect(Resource* resource) {
    RECT rc;
    rc.left = (LONG)(resource->pos.x + 8);
    rc.top = (LONG)(resource->pos.y + 8);
    rc.right = (LONG)(resource->pos.x + resource->size - 8);
    rc.bottom = (LONG)(resource->pos.y + resource->size - 4);
    return rc;
}

bool ResourceIntersectsPlayer(Resource* resource, Player* player) {
    if (!resource->active) return false;

    RECT hit;
    RECT resourceRect = GetResourceRect(resource);
    RECT playerRect = GetPlayerRect(player);
    return IntersectRect(&hit, &resourceRect, &playerRect);
}

bool HasRailOnResourceSpawn(Resource* resource, Rail* rail) {
    int tileX = (int)((resource->spawnPos.x + resource->size / 2.0f) / TILE_SIZE);
    int tileY = (int)((resource->spawnPos.y + resource->size / 2.0f) / TILE_SIZE);
    return HasRail(rail, tileX, tileY);
}

void StartResourceRespawn(Resource* resource) {
    resource->active = false;
    resource->harvestProgress = 0.0f;
    resource->respawnStartTime = GetTickCount64();
}

void HarvestResource(Resource* resource, Player* player, float deltaTime) {
    resource->harvestProgress += deltaTime;
    if (resource->harvestProgress >= 1.0f) {
        if (resource->type == RESOURCE_TREE) player->wood++;
        else player->stone++;
        StartResourceRespawn(resource);
    }
}

void UpdateResource(Resource* resource, Player* p1, Player* p2, Rail* rail, float deltaTime) {
    if (resource->active && HasRailOnResourceSpawn(resource, rail)) {
        StartResourceRespawn(resource);
    }

    if (!resource->active) {
        if (GetTickCount64() - resource->respawnStartTime >= resource->respawnDelay) {
            float newX, newY;
            if (FindRandomResourcePosition(&newX, &newY)) {
                resource->pos = { newX, newY };
                resource->spawnPos = resource->pos;
                resource->active = true;
            }
        }
        return;
    }

    if (ResourceIntersectsPlayer(resource, p1)) HarvestResource(resource, p1, deltaTime);
    else if (ResourceIntersectsPlayer(resource, p2)) HarvestResource(resource, p2, deltaTime);
}

void DrawResource(Resource* resource, Graphics* g) {
    if (!resource->active) return;

    float scale = 1.0f - resource->harvestProgress * 0.6f;
    float drawSize = resource->size * scale;
    float drawX = resource->pos.x + (resource->size - drawSize) / 2.0f;
    float drawY = resource->pos.y + (resource->size - drawSize) / 2.0f;

    if (resource->type == RESOURCE_TREE) {
        SolidBrush trunk(Color(255, 120, 72, 32));
        SolidBrush leaves(Color(255, 32, 150, 76));
        g->FillRectangle(&trunk, drawX + drawSize * 0.42f, drawY + drawSize * 0.48f, drawSize * 0.16f, drawSize * 0.42f);
        g->FillEllipse(&leaves, drawX + drawSize * 0.12f, drawY + drawSize * 0.02f, drawSize * 0.76f, drawSize * 0.62f);
    }
    else {
        SolidBrush rock(Color(255, 115, 120, 130));
        Pen edge(Color(255, 75, 80, 90), 2.0f);
        RectF rect(drawX + drawSize * 0.08f, drawY + drawSize * 0.22f, drawSize * 0.84f, drawSize * 0.58f);
        g->FillEllipse(&rock, rect);
        g->DrawEllipse(&edge, rect);
    }
}

bool CanPlaceResourceAt(float x, float y) {
    float padding = 8.0f;
    float size = 64.0f;
    RECT resourceRect = {
        (LONG)x,
        (LONG)y,
        (LONG)(x + size),
        (LONG)(y + size)
    };

    if (IsRectInsideAnyBase(resourceRect)) return false;

    if (IsSolid(&game.map, x + padding, y + padding) ||
        IsSolid(&game.map, x + size - padding, y + padding) ||
        IsSolid(&game.map, x + padding, y + size - padding) ||
        IsSolid(&game.map, x + size - padding, y + size - padding)) {
        return false;
    }

    int tileX = (int)((x + size / 2.0f) / TILE_SIZE);
    int tileY = (int)((y + size / 2.0f) / TILE_SIZE);
    return !HasRail(&game.rail, tileX, tileY) && !HasObstacle(tileX, tileY);
}

bool FindRandomResourcePosition(float* outX, float* outY, int preferredSide) {
    int centerTileX = MAP_WIDTH / 2;
    int sideStartTileX[2] = { 2, centerTileX + 2 };
    int sideEndTileX[2] = { centerTileX - 3, MAP_WIDTH - 3 };

    for (int attempt = 0; attempt < 5000; attempt++) {
        int side = (preferredSide == 0 || preferredSide == 1) ? preferredSide : rand() % 2;
        int width = sideEndTileX[side] - sideStartTileX[side] + 1;
        int tileX = sideStartTileX[side] + (rand() % width);
        int tileY = 3 + (rand() % (MAP_HEIGHT - 6));
        float x = (float)(tileX * TILE_SIZE);
        float y = (float)(tileY * TILE_SIZE);

        bool samePlace = false;
        for (int i = 0; i < (int)game.resources.size(); i++) {
            if (!game.resources[i].active) continue;

            int oldTileX = (int)(game.resources[i].spawnPos.x / TILE_SIZE);
            int oldTileY = (int)(game.resources[i].spawnPos.y / TILE_SIZE);
            if (oldTileX == tileX && oldTileY == tileY) {
                samePlace = true;
                break;
            }
        }

        if (!samePlace && CanPlaceResourceAt(x, y)) {
            *outX = x;
            *outY = y;
            return true;
        }
    }

    return false;
}

void CreateResources() {
    int resourcesPerSide = 25;

    game.resources.clear();
    game.resources.reserve(resourcesPerSide * 2);

    for (int side = 0; side < 2; side++) {
        int created = 0;

        while (created < resourcesPerSide) {
            float x, y;
            if (FindRandomResourcePosition(&x, &y, side)) {
                Resource resource;
                InitResource(&resource, (created % 2 == 0) ? RESOURCE_TREE : RESOURCE_ROCK, x, y);
                game.resources.push_back(resource);
                created++;
            }
            else {
                break;
            }
        }
    }
}

void DrawVictoryScreen(Graphics* g) {
    SolidBrush overlay(Color(180, 0, 0, 0));
    g->FillRectangle(&overlay, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, 80, FontStyleBold, UnitPixel);
    SolidBrush textBrush(Color(255, 255, 255, 0));
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);

    std::wstring msg;
    if (game.winner == 1) msg = L"Player 2 WIN!";
    else if (game.winner == 2) msg = L"Player 1 WIN!";
    else msg = L"DRAW!";

    RectF rect(0.0f, 0.0f, (REAL)SCREEN_WIDTH, (REAL)SCREEN_HEIGHT);
    g->DrawString(msg.c_str(), -1, &font, rect, &sf, &textBrush);
}

