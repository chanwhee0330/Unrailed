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
#define MAP_WIDTH 60
#define MAP_HEIGHT 30
#define TILE_SIZE 32
#define BASE_WIDTH_TILES 10
#define BASE_HEIGHT_TILES 10
#define BASE_DOOR_HEIGHT_TILES 4
#define BASE_START_RAIL_TILES 11
#define RAIL_CRAFT_TIME 1.0f
#define OBSTACLE_CRAFT_TIME 1.5f
#define BOMB_CRAFT_TIME 2.0f
#define OBSTACLE_WOOD_COST 2
#define OBSTACLE_STONE_COST 2
#define BOMB_WOOD_COST 4
#define BOMB_STONE_COST 4
#define BOMB_EXPLODE_TIME 1.0f
#define EXPLOSION_EFFECT_TIME 0.45f
#define BASE_EXPLOSION_FLASH_TIME 1.4f
#define PLAYER_SIZE 64.0f
#define PLAYER_COLLISION_LEFT 14
#define PLAYER_COLLISION_RIGHT 50
#define PLAYER_COLLISION_TOP 17
#define PLAYER_COLLISION_BOTTOM 42
#define TRAIN_WIDTH 96.0f
#define TRAIN_HEIGHT 48.0f
#define TRAIN_BOOST_MULTIPLIER 8.0f
#define TRAIN_ROT_SPEED 180.0f  // 기차 회전 속도(초당 도). 작을수록 천천히 회전
#define BUCKET_SIZE 64.0f
#define HELD_BUCKET_SIZE 48.0f
#define RESOURCE_SIZE 64.0f

#define MAX_HEAT 100.0f
#define HEAT_RATE 0.035f
#define TIMER_ID 1
#define TARGET_FRAME_MS 16
#define MAX_FRAME_DELTA 0.05f
#define marginX 16
#define marginY 39

#define START_BTN_LEFT   440
#define START_BTN_RIGHT  840
#define START_BTN_TOP    548
#define START_BTN_BOTTOM 650

// 시작화면 우상단 "조작법" 버튼 (Helper.png, 659x379 비율 유지)
#define CTRL_BTN_LEFT    1142
#define CTRL_BTN_RIGHT   1252
#define CTRL_BTN_TOP     18
#define CTRL_BTN_BOTTOM  81

// 조작법 화면 "뒤로" 버튼
#define CTRL_BACK_LEFT   560
#define CTRL_BACK_RIGHT  720
#define CTRL_BACK_TOP    648
#define CTRL_BACK_BOTTOM 698

struct Vec2 {
    float x, y;
};

struct Camera {
    float x, y;
};

enum PlayerDir { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_UP };
enum RailDir { RAIL_HORIZONTAL, RAIL_VERTICAL, RAIL_TURN_RD, RAIL_TURN_LD, RAIL_TURN_RU, RAIL_TURN_LU };
enum ResourceType { RESOURCE_TREE, RESOURCE_ROCK };
enum PlacementType { PLACEMENT_RAIL, PLACEMENT_OBSTACLE, PLACEMENT_BOMB, PLACEMENT_NONE };
enum GameState { STATE_START, STATE_GAME_START, STATE_PLAYING, STATE_PAUSE_MENU, STATE_SETTINGS, STATE_CONTROLS };

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
    int bombCount;
    float railCraftProgress;
    float obstacleCraftProgress;
    float bombCraftProgress;
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
    // 소유자(0=중립,1,2) × 방향(RailDir 6종)별로 회전·틴트까지 미리 구워둔 32x32 이미지
    // → 매 프레임 Save/Restore/Rotate 없이 단순 1장 그리기만 하면 됨
    Bitmap* variant[3][6];
    int8_t grid[MAP_HEIGHT][MAP_WIDTH];      // -1=없음, 0이상=RailDir
    int8_t ownerGrid[MAP_HEIGHT][MAP_WIDTH]; // 0=없음, 1=P1, 2=P2
};

struct Obstacle {
    int tileX, tileY;
    int owner;
};

struct Bomb {
    int tileX, tileY;
    int owner;
    float timer;
};

struct ExplosionEffect {
    float x, y;
    float timer;
};

struct Train {
    Vec2 pos;
    float speed;
    float heat;
    float dirX, dirY;
    bool finished;
    int bombCargo;
    ULONGLONG lastTime;
    Bitmap* image;
    float renderAngle; // 화면에 표시되는 회전 각도(도). 목표 방향으로 부드럽게 보간됨
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
    std::vector<Bomb> bombs;
    std::vector<ExplosionEffect> explosions;
    bool obstacleGrid[MAP_HEIGHT][MAP_WIDTH];
    bool gameOver;
    int winner;
    bool baseExplosionActive;
    ULONGLONG baseExplosionStartTime;
    bool rKeyPrev;
    bool twoKeyPrev;
    bool f2KeyPrev;
    bool infiniteRailMode;
    bool infiniteResourceMode;
    HDC memDC;
    HBITMAP memBitmap;
    HBITMAP oldBitmap;
    ULONGLONG lastUpdateTime;
    RailDir selectedDir1;
    RailDir selectedDir2;
    PlacementType selectedPlacement1;
    PlacementType selectedPlacement2;
    Bucket bucket1; // p1용
    Bucket bucket2; // p2용
    bool fKeyPrev;
    bool eKeyPrev;
    bool threeKeyPrev;
    bool qKeyPrev;
    bool oneKeyPrev;
    bool zeroKeyPrev;
    GameState gameState;
    Bitmap* startScreenImage;
    int volume;
    bool volDragging;
    float gameStartCountdown;
    bool trainSoundStarted;
};
Bitmap* g_emptyBucket = nullptr;
Bitmap* g_fullBucket = nullptr;
Bitmap* g_bombImage = nullptr;
Bitmap* g_emergencyImage = nullptr;
Bitmap* g_helperImage = nullptr;
HFONT g_inventoryFont = nullptr;
SolidBrush* g_overHeatBrush = nullptr;
POINT g_mousePos = { 0, 0 };
// 전체화면 상태 + 백버퍼 출력 영역(마우스 좌표 변환용)
bool g_fullscreen = false;
RECT g_windowedRect = { 0, 0, 0, 0 };
DWORD g_windowedStyle = 0;
int g_presentX = 0, g_presentY = 0, g_presentW = SCREEN_WIDTH, g_presentH = SCREEN_HEIGHT;
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
bool IsWorldRectVisible(float x, float y, float w, float h, Camera cam, int viewW, int viewH, float padding = 0.0f);
bool IsPlayerInsideBase(Player* p, BaseArea base);
bool IsBlockedByBaseWall(Player* p, Vec2 nextPos);
bool IsBlockedByObstacle(Player* p, Vec2 nextPos);
bool IsRectInsideAnyBase(RECT rc);
bool DoesTileOverlapAnyPlayer(int tileX, int tileY);
RECT GetTrainRect(Train* train);
RECT GetRailCraftStationRect(BaseArea base);
RECT GetObstacleCraftStationRect(BaseArea base);
RECT GetBombCraftStationRect(BaseArea base);
bool IsPlayerTouchingRailCraftStation(Player* p);
bool IsPlayerTouchingObstacleCraftStation(Player* p);
bool IsPlayerTouchingBombCraftStation(Player* p);
void UpdateRailCraft(Player* p, float deltaTime);
void UpdateObstacleCraft(Player* p, float deltaTime);
void UpdateBombCraft(Player* p, float deltaTime);
void DrawRailCraftStations(Graphics* g, Player* viewer);
void DrawObstacleCraftStations(Graphics* g, Player* viewer);
void DrawBombCraftStations(Graphics* g, Player* viewer);
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
bool HasBomb(int tileX, int tileY);
bool CanPlaceRailAt(Rail* rail, int tileX, int tileY, RailDir dir, int owner);
bool CanPlaceObstacleAt(int tileX, int tileY);
bool CanPlaceBombAt(int tileX, int tileY);
bool PlaceObstacle(int tileX, int tileY, int owner);
bool PlaceBomb(int tileX, int tileY, int owner);
void AddExplosion(float x, float y);
void AddBaseExplosion(BaseArea base);
void UpdateBombs(float deltaTime);
void UpdateExplosions(float deltaTime);
bool HandleBombTrainCollision();
void UpdateRailNeighbors(Rail* rail, int tileX, int tileY);
bool PlaceRail(Rail* rail, int tileX, int tileY, RailDir dir, int owner);
PlacementType GetNextPlacementType(Player* player, PlacementType placementType);
bool PlaceSelectedItem(Player* player, PlacementType placementType, RailDir railDir, int owner);
void DrawOneRailImage(Rail* rail, Graphics* g, float x, float y, RailDir dir, bool preview, int owner);
void DrawRailPreview(Rail* rail, Graphics* g, int x, int y, RailDir dir, int owner);
void DrawObstaclePreview(Graphics* g, int x, int y);
void DrawBombIcon(Graphics* g, float x, float y, float size, BYTE alpha);
void DrawBombPreview(Graphics* g, int x, int y);
void DrawPlacementPreview(Graphics* g, Player* player, PlacementType placementType, RailDir railDir, Color tileColor);
void DrawRails(Rail* rail, Graphics* g, Camera cam, int viewW, int viewH);
void DrawObstacles(Graphics* g, Camera cam, int viewW, int viewH);
void DrawBombs(Graphics* g, Camera cam, int viewW, int viewH);
void DrawExplosions(Graphics* g, Camera cam, int viewW, int viewH);
void InitTrain(Train* train, float x, float y, int direction, const wchar_t* imagePath);
void ReleaseTrain(Train* train);
bool IsTrainOverheated(Train* train);
void UpdateTrainDirection(Train* train, RailDir rd);
int CountRailsAhead(Train* train, Rail* rail, int maxCount);
void UpdateTrain(Train* train, Rail* rail, float deltaTime, float speedMultiplier = 1.0f);
void DrawHeatBar(Train* train, Graphics* g);
void DrawTrain(Train* train, Graphics* g, Camera cam, int viewW, int viewH);
void InitResource(Resource* resource, ResourceType type, float x, float y);
RECT GetResourceRect(Resource* resource);
bool ResourceIntersectsPlayer(Resource* resource, Player* player);
bool HasRailOnResourceSpawn(Resource* resource, Rail* rail);
void StartResourceRespawn(Resource* resource);
void HarvestResource(Resource* resource, Player* player, float deltaTime);
void UpdateResource(Resource* resource, Player* p1, Player* p2, Rail* rail, float deltaTime);
void DrawResource(Resource* resource, Graphics* g, Camera cam, int viewW, int viewH);
bool CanPlaceResourceAt(float x, float y);
bool FindRandomResourcePosition(float* outX, float* outY, int preferredSide = -1);
void CreateResources();
void DrawVictoryScreen(Graphics* g);
void DrawBaseExplosionOverlay(Graphics* g, float elapsed);
void DrawMenuButton(Graphics* g, const wchar_t* text, int x, int y, int w, int h);
void DrawPauseMenu(Graphics* g);
void DrawSettingsMenu(Graphics* g);
void DrawControlsScreen(Graphics* g);
void DrawGameStartOverlay(Graphics* g);
bool IsWater(MapData* map, float x, float y) {
    if (x < 0 || y < 0 || x >= map->pixelW || y >= map->pixelH) return false;
    if (map->waterBits.empty()) return false;
    int index = (int)y * map->pixelW + (int)x;
    return (map->waterBits[index / 8] & (1 << (index % 8))) != 0;
}

void ApplyVolume(int vol) {
    DWORD v = (DWORD)(vol * 0xFFFF / 100);
    waveOutSetVolume(0, (v << 16) | v);
}

// 1280x720 백버퍼를 클라이언트 영역에 비율 유지(레터박스)로 늘려 출력
void PresentBackBuffer(HWND hWnd, HDC hDC) {
    RECT cr; GetClientRect(hWnd, &cr);
    int cw = cr.right - cr.left;
    int ch = cr.bottom - cr.top;
    if (cw <= 0 || ch <= 0) return;

    float sx = (float)cw / SCREEN_WIDTH;
    float sy = (float)ch / SCREEN_HEIGHT;
    float scale = (sx < sy) ? sx : sy;
    int dw = (int)(SCREEN_WIDTH * scale + 0.5f);
    int dh = (int)(SCREEN_HEIGHT * scale + 0.5f);
    int dx = (cw - dw) / 2;
    int dy = (ch - dh) / 2;
    g_presentX = dx; g_presentY = dy; g_presentW = dw; g_presentH = dh;

    // 레터박스(남는 영역) 검은색으로 채움
    if (dx > 0 || dy > 0) {
        HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH);
        RECT full = { 0, 0, cw, ch };
        FillRect(hDC, &full, black);
    }

    if (dw == SCREEN_WIDTH && dh == SCREEN_HEIGHT)
        BitBlt(hDC, dx, dy, dw, dh, game.memDC, 0, 0, SRCCOPY); // 1:1이면 빠른 복사
    else {
        SetStretchBltMode(hDC, COLORONCOLOR);
        StretchBlt(hDC, dx, dy, dw, dh, game.memDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SRCCOPY);
    }
}

// 클라이언트 마우스 좌표 → 1280x720 논리 좌표로 변환
void MapMouseToLogical(int* mx, int* my) {
    if (g_presentW > 0) *mx = (*mx - g_presentX) * SCREEN_WIDTH / g_presentW;
    if (g_presentH > 0) *my = (*my - g_presentY) * SCREEN_HEIGHT / g_presentH;
}

void ToggleFullscreen(HWND hWnd) {
    g_fullscreen = !g_fullscreen;
    if (g_fullscreen) {
        g_windowedStyle = GetWindowLong(hWnd, GWL_STYLE);
        GetWindowRect(hWnd, &g_windowedRect);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &mi);
        SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(hWnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    else {
        SetWindowLong(hWnd, GWL_STYLE, g_windowedStyle);
        SetWindowPos(hWnd, HWND_TOP,
            g_windowedRect.left, g_windowedRect.top,
            g_windowedRect.right - g_windowedRect.left,
            g_windowedRect.bottom - g_windowedRect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    InvalidateRect(hWnd, NULL, TRUE);
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
        game.volume = 80;
        game.volDragging = false;
        game.gameStartCountdown = 0.0f;
        game.trainSoundStarted = false;
        PlaySound(L"Sound\\start sound.wav", NULL, SND_FILENAME | SND_LOOP | SND_ASYNC);
        ApplyVolume(game.volume);
        game.gameOver = false;
        game.winner = 0;
        game.baseExplosionActive = false;
        game.baseExplosionStartTime = 0;
        game.rKeyPrev = false;
        game.twoKeyPrev = false;
        game.f2KeyPrev = false;
        game.infiniteRailMode = false;
        game.infiniteResourceMode = false;
        game.selectedDir1 = RAIL_HORIZONTAL;
        game.selectedDir2 = RAIL_HORIZONTAL;
        game.selectedPlacement1 = PLACEMENT_RAIL;
        game.selectedPlacement2 = PLACEMENT_RAIL;
        BaseArea base1 = GetBaseArea(0);
        BaseArea base2 = GetBaseArea(1);
        game.bucket1 = { {(float)(base1.x + 400), (float)(base1.y + 112)}, false };
        game.bucket2 = { {(float)(base2.x - 160), (float)(base2.y + 124)}, false };
        game.fKeyPrev = false;
        game.eKeyPrev = false;
        game.threeKeyPrev = false;
        game.qKeyPrev = false;
        game.oneKeyPrev = false;
        game.zeroKeyPrev = false;
        game.obstacles.clear();
        game.bombs.clear();
        game.explosions.clear();
        memset(game.obstacleGrid, 0, sizeof(game.obstacleGrid));
        g_inventoryFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        g_overHeatBrush = new SolidBrush(Color(120, 255, 0, 0));

        InitPlayer(&game.p1, 1, (float)(base1.x + 300), (float)(base1.y + 112));
        InitPlayer(&game.p2, 2, (float)(base2.x - 60), (float)(base2.y + 124));
        InitMap(&game.map);
        LoadMapCsv(&game.map, L"Image\\Map\\unTiled map_Tile Layer 1.csv");
        InitRail(&game.rail);
        g_emptyBucket = new Bitmap(L"Image\\train\\bucket.png");  // 경로 채워줘
        g_fullBucket = new Bitmap(L"Image\\train\\waterbucket.png");  // 경로 채워줘
        g_bombImage = new Bitmap(L"Image\\train\\bomb.png");
        g_emergencyImage = new Bitmap(L"Image\\train\\emergemcy.png");
        g_helperImage = new Bitmap(L"Image\\scene\\Helper.png");
        HDC hdc = GetDC(hWnd);
        game.memDC = CreateCompatibleDC(hdc);
        game.memBitmap = CreateCompatibleBitmap(hdc, SCREEN_WIDTH, SCREEN_HEIGHT);
        game.oldBitmap = (HBITMAP)SelectObject(game.memDC, game.memBitmap);
        ReleaseDC(hWnd, hdc);

        InitTrain(&game.train1, (float)(base1.x + 64), (float)(base1.y + 136), 1, L"Image\\train\\locomoto.png");
        InitTrain(&game.train2, (float)(base2.x + 160), (float)(base2.y + 136), -1, L"Image\\train\\locomoto2.png");

        game.cam1 = { 0, 0 };
        game.cam2 = { 0, 0 };
        game.lastUpdateTime = GetTickCount64();

        // start rail
        int t1X = (int)(game.train1.pos.x / TILE_SIZE);
        int t1Y = (int)((game.train1.pos.y + TRAIN_HEIGHT / 2.0f) / TILE_SIZE);
        for (int i = 0; i < BASE_START_RAIL_TILES; i++) PlaceRail(&game.rail, t1X + i, t1Y, RAIL_HORIZONTAL, 1);

        int t2X = (int)(game.train2.pos.x / TILE_SIZE);
        int t2Y = (int)((game.train2.pos.y + TRAIN_HEIGHT / 2.0f) / TILE_SIZE);
        for (int i = 0; i < BASE_START_RAIL_TILES; i++) PlaceRail(&game.rail, t2X + 2 - i, t2Y, RAIL_HORIZONTAL, 2);

        CreateResources();
        SetTimer(hWnd, TIMER_ID, TARGET_FRAME_MS, NULL);
        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == TIMER_ID) {
            if (game.gameState == STATE_START ||
                game.gameState == STATE_CONTROLS ||
                game.gameState == STATE_PAUSE_MENU ||
                game.gameState == STATE_SETTINGS) {
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }

            if (game.gameState == STATE_GAME_START) {
                ULONGLONG now = GetTickCount64();
                float dt = (float)(now - game.lastUpdateTime) / 1000.0f;
                if (dt > MAX_FRAME_DELTA) dt = MAX_FRAME_DELTA;
                game.lastUpdateTime = now;

                // 카메라를 플레이어 위치로 맞춤
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

                game.gameStartCountdown -= dt;

                // 클릭음(0.5s)이 끝난 뒤 기차음 재생
                if (!game.trainSoundStarted && game.gameStartCountdown <= 5.0f) {
                    PlaySound(L"Sound\\trainsound.wav", NULL, SND_FILENAME | SND_ASYNC);
                    game.trainSoundStarted = true;
                }

                if (game.gameStartCountdown <= 0.0f) {
                    PlaySound(L"Sound\\gamevolume.wav", NULL, SND_FILENAME | SND_LOOP | SND_ASYNC);
                    game.gameState = STATE_PLAYING;
                }
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
                UpdateBombCraft(&game.p1, deltaTime);
                UpdateBombCraft(&game.p2, deltaTime);
                UpdateBombs(deltaTime);
                UpdateExplosions(deltaTime);

                // placement type change
                bool qKey = GetAsyncKeyState('Q') & 0x8000;
                if (qKey && !game.qKeyPrev) {
                    game.selectedPlacement1 = GetNextPlacementType(&game.p1, game.selectedPlacement1);
                }
                game.qKeyPrev = qKey;

                bool oneKey = (GetAsyncKeyState(VK_NUMPAD1) & 0x8000) || (GetAsyncKeyState('1') & 0x8000);
                if (oneKey && !game.oneKeyPrev) {
                    game.selectedPlacement2 = GetNextPlacementType(&game.p2, game.selectedPlacement2);
                }
                game.oneKeyPrev = oneKey;

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

                // place selected item
                bool eKey = GetAsyncKeyState('E') & 0x8000;
                if (eKey && !game.eKeyPrev) {
                    PlaceSelectedItem(&game.p1, game.selectedPlacement1, game.selectedDir1, 1);
                }
                game.eKeyPrev = eKey;

                bool threeKey = (GetAsyncKeyState('3') & 0x8000) || (GetAsyncKeyState(VK_NUMPAD3) & 0x8000);
                if (threeKey && !game.threeKeyPrev) {
                    PlaceSelectedItem(&game.p2, game.selectedPlacement2, game.selectedDir2, 2);
                }
                game.threeKeyPrev = threeKey;

                // Player1
                // Player1 - F키 줍기/놓기
                bool fKey = GetAsyncKeyState('F') & 0x8000;
                if (fKey && !game.fKeyPrev) {
                    if (!game.p1.hasBucket && !game.bucket1.pickedUp) {
                        RECT pr = GetPlayerRect(&game.p1);
                        RECT br = { (LONG)game.bucket1.pos.x, (LONG)game.bucket1.pos.y,
                                    (LONG)(game.bucket1.pos.x + BUCKET_SIZE), (LONG)(game.bucket1.pos.y + BUCKET_SIZE) };
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
                                (LONG)(game.train1.pos.x + TRAIN_WIDTH), (LONG)(game.train1.pos.y + TRAIN_HEIGHT) };
                    if (RectsOverlap(pr, tr)) {
                        game.train1.heat -= 90.0f;
                        if (game.train1.heat < 0) game.train1.heat = 0;
                        game.p1.bucketFull = false;
                    }
                }

                // Player2
               // Player2 - 0키 줍기/놓기
                bool zeroKey = (GetAsyncKeyState('0') & 0x8000) || (GetAsyncKeyState(VK_NUMPAD0) & 0x8000);
                if (zeroKey && !game.zeroKeyPrev) {
                    if (!game.p2.hasBucket && !game.bucket2.pickedUp) {
                        RECT pr = GetPlayerRect(&game.p2);
                        RECT br = { (LONG)game.bucket2.pos.x, (LONG)game.bucket2.pos.y,
                                    (LONG)(game.bucket2.pos.x + BUCKET_SIZE), (LONG)(game.bucket2.pos.y + BUCKET_SIZE) };
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
                game.zeroKeyPrev = zeroKey;
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
                                (LONG)(game.train2.pos.x + TRAIN_WIDTH), (LONG)(game.train2.pos.y + TRAIN_HEIGHT) };
                    if (RectsOverlap(pr, tr)) {
                        game.train2.heat -= 90.0f;
                        if (game.train2.heat < 0) game.train2.heat = 0;
                        game.p2.bucketFull = false;
                    }
                }
                // train move
                float train1SpeedMultiplier = (GetAsyncKeyState(VK_F4) & 0x8000) ? TRAIN_BOOST_MULTIPLIER : 1.0f;
                float train2SpeedMultiplier = (GetAsyncKeyState(VK_F5) & 0x8000) ? TRAIN_BOOST_MULTIPLIER : 1.0f;
                UpdateTrain(&game.train1, &game.rail, deltaTime, train1SpeedMultiplier);
                HandleBombTrainCollision();
                UpdateTrain(&game.train2, &game.rail, deltaTime, train2SpeedMultiplier);
                HandleBombTrainCollision();

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

            // 시작화면 볼륨 슬라이더 (우하단)
            // 슬라이더 트랙: sx=1020, sy=672, sw=200, sh=10
            {
                const int sx = 1020, sy = 672, sw = 200, sh = 10;
                // 반투명 배경
                SolidBrush bgBox(Color(160, 0, 0, 0));
                g.FillRectangle(&bgBox, sx - 60, sy - 14, sw + 70, sh + 28);

                // "볼륨" 레이블
                FontFamily ff(L"Arial");
                Font lf(&ff, 14, FontStyleBold, UnitPixel);
                SolidBrush white(Color(255, 255, 255, 255));
                StringFormat sfL;
                g.DrawString(L"볼륨", -1, &lf, PointF((REAL)(sx - 52), (REAL)(sy - 2)), &sfL, &white);

                // 트랙
                SolidBrush trackBg(Color(255, 80, 80, 80));
                g.FillRectangle(&trackBg, sx, sy, sw, sh);

                // 채워진 부분
                int filled = sw * game.volume / 100;
                SolidBrush fillBrush(Color(255, 100, 180, 255));
                g.FillRectangle(&fillBrush, sx, sy, filled, sh);

                // 썸
                SolidBrush thumbBrush(Color(255, 230, 230, 230));
                g.FillRectangle(&thumbBrush, sx + filled - 8, sy - 5, 16, 20);

                // 퍼센트 텍스트
                wchar_t volStr[16];
                swprintf_s(volStr, L"%d%%", game.volume);
                g.DrawString(volStr, -1, &lf, PointF((REAL)(sx + sw + 6), (REAL)(sy - 2)), &sfL, &white);
            }

            // 우상단 "조작법" 버튼 (Helper 이미지)
            if (g_helperImage && g_helperImage->GetLastStatus() == Ok) {
                g.DrawImage(g_helperImage, CTRL_BTN_LEFT, CTRL_BTN_TOP,
                    CTRL_BTN_RIGHT - CTRL_BTN_LEFT, CTRL_BTN_BOTTOM - CTRL_BTN_TOP);
            }
            else {
                DrawMenuButton(&g, L"조작법", CTRL_BTN_LEFT, CTRL_BTN_TOP,
                    CTRL_BTN_RIGHT - CTRL_BTN_LEFT, CTRL_BTN_BOTTOM - CTRL_BTN_TOP);
            }

            PresentBackBuffer(hWnd, hDC);
            EndPaint(hWnd, &ps);
            return 0;
        }

        if (game.gameState == STATE_CONTROLS) {
            DrawControlsScreen(&g);
            PresentBackBuffer(hWnd, hDC);
            EndPaint(hWnd, &ps);
            return 0;
        }

        // draw to back buffer
        int halfH = SCREEN_HEIGHT / 2;

        // top screen
        Graphics g1(game.memDC);
        // 빠른 렌더링 모드 (기본 고품질 보간 대신 최저 비용) — 끊김 완화
        g1.SetInterpolationMode(InterpolationModeNearestNeighbor);
        g1.SetPixelOffsetMode(PixelOffsetModeHalf);
        g1.SetSmoothingMode(SmoothingModeNone);
        g1.SetCompositingQuality(CompositingQualityHighSpeed);
        g1.SetClip(Rect(0, 0, SCREEN_WIDTH, halfH));
        g1.Clear(Color(255, 255, 255));
        g1.TranslateTransform(-game.cam1.x, -game.cam1.y);
        DrawMap(&game.map, &g1, game.cam1, SCREEN_WIDTH, halfH);

        for (int i = 0; i < (int)game.resources.size(); i++) DrawResource(&game.resources[i], &g1, game.cam1, SCREEN_WIDTH, halfH);
        DrawRails(&game.rail, &g1, game.cam1, SCREEN_WIDTH, halfH);
        DrawObstacles(&g1, game.cam1, SCREEN_WIDTH, halfH);
        DrawBombs(&g1, game.cam1, SCREEN_WIDTH, halfH);
        DrawExplosions(&g1, game.cam1, SCREEN_WIDTH, halfH);
        DrawTrain(&game.train1, &g1, game.cam1, SCREEN_WIDTH, halfH);
        DrawTrain(&game.train2, &g1, game.cam1, SCREEN_WIDTH, halfH);
        DrawPlacementPreview(&g1, &game.p1, game.selectedPlacement1, game.selectedDir1, Color(120, 255, 255, 0));
        // 바닥에 있는 양동이
        if (!game.bucket1.pickedUp &&
            IsWorldRectVisible(game.bucket1.pos.x, game.bucket1.pos.y, BUCKET_SIZE, BUCKET_SIZE, game.cam1, SCREEN_WIDTH, halfH))
            g1.DrawImage(g_emptyBucket, game.bucket1.pos.x, game.bucket1.pos.y, BUCKET_SIZE, BUCKET_SIZE);

        // 플레이어가 든 양동이
        if (game.p1.hasBucket &&
            IsWorldRectVisible(game.p1.pos.x, game.p1.pos.y, game.p1.size, game.p1.size, game.cam1, SCREEN_WIDTH, halfH)) {
            Bitmap* img = game.p1.bucketFull ? g_fullBucket : g_emptyBucket;
            g1.DrawImage(img, game.p1.pos.x + 15.0f, game.p1.pos.y + 8.0f, HELD_BUCKET_SIZE, HELD_BUCKET_SIZE);
        }
        DrawBases(&g1, &game.p1);
        DrawRailCraftStations(&g1, &game.p1);
        DrawObstacleCraftStations(&g1, &game.p1);
        DrawBombCraftStations(&g1, &game.p1);

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
        g2.SetInterpolationMode(InterpolationModeNearestNeighbor);
        g2.SetPixelOffsetMode(PixelOffsetModeHalf);
        g2.SetSmoothingMode(SmoothingModeNone);
        g2.SetCompositingQuality(CompositingQualityHighSpeed);
        g2.SetClip(Rect(0, halfH, SCREEN_WIDTH, halfH));
        g2.TranslateTransform(0, (REAL)halfH);
        g2.TranslateTransform(-game.cam2.x, -game.cam2.y);
        DrawMap(&game.map, &g2, game.cam2, SCREEN_WIDTH, halfH);

        for (int i = 0; i < (int)game.resources.size(); i++) DrawResource(&game.resources[i], &g2, game.cam2, SCREEN_WIDTH, halfH);
        DrawRails(&game.rail, &g2, game.cam2, SCREEN_WIDTH, halfH);
        DrawObstacles(&g2, game.cam2, SCREEN_WIDTH, halfH);
        DrawBombs(&g2, game.cam2, SCREEN_WIDTH, halfH);
        DrawExplosions(&g2, game.cam2, SCREEN_WIDTH, halfH);
        DrawTrain(&game.train1, &g2, game.cam2, SCREEN_WIDTH, halfH);
        DrawTrain(&game.train2, &g2, game.cam2, SCREEN_WIDTH, halfH);
        DrawPlacementPreview(&g2, &game.p2, game.selectedPlacement2, game.selectedDir2, Color(120, 0, 255, 255));
        if (!game.bucket2.pickedUp &&
            IsWorldRectVisible(game.bucket2.pos.x, game.bucket2.pos.y, BUCKET_SIZE, BUCKET_SIZE, game.cam2, SCREEN_WIDTH, halfH))
            g2.DrawImage(g_emptyBucket, game.bucket2.pos.x, game.bucket2.pos.y, BUCKET_SIZE, BUCKET_SIZE);
        if (game.p2.hasBucket &&
            IsWorldRectVisible(game.p2.pos.x, game.p2.pos.y, game.p2.size, game.p2.size, game.cam2, SCREEN_WIDTH, halfH)) {
            Bitmap* img = game.p2.bucketFull ? g_fullBucket : g_emptyBucket;
            g2.DrawImage(img, game.p2.pos.x + 15.0f, game.p2.pos.y + 8.0f, HELD_BUCKET_SIZE, HELD_BUCKET_SIZE);
        }
        DrawBases(&g2, &game.p2);
        DrawRailCraftStations(&g2, &game.p2);
        DrawObstacleCraftStations(&g2, &game.p2);
        DrawBombCraftStations(&g2, &game.p2);

        g2.Flush();
        SaveDC(game.memDC);
        IntersectClipRect(game.memDC, 0, halfH, SCREEN_WIDTH, SCREEN_HEIGHT);
        DrawPlayer(&game.p1, game.memDC, game.cam2, halfH);
        DrawPlayer(&game.p2, game.memDC, game.cam2, halfH);
        DrawInventory(&game.p1, game.memDC, game.cam2, halfH);
        DrawInventory(&game.p2, game.memDC, game.cam2, halfH);
        RestoreDC(game.memDC, -1);

        Pen pen(Color(255, 0, 0, 0), 5);
        g.DrawLine(&pen, 0, halfH, SCREEN_WIDTH, halfH);

        // game over screen
        if (game.gameOver) {
            Graphics vg(game.memDC);
            DrawVictoryScreen(&vg);
        }

        // pause menu / settings overlay
        if (game.gameState == STATE_PAUSE_MENU) {
            Graphics og(game.memDC);
            DrawPauseMenu(&og);
        }
        else if (game.gameState == STATE_SETTINGS) {
            Graphics og(game.memDC);
            DrawSettingsMenu(&og);
        }
        else if (game.gameState == STATE_GAME_START) {
            Graphics og(game.memDC);
            DrawGameStartOverlay(&og);
        }

        PresentBackBuffer(hWnd, hDC);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // 배경 지우기 생략 (전체화면 전환 시 깜빡임 방지)

    case WM_KEYDOWN:
        if (wParam == VK_F11) {
            ToggleFullscreen(hWnd);
            return 0;
        }
        if (wParam == VK_F3) {
            if (!(lParam & 0x40000000)) { // 키 반복(꾹 누름) 무시
                game.infiniteResourceMode = !game.infiniteResourceMode;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            if (game.gameState == STATE_PLAYING) {
                game.gameState = STATE_PAUSE_MENU;
            }
            else if (game.gameState == STATE_PAUSE_MENU) {
                game.gameState = STATE_PLAYING;
            }
            else if (game.gameState == STATE_SETTINGS) {
                game.gameState = STATE_PAUSE_MENU;
            }
            else if (game.gameState == STATE_CONTROLS) {
                game.gameState = STATE_START;
            }
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;

    case WM_LBUTTONDOWN:
    {
        int mx = LOWORD(lParam);
        int my = HIWORD(lParam);
        MapMouseToLogical(&mx, &my); // 화면 좌표 → 논리 좌표

        if (game.gameState == STATE_START) {
            // 게임시작 버튼
            if (mx >= START_BTN_LEFT && mx <= START_BTN_RIGHT &&
                my >= START_BTN_TOP  && my <= START_BTN_BOTTOM) {
                PlaySound(NULL, NULL, 0);
                PlaySound(L"Sound\\reacionsound.wav", NULL, SND_FILENAME | SND_ASYNC);
                game.gameStartCountdown = 5.5f;   // 0.5s 클릭음 + 5.0s 기차음
                game.trainSoundStarted = false;
                game.lastUpdateTime = GetTickCount64();
                game.gameState = STATE_GAME_START;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            // 우상단 "조작법" 버튼
            else if (mx >= CTRL_BTN_LEFT && mx <= CTRL_BTN_RIGHT &&
                     my >= CTRL_BTN_TOP  && my <= CTRL_BTN_BOTTOM) {
                game.gameState = STATE_CONTROLS;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            // 볼륨 슬라이더 (sx=1020, sy=672, sw=200, sh=10)
            else {
                const int sx = 1020, sy = 672, sw = 200;
                if (mx >= sx && mx <= sx + sw && my >= sy - 10 && my <= sy + 20) {
                    game.volDragging = true;
                    SetCapture(hWnd);
                    int vol = (mx - sx) * 100 / sw;
                    if (vol < 0) vol = 0;
                    if (vol > 100) vol = 100;
                    game.volume = vol;
                    ApplyVolume(vol);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
        }
        else if (game.gameState == STATE_CONTROLS) {
            // "뒤로" 버튼
            if (mx >= CTRL_BACK_LEFT && mx <= CTRL_BACK_RIGHT &&
                my >= CTRL_BACK_TOP  && my <= CTRL_BACK_BOTTOM) {
                game.gameState = STATE_START;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        else if (game.gameState == STATE_PAUSE_MENU) {
            // bx=470, by=190
            // 계속하기: 490, 255, 300, 50
            if (mx >= 490 && mx <= 790 && my >= 255 && my <= 305) {
                game.gameState = STATE_PLAYING;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            // 설정: 490, 320, 300, 50
            else if (mx >= 490 && mx <= 790 && my >= 320 && my <= 370) {
                game.gameState = STATE_SETTINGS;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            // 게임 종료: 490, 385, 300, 50
            else if (mx >= 490 && mx <= 790 && my >= 385 && my <= 435) {
                PostQuitMessage(0);
            }
        }
        else if (game.gameState == STATE_SETTINGS) {
            // bx=440, by=230
            // volume slider track: sx=520, sy=305, sw=290, sh=10
            const int sx = 520, sy = 305, sw = 290;
            if (mx >= sx && mx <= sx + sw && my >= sy - 10 && my <= sy + 20) {
                game.volDragging = true;
                SetCapture(hWnd);
                int vol = (mx - sx) * 100 / sw;
                if (vol < 0) vol = 0;
                if (vol > 100) vol = 100;
                game.volume = vol;
                ApplyVolume(vol);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            // 뒤로 button: bx+20=460, by+160=390, w=360, h=50
            else if (mx >= 460 && mx <= 820 && my >= 390 && my <= 440) {
                game.gameState = STATE_PAUSE_MENU;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        int mmx = (int)(SHORT)LOWORD(lParam);
        int mmy = (int)(SHORT)HIWORD(lParam);
        MapMouseToLogical(&mmx, &mmy); // 화면 좌표 → 논리 좌표
        g_mousePos = { (LONG)mmx, (LONG)mmy };
        if (game.volDragging) {
            // 슬라이더 트랙 위치: 시작화면 sx=1020 sw=200 / 설정화면 sx=520 sw=290
            int sx = (game.gameState == STATE_START) ? 1020 : 520;
            int sw = (game.gameState == STATE_START) ? 200  : 290;
            int vol = (mmx - sx) * 100 / sw;
            if (vol < 0) vol = 0;
            if (vol > 100) vol = 100;
            game.volume = vol;
            ApplyVolume(vol);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONUP:
    {
        if (game.volDragging) {
            game.volDragging = false;
            ReleaseCapture();
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
        delete g_bombImage;
        g_bombImage = nullptr;
        delete g_emergencyImage;
        g_emergencyImage = nullptr;
        delete g_helperImage;
        g_helperImage = nullptr;
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

    map->image = new Bitmap(L"Image\\Map\\newUnrailedmap.png");
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
        base.y = (MAP_HEIGHT - BASE_HEIGHT_TILES - 7) * TILE_SIZE;
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

bool IsWorldRectVisible(float x, float y, float w, float h, Camera cam, int viewW, int viewH, float padding) {
    return x + w >= cam.x - padding &&
           x <= cam.x + viewW + padding &&
           y + h >= cam.y - padding &&
           y <= cam.y + viewH + padding;
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

bool DoesTileOverlapAnyPlayer(int tileX, int tileY) {
    RECT tileRect = {
        tileX * TILE_SIZE,
        tileY * TILE_SIZE,
        tileX * TILE_SIZE + TILE_SIZE,
        tileY * TILE_SIZE + TILE_SIZE
    };

    return RectsOverlap(tileRect, GetPlayerRect(&game.p1)) ||
           RectsOverlap(tileRect, GetPlayerRect(&game.p2));
}

RECT GetTrainRect(Train* train) {
    return {
        (LONG)train->pos.x,
        (LONG)train->pos.y,
        (LONG)(train->pos.x + TRAIN_WIDTH),
        (LONG)(train->pos.y + TRAIN_HEIGHT)
    };
}

RECT GetRailCraftStationRect(BaseArea base) {
    int stationX = base.doorOnRight ? base.x + 2 * TILE_SIZE : base.x + base.w - 3 * TILE_SIZE;
    int stationY = base.y + 3 * TILE_SIZE;
    return { stationX, stationY, stationX + TILE_SIZE, stationY + TILE_SIZE };
}

RECT GetObstacleCraftStationRect(BaseArea base) {
    int stationX = base.doorOnRight ? base.x + 4 * TILE_SIZE : base.x + base.w - 5 * TILE_SIZE;
    int stationY = base.y + 3 * TILE_SIZE;
    return { stationX, stationY, stationX + TILE_SIZE, stationY + TILE_SIZE };
}

RECT GetBombCraftStationRect(BaseArea base) {
    int stationX = base.doorOnRight ? base.x + 6 * TILE_SIZE : base.x + base.w - 7 * TILE_SIZE;
    int stationY = base.y + 3 * TILE_SIZE;
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

bool IsPlayerTouchingBombCraftStation(Player* p) {
    RECT playerRect = GetPlayerRect(p);

    for (int i = 0; i < 2; i++) {
        RECT stationRect = GetBombCraftStationRect(GetBaseArea(i));
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

void UpdateBombCraft(Player* p, float deltaTime) {
    if (!IsPlayerTouchingBombCraftStation(p) ||
        (!game.infiniteResourceMode &&
            (p->wood < BOMB_WOOD_COST ||
             p->stone < BOMB_STONE_COST))) {
        p->bombCraftProgress = 0.0f;
        return;
    }

    p->bombCraftProgress += deltaTime;

    if (p->bombCraftProgress >= BOMB_CRAFT_TIME) {
        if (!game.infiniteResourceMode) {
            p->wood -= BOMB_WOOD_COST;
            p->stone -= BOMB_STONE_COST;
        }
        p->bombCount++;
        p->bombCraftProgress = 0.0f;
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

void DrawBombCraftStations(Graphics* g, Player* viewer) {
    for (int i = 0; i < 2; i++) {
        BaseArea base = GetBaseArea(i);
        if (!IsPlayerInsideBase(viewer, base)) continue;

        RECT rc = GetBombCraftStationRect(base);
        SolidBrush tableBrush(Color(255, 76, 62, 58));
        Pen tableEdge(Color(255, 38, 28, 26), 2.0f);

        g->FillRectangle(&tableBrush, rc.left, rc.top, TILE_SIZE, TILE_SIZE);
        g->DrawRectangle(&tableEdge, rc.left, rc.top, TILE_SIZE, TILE_SIZE);
        DrawBombIcon(g, (float)rc.left + 5.0f, (float)rc.top + 5.0f, 22.0f, 255);

        if (RectsOverlap(GetPlayerRect(viewer), rc) && viewer->bombCraftProgress > 0.0f) {
            float ratio = viewer->bombCraftProgress / BOMB_CRAFT_TIME;
            if (ratio > 1.0f) ratio = 1.0f;

            SolidBrush bgBrush(Color(255, 0, 0, 0));
            SolidBrush fillBrush(Color(255, 255, 80, 45));
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
    p->size = PLAYER_SIZE;
    p->dir = DIR_DOWN;
    p->isMoving = false;
    p->frame = 0;
    p->wood = 0;
    p->stone = 0;
    p->railCount = 0;
    p->obstacleCount = 0;
    p->bombCount = 0;
    p->railCraftProgress = 0.0f;
    p->obstacleCraftProgress = 0.0f;
    p->bombCraftProgress = 0.0f;
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
    if (!IsWorldRectVisible(p->pos.x, p->pos.y, p->size, p->size, cam, SCREEN_WIDTH, SCREEN_HEIGHT / 2, 20.0f)) return;

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
    if (!IsWorldRectVisible(p->pos.x, p->pos.y, p->size + 260.0f, p->size + 24.0f, cam, SCREEN_WIDTH, SCREEN_HEIGHT / 2, 20.0f)) return;

    int x = (int)(p->pos.x - cam.x);
    int y = (int)(p->pos.y - cam.y) + offsetY;

    wchar_t text[128];
    if (game.infiniteResourceMode) {
        swprintf_s(text, L"나무:∞ / 돌:∞ / 레일:∞ / 장애물:∞ / 폭탄:∞");
    }
    else if (game.infiniteRailMode) {
        swprintf_s(text, L"나무:%d / 돌:%d / 레일:∞ / 장애물:%d / 폭탄:%d", p->wood, p->stone, p->obstacleCount, p->bombCount);
    }
    else if (p->railCount > 0 || p->obstacleCount > 0 || p->bombCount > 0) {
        swprintf_s(text, L"나무:%d / 돌:%d / 레일:%d / 장애물:%d / 폭탄:%d", p->wood, p->stone, p->railCount, p->obstacleCount, p->bombCount);
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

// 원본 이미지에 색 틴트를 입힌 새 비트맵을 생성 (로드 시 1회만 호출)
// 원본 이미지를 회전(rotDeg) + 색 틴트해서 TILE_SIZE x TILE_SIZE 비트맵으로 구움 (로드 시 1회)
static Bitmap* CreateRailVariant(Bitmap* src, float r, float g, float b, int rotDeg) {
    Bitmap* dst = new Bitmap(TILE_SIZE, TILE_SIZE, PixelFormat32bppARGB);
    Graphics gr(dst);
    gr.Clear(Color(0, 0, 0, 0));
    gr.SetInterpolationMode(InterpolationModeHighQualityBicubic); // 1회뿐이라 고품질 OK
    gr.TranslateTransform(TILE_SIZE / 2.0f, TILE_SIZE / 2.0f);
    gr.RotateTransform((REAL)rotDeg);
    ColorMatrix cm = {
        r, 0, 0, 0, 0,
        0, g, 0, 0, 0,
        0, 0, b, 0, 0,
        0, 0, 0, 1, 0,
        0, 0, 0, 0, 1 };
    ImageAttributes ia;
    ia.SetColorMatrix(&cm);
    gr.DrawImage(src, RectF(-TILE_SIZE / 2.0f, -TILE_SIZE / 2.0f, (REAL)TILE_SIZE, (REAL)TILE_SIZE),
        0, 0, (REAL)src->GetWidth(), (REAL)src->GetHeight(), UnitPixel, &ia);
    return dst;
}

void InitRail(Rail* rail) {
    rail->railImage = new Bitmap(L"Image\\train\\rail.png");
    rail->turnImage = new Bitmap(L"Image\\train\\turn rail.png");

    // 소유자별 색 (0=중립, 1=P1 붉은빛, 2=P2 푸른빛)
    float tints[3][3] = {
        { 1.0f, 1.0f,  1.0f },
        { 1.0f, 0.55f, 0.5f },
        { 0.5f, 0.7f,  1.0f },
    };
    // 방향별 회전각은 아래에서 직접 지정 (기존 DrawOneRailImage 회전 규칙과 동일)
    for (int o = 0; o < 3; o++) {
        float r = tints[o][0], g = tints[o][1], b = tints[o][2];
        rail->variant[o][RAIL_HORIZONTAL] = CreateRailVariant(rail->railImage, r, g, b, 0);
        rail->variant[o][RAIL_VERTICAL]   = CreateRailVariant(rail->railImage, r, g, b, 90);
        rail->variant[o][RAIL_TURN_RD]    = CreateRailVariant(rail->turnImage, r, g, b, 0);
        rail->variant[o][RAIL_TURN_LD]    = CreateRailVariant(rail->turnImage, r, g, b, 90);
        rail->variant[o][RAIL_TURN_LU]    = CreateRailVariant(rail->turnImage, r, g, b, 180);
        rail->variant[o][RAIL_TURN_RU]    = CreateRailVariant(rail->turnImage, r, g, b, 270);
    }

    memset(rail->grid, -1, sizeof(rail->grid));
    memset(rail->ownerGrid, 0, sizeof(rail->ownerGrid));
}

void ReleaseRail(Rail* rail) {
    delete rail->railImage;
    delete rail->turnImage;
    for (int o = 0; o < 3; o++)
        for (int d = 0; d < 6; d++)
            delete rail->variant[o][d];
    rail->rails.clear();
}

bool HasHorizontal(Rail* rail, int tileX, int tileY) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) return false;
    return rail->grid[tileY][tileX] == (int8_t)RAIL_HORIZONTAL;
}

bool HasVertical(Rail* rail, int tileX, int tileY) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) return false;
    return rail->grid[tileY][tileX] == (int8_t)RAIL_VERTICAL;
}

bool HasRail(Rail* rail, int tileX, int tileY) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) return false;
    return rail->grid[tileY][tileX] >= 0;
}

// 특정 소유자(owner)의 레일만 인식하는 버전 — 플레이어/기차별 레일 분리에 사용
bool HasRailOwner(Rail* rail, int tileX, int tileY, int owner) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) return false;
    return rail->grid[tileY][tileX] >= 0 && rail->ownerGrid[tileY][tileX] == (int8_t)owner;
}

bool HasHorizontalOwner(Rail* rail, int tileX, int tileY, int owner) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) return false;
    return rail->grid[tileY][tileX] == (int8_t)RAIL_HORIZONTAL && rail->ownerGrid[tileY][tileX] == (int8_t)owner;
}

bool HasVerticalOwner(Rail* rail, int tileX, int tileY, int owner) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) return false;
    return rail->grid[tileY][tileX] == (int8_t)RAIL_VERTICAL && rail->ownerGrid[tileY][tileX] == (int8_t)owner;
}

bool HasObstacle(int tileX, int tileY) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) return false;
    return game.obstacleGrid[tileY][tileX];
}

bool HasBomb(int tileX, int tileY) {
    for (int i = 0; i < (int)game.bombs.size(); i++) {
        if (game.bombs[i].tileX == tileX && game.bombs[i].tileY == tileY) return true;
    }
    return false;
}

bool CanPlaceRailAt(Rail* rail, int tileX, int tileY, RailDir, int) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) return false;
    if (HasRail(rail, tileX, tileY) || HasObstacle(tileX, tileY)) return false;

    return true;
}

bool CanPlaceObstacleAt(int tileX, int tileY) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) return false;
    if (HasRail(&game.rail, tileX, tileY) || HasObstacle(tileX, tileY)) return false;
    if (DoesTileOverlapAnyPlayer(tileX, tileY)) return false;

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

bool CanPlaceBombAt(int tileX, int tileY) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) return false;

    RECT tileRect = {
        tileX * TILE_SIZE,
        tileY * TILE_SIZE,
        tileX * TILE_SIZE + TILE_SIZE,
        tileY * TILE_SIZE + TILE_SIZE
    };

    bool overlapsTrain = (RectsOverlap(tileRect, GetTrainRect(&game.train1)) && game.train1.bombCargo == 0) ||
                         (RectsOverlap(tileRect, GetTrainRect(&game.train2)) && game.train2.bombCargo == 0);

    return (HasObstacle(tileX, tileY) && !HasBomb(tileX, tileY)) || overlapsTrain;
}

bool PlaceObstacle(int tileX, int tileY, int owner) {
    if (!CanPlaceObstacleAt(tileX, tileY)) return false;

    Obstacle obstacle;
    obstacle.tileX = tileX;
    obstacle.tileY = tileY;
    obstacle.owner = owner;
    game.obstacles.push_back(obstacle);
    game.obstacleGrid[tileY][tileX] = true;
    return true;
}

bool PlaceBomb(int tileX, int tileY, int owner) {
    if (!CanPlaceBombAt(tileX, tileY)) return false;

    RECT tileRect = {
        tileX * TILE_SIZE,
        tileY * TILE_SIZE,
        tileX * TILE_SIZE + TILE_SIZE,
        tileY * TILE_SIZE + TILE_SIZE
    };

    if (RectsOverlap(tileRect, GetTrainRect(&game.train1)) && game.train1.bombCargo == 0) {
        game.train1.bombCargo = 1;
        return true;
    }
    if (RectsOverlap(tileRect, GetTrainRect(&game.train2)) && game.train2.bombCargo == 0) {
        game.train2.bombCargo = 1;
        return true;
    }

    Bomb bomb;
    bomb.tileX = tileX;
    bomb.tileY = tileY;
    bomb.owner = owner;
    bomb.timer = BOMB_EXPLODE_TIME;
    game.bombs.push_back(bomb);
    return true;
}

void AddExplosion(float x, float y) {
    ExplosionEffect explosion;
    explosion.x = x;
    explosion.y = y;
    explosion.timer = EXPLOSION_EFFECT_TIME;
    game.explosions.push_back(explosion);
}

void AddBaseExplosion(BaseArea base) {
    float centerX = (float)(base.x + base.w / 2);
    float centerY = (float)(base.y + base.h / 2);
    AddExplosion(centerX, centerY);
    AddExplosion(centerX - TILE_SIZE * 1.5f, centerY - TILE_SIZE * 1.0f);
    AddExplosion(centerX + TILE_SIZE * 1.5f, centerY - TILE_SIZE * 0.8f);
    AddExplosion(centerX - TILE_SIZE * 1.0f, centerY + TILE_SIZE * 1.2f);
    AddExplosion(centerX + TILE_SIZE * 1.0f, centerY + TILE_SIZE * 1.1f);
}

void UpdateBombs(float deltaTime) {
    for (int i = (int)game.bombs.size() - 1; i >= 0; i--) {
        game.bombs[i].timer -= deltaTime;
        if (game.bombs[i].timer > 0.0f) continue;

        int tileX = game.bombs[i].tileX;
        int tileY = game.bombs[i].tileY;
        AddExplosion((float)(tileX * TILE_SIZE + TILE_SIZE / 2), (float)(tileY * TILE_SIZE + TILE_SIZE / 2));

        for (int j = (int)game.obstacles.size() - 1; j >= 0; j--) {
            if (game.obstacles[j].tileX == tileX && game.obstacles[j].tileY == tileY) {
                game.obstacleGrid[tileY][tileX] = false;
                game.obstacles.erase(game.obstacles.begin() + j);
                break;
            }
        }
        game.bombs.erase(game.bombs.begin() + i);
    }
}

void UpdateExplosions(float deltaTime) {
    for (int i = (int)game.explosions.size() - 1; i >= 0; i--) {
        game.explosions[i].timer -= deltaTime;
        if (game.explosions[i].timer <= 0.0f) {
            game.explosions.erase(game.explosions.begin() + i);
        }
    }
}

bool HandleBombTrainCollision() {
    if (game.gameOver) return false;
    if (!RectsOverlap(GetTrainRect(&game.train1), GetTrainRect(&game.train2))) return false;

    int winner = 0;
    if (game.train1.bombCargo > 0 && game.train2.bombCargo > 0) {
        winner = 0;
    }
    else if (game.train1.bombCargo > 0) {
        winner = 1;
    }
    else if (game.train2.bombCargo > 0) {
        winner = 2;
    }
    else {
        return false;
    }

    float centerX = (game.train1.pos.x + game.train2.pos.x + TRAIN_WIDTH) / 2.0f;
    float centerY = (game.train1.pos.y + game.train2.pos.y + TRAIN_HEIGHT) / 2.0f;
    AddExplosion(centerX, centerY);
    AddExplosion(centerX - TRAIN_WIDTH * 0.35f, centerY - TRAIN_HEIGHT * 0.45f);
    AddExplosion(centerX + TRAIN_WIDTH * 0.35f, centerY + TRAIN_HEIGHT * 0.45f);

    game.train1.finished = true;
    game.train2.finished = true;
    game.train1.bombCargo = 0;
    game.train2.bombCargo = 0;
    game.gameOver = true;
    game.winner = winner;
    game.baseExplosionActive = true;
    game.baseExplosionStartTime = GetTickCount64();
    return true;
}

RailDir GetRailDir(Rail* rail, int tileX, int tileY) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) return RAIL_HORIZONTAL;
    int8_t v = rail->grid[tileY][tileX];
    return (v >= 0) ? (RailDir)v : RAIL_HORIZONTAL;
}

RailDir AutoDetectRailDir(Rail* rail, int tileX, int tileY, RailDir baseDir, int owner) {
    bool left = HasRailOwner(rail, tileX - 1, tileY, owner);
    bool right = HasRailOwner(rail, tileX + 1, tileY, owner);
    bool up = HasRailOwner(rail, tileX, tileY - 1, owner);
    bool down = HasRailOwner(rail, tileX, tileY + 1, owner);

    bool horiz = left || right;
    bool vert = up || down;

    // 이미 양옆(또는 위아래)이 모두 연결된 '관통' 레일은 직선 유지 — 턴으로 바꾸지 않음
    // (직선 위에 수직 레일을 놔도 선이 꺾여 망가지지 않도록)
    if (left && right) return RAIL_HORIZONTAL;
    if (up && down)    return RAIL_VERTICAL;

    // 가로 한쪽 + 세로 한쪽이면 코너(턴) — 연결되는 두 방향으로 정확히 판별
    if (horiz && vert) {
        if (right && down) return RAIL_TURN_RD;
        if (left && down)  return RAIL_TURN_LD;
        if (right && up)   return RAIL_TURN_RU;
        if (left && up)    return RAIL_TURN_LU;
    }

    // 직선: 이웃 방향에 맞춰 결정
    if (vert && !horiz) return RAIL_VERTICAL;
    if (horiz && !vert) return RAIL_HORIZONTAL;

    // 이웃이 없으면 플레이어가 고른 방향 유지 (턴 값이면 가로로 환원)
    if (baseDir == RAIL_VERTICAL) return RAIL_VERTICAL;
    return RAIL_HORIZONTAL;
}

void UpdateRailNeighbors(Rail* rail, int tileX, int tileY) {
    int dx[] = { -1, 1, 0, 0 };
    int dy[] = { 0, 0, -1, 1 };

    // 그리드로 O(1) 조회 (전체 레일 벡터 스캔 제거)
    for (int i = 0; i < 4; i++) {
        int nx = tileX + dx[i];
        int ny = tileY + dy[i];
        if (nx < 0 || ny < 0 || nx >= MAP_WIDTH || ny >= MAP_HEIGHT) continue;
        if (rail->grid[ny][nx] < 0) continue; // 레일 없음
        int o = rail->ownerGrid[ny][nx];
        RailDir nd = AutoDetectRailDir(rail, nx, ny, (RailDir)rail->grid[ny][nx], o);
        rail->grid[ny][nx] = (int8_t)nd;
    }
}

bool PlaceRail(Rail* rail, int tileX, int tileY, RailDir dir, int owner) {
    if (!CanPlaceRailAt(rail, tileX, tileY, dir, owner)) return false;

    RailData data;
    data.tileX = tileX;
    data.tileY = tileY;
    data.dir = AutoDetectRailDir(rail, tileX, tileY, dir, owner);
    data.owner = owner;
    rail->rails.push_back(data);
    rail->grid[tileY][tileX] = (int8_t)data.dir;
    rail->ownerGrid[tileY][tileX] = (int8_t)owner;
    UpdateRailNeighbors(rail, tileX, tileY);
    return true;
}

PlacementType GetNextPlacementType(Player* player, PlacementType placementType) {
    PlacementType options[4];
    int count = 0;

    if (game.infiniteRailMode || game.infiniteResourceMode || player->railCount > 0) {
        options[count++] = PLACEMENT_RAIL;
    }
    if (game.infiniteResourceMode || player->obstacleCount > 0) {
        options[count++] = PLACEMENT_OBSTACLE;
    }
    if (game.infiniteResourceMode || player->bombCount > 0) {
        options[count++] = PLACEMENT_BOMB;
    }
    // 선택지가 2개 이상일 때만 NONE 추가 (1개면 사이클해도 계속 같은 타입 유지)
    if (count != 1) options[count++] = PLACEMENT_NONE;

    for (int i = 0; i < count; i++) {
        if (options[i] == placementType) {
            return options[(i + 1) % count];
        }
    }

    return options[0];
}

bool PlaceSelectedItem(Player* player, PlacementType placementType, RailDir railDir, int owner) {
    if (placementType == PLACEMENT_NONE) return false;

    int tileX, tileY;
    GetPlacementTile(player, &tileX, &tileY);

    if (placementType == PLACEMENT_RAIL) {
        if (!(game.infiniteRailMode || game.infiniteResourceMode || player->railCount > 0)) return false;
        if (!PlaceRail(&game.rail, tileX, tileY, railDir, owner)) return false;
        if (!game.infiniteRailMode && !game.infiniteResourceMode) player->railCount--;
        return true;
    }

    if (placementType == PLACEMENT_BOMB) {
        if (!(game.infiniteResourceMode || player->bombCount > 0)) return false;
        if (!PlaceBomb(tileX, tileY, owner)) return false;
        if (!game.infiniteResourceMode) player->bombCount--;
        return true;
    }

    if (!(game.infiniteResourceMode || player->obstacleCount > 0)) return false;
    if (!PlaceObstacle(tileX, tileY, owner)) return false;
    if (!game.infiniteResourceMode) player->obstacleCount--;
    return true;
}

void DrawOneRailImage(Rail* rail, Graphics* g, float x, float y, RailDir dir, bool preview, int owner) {
    int o = (owner == 1 || owner == 2) ? owner : 0;
    int d = (dir >= 0 && dir < 6) ? (int)dir : 0;
    Bitmap* img = rail->variant[o][d]; // 회전·틴트까지 이미 구워진 이미지

    if (preview) {
        // 미리보기만 반투명 (프레임당 1~2개라 비용 무시 가능)
        ColorMatrix cm = { 1,0,0,0,0, 0,1,0,0,0, 0,0,1,0,0, 0,0,0,0.5f,0, 0,0,0,0,1 };
        ImageAttributes ia;
        ia.SetColorMatrix(&cm);
        g->DrawImage(img, RectF(x, y, (REAL)TILE_SIZE, (REAL)TILE_SIZE),
            0, 0, (REAL)TILE_SIZE, (REAL)TILE_SIZE, UnitPixel, &ia);
    }
    else {
        // 설치된 레일: Save/Restore·회전 없이 단순 1장 그리기 (가장 빠름)
        g->DrawImage(img, (INT)x, (INT)y);
    }
}

void DrawRailPreview(Rail* rail, Graphics* g, int x, int y, RailDir dir, int owner) {
    DrawOneRailImage(rail, g, (float)x, (float)y, dir, true, owner);
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

void DrawBombIcon(Graphics* g, float x, float y, float size, BYTE alpha) {
    if (g_bombImage && g_bombImage->GetLastStatus() == Ok) {
        ColorMatrix cm = { 1,0,0,0,0, 0,1,0,0,0, 0,0,1,0,0, 0,0,0,alpha / 255.0f,0, 0,0,0,0,1 };
        ImageAttributes ia;
        ia.SetColorMatrix(&cm);
        g->DrawImage(g_bombImage, RectF(x, y, size, size),
            0, 0, (float)g_bombImage->GetWidth(), (float)g_bombImage->GetHeight(), UnitPixel, &ia);
        return;
    }

    SolidBrush body(Color(alpha, 28, 28, 32));
    SolidBrush shine(Color(alpha, 80, 80, 88));
    Pen fuse(Color(alpha, 240, 180, 70), 2.0f);
    g->FillEllipse(&body, x + size * 0.15f, y + size * 0.25f, size * 0.68f, size * 0.68f);
    g->FillEllipse(&shine, x + size * 0.32f, y + size * 0.38f, size * 0.16f, size * 0.16f);
    g->DrawLine(&fuse, x + size * 0.62f, y + size * 0.25f, x + size * 0.84f, y + size * 0.08f);
}

void DrawBombPreview(Graphics* g, int x, int y) {
    DrawBombIcon(g, (float)x + 4.0f, (float)y + 4.0f, (float)TILE_SIZE - 8.0f, 150);
}

void DrawPlacementPreview(Graphics* g, Player* player, PlacementType placementType, RailDir railDir, Color tileColor) {
    if (placementType == PLACEMENT_NONE) return;

    int tileX, tileY;
    GetPlacementTile(player, &tileX, &tileY);
    int preX = tileX * TILE_SIZE;
    int preY = tileY * TILE_SIZE;

    bool hasRailItem = game.infiniteRailMode || game.infiniteResourceMode || player->railCount > 0;
    bool hasObstacleItem = game.infiniteResourceMode || player->obstacleCount > 0;
    bool hasBombItem = game.infiniteResourceMode || player->bombCount > 0;
    bool canPlaceRail = hasRailItem && CanPlaceRailAt(&game.rail, tileX, tileY, railDir, player->id);
    bool canPlaceObstacle = hasObstacleItem && CanPlaceObstacleAt(tileX, tileY);
    bool canPlaceBomb = hasBombItem && CanPlaceBombAt(tileX, tileY);
    bool canPlace = false;
    if (placementType == PLACEMENT_RAIL) canPlace = canPlaceRail;
    else if (placementType == PLACEMENT_OBSTACLE) canPlace = canPlaceObstacle;
    else if (placementType == PLACEMENT_BOMB) canPlace = canPlaceBomb;

    Color previewColor = tileColor;
    if (placementType == PLACEMENT_OBSTACLE) previewColor = Color(120, 255, 130, 70);
    if (placementType == PLACEMENT_BOMB) previewColor = Color(120, 255, 80, 45);
    if (!canPlace) previewColor = Color(90, 120, 120, 120);
    SolidBrush previewBrush(previewColor);
    g->FillRectangle(&previewBrush, (float)preX, (float)preY, (float)TILE_SIZE, (float)TILE_SIZE);

    if (placementType == PLACEMENT_RAIL) DrawRailPreview(&game.rail, g, preX, preY, railDir, player->id);
    else if (placementType == PLACEMENT_OBSTACLE) DrawObstaclePreview(g, preX, preY);
    else DrawBombPreview(g, preX, preY);
}

void DrawRails(Rail* rail, Graphics* g, Camera cam, int viewW, int viewH) {
    const float left = cam.x - TILE_SIZE;
    const float top = cam.y - TILE_SIZE;
    const float right = cam.x + viewW + TILE_SIZE;
    const float bottom = cam.y + viewH + TILE_SIZE;

    for (int i = 0; i < (int)rail->rails.size(); i++) {
        int tx = rail->rails[i].tileX;
        int ty = rail->rails[i].tileY;
        float x = (float)(tx * TILE_SIZE);
        float y = (float)(ty * TILE_SIZE);

        if (x > right || x + TILE_SIZE < left || y > bottom || y + TILE_SIZE < top) continue;
        // 방향은 그리드에서 읽음 (이웃 갱신으로 최신 상태 유지)
        RailDir dir = (RailDir)rail->grid[ty][tx];
        DrawOneRailImage(rail, g, x, y, dir, false, rail->rails[i].owner);
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

void DrawBombs(Graphics* g, Camera cam, int viewW, int viewH) {
    const float left = cam.x - TILE_SIZE;
    const float top = cam.y - TILE_SIZE;
    const float right = cam.x + viewW + TILE_SIZE;
    const float bottom = cam.y + viewH + TILE_SIZE;

    SolidBrush bgBrush(Color(255, 0, 0, 0));
    SolidBrush fillBrush(Color(255, 255, 60, 35));

    for (int i = 0; i < (int)game.bombs.size(); i++) {
        float x = (float)(game.bombs[i].tileX * TILE_SIZE);
        float y = (float)(game.bombs[i].tileY * TILE_SIZE);

        if (x > right || x + TILE_SIZE < left || y > bottom || y + TILE_SIZE < top) continue;

        DrawBombIcon(g, x + 4.0f, y + 4.0f, (float)TILE_SIZE - 8.0f, 255);

        float ratio = game.bombs[i].timer / BOMB_EXPLODE_TIME;
        if (ratio < 0.0f) ratio = 0.0f;
        g->FillRectangle(&bgBrush, x, y - 7.0f, (float)TILE_SIZE, 4.0f);
        g->FillRectangle(&fillBrush, x, y - 7.0f, (float)TILE_SIZE * ratio, 4.0f);
    }
}

void DrawExplosions(Graphics* g, Camera cam, int viewW, int viewH) {
    const float padding = TILE_SIZE * 2.0f;
    const float left = cam.x - padding;
    const float top = cam.y - padding;
    const float right = cam.x + viewW + padding;
    const float bottom = cam.y + viewH + padding;

    if (game.explosions.empty()) return;

    SolidBrush outerBrush(Color(0, 0, 0, 0));
    SolidBrush midBrush(Color(0, 0, 0, 0));
    SolidBrush smokeBrush(Color(0, 0, 0, 0));
    Pen ringPen(Color(0, 0, 0, 0), 2.0f);

    for (int i = 0; i < (int)game.explosions.size(); i++) {
        ExplosionEffect effect = game.explosions[i];
        if (effect.x < left || effect.x > right || effect.y < top || effect.y > bottom) continue;

        float age = 1.0f - (effect.timer / EXPLOSION_EFFECT_TIME);
        if (age < 0.0f) age = 0.0f;
        if (age > 1.0f) age = 1.0f;

        BYTE alpha = (BYTE)(220 * (1.0f - age));
        float outerRadius = 10.0f + 34.0f * age;
        float innerRadius = 6.0f + 14.0f * age;

        outerBrush.SetColor(Color(alpha, 255, 85, 30));
        midBrush.SetColor(Color((BYTE)(alpha * 0.85f), 255, 180, 45));
        smokeBrush.SetColor(Color((BYTE)(alpha * 0.45f), 60, 52, 48));
        ringPen.SetColor(Color(alpha, 255, 230, 90));

        g->FillEllipse(&outerBrush, effect.x - outerRadius, effect.y - outerRadius,
            outerRadius * 2.0f, outerRadius * 2.0f);
        g->FillEllipse(&midBrush, effect.x - innerRadius, effect.y - innerRadius,
            innerRadius * 2.0f, innerRadius * 2.0f);
        g->DrawEllipse(&ringPen, effect.x - outerRadius - 4.0f, effect.y - outerRadius - 4.0f,
            (outerRadius + 4.0f) * 2.0f, (outerRadius + 4.0f) * 2.0f);

        g->FillEllipse(&smokeBrush, effect.x - outerRadius * 0.85f, effect.y - outerRadius * 1.05f,
            outerRadius * 0.7f, outerRadius * 0.55f);
        g->FillEllipse(&smokeBrush, effect.x + outerRadius * 0.15f, effect.y - outerRadius * 0.95f,
            outerRadius * 0.8f, outerRadius * 0.6f);
    }
}

void InitTrain(Train* train, float x, float y, int direction, const wchar_t* imagePath) {
    train->pos = { x, y };
    train->speed = 5.0f;
    train->heat = 0.0f;
    train->dirX = (float)direction;
    train->dirY = 0.0f;
    train->finished = false;
    train->bombCargo = 0;
    train->lastTime = GetTickCount64();
    train->image = new Bitmap(imagePath);
    // 시작 방향 헤딩(+x=0도, -x=180도)
    train->renderAngle = (direction > 0) ? 0.0f : 180.0f;
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

// 레일 종류에 따라 방향(dx,dy)을 갱신 (UpdateTrainDirection의 로컬 버전)
static void StepRailDir(RailDir rd, float& dx, float& dy) {
    if (rd == RAIL_HORIZONTAL) dy = 0;
    else if (rd == RAIL_VERTICAL) dx = 0;
    else if (rd == RAIL_TURN_RD) {
        if (dx < 0) { dx = 0; dy = 1; }
        else if (dy < 0) { dx = 1; dy = 0; }
    }
    else if (rd == RAIL_TURN_LD) {
        if (dx > 0) { dx = 0; dy = 1; }
        else if (dy < 0) { dx = -1; dy = 0; }
    }
    else if (rd == RAIL_TURN_RU) {
        if (dx < 0) { dx = 0; dy = -1; }
        else if (dy > 0) { dx = 1; dy = 0; }
    }
    else if (rd == RAIL_TURN_LU) {
        if (dx > 0) { dx = 0; dy = -1; }
        else if (dy > 0) { dx = -1; dy = 0; }
    }
}

// 기차 진행 방향으로 커브를 따라가며 앞에 남은 레일 타일 수를 센다 (현재 타일 제외)
int CountRailsAhead(Train* train, Rail* rail, int maxCount) {
    int owner = (train == &game.train1) ? 1 : 2;
    int tx = (int)((train->pos.x + TRAIN_WIDTH / 2.0f) / TILE_SIZE);
    int ty = (int)((train->pos.y + TRAIN_HEIGHT / 2.0f) / TILE_SIZE);
    float dx = train->dirX, dy = train->dirY;
    if (HasRailOwner(rail, tx, ty, owner)) StepRailDir(GetRailDir(rail, tx, ty), dx, dy);

    int count = 0;
    while (count < maxCount) {
        int nx = tx + (dx > 0 ? 1 : (dx < 0 ? -1 : 0));
        int ny = ty + (dy > 0 ? 1 : (dy < 0 ? -1 : 0));
        if (!HasRailOwner(rail, nx, ny, owner)) break;
        tx = nx; ty = ny;
        StepRailDir(GetRailDir(rail, tx, ty), dx, dy);
        count++;
    }
    return count;
}

void UpdateTrain(Train* train, Rail* rail, float deltaTime, float speedMultiplier) {
    if (game.gameOver) return;
    if (train->finished) return;

    if (!game.infiniteResourceMode && IsTrainOverheated(train)) {
        train->heat = MAX_HEAT;
        return;
    }

    int tileX = (int)((train->pos.x + TRAIN_WIDTH / 2.0f) / TILE_SIZE);
    int tileY = (int)((train->pos.y + TRAIN_HEIGHT / 2.0f) / TILE_SIZE);
    int owner = (train == &game.train1) ? 1 : 2;
    BaseArea targetBase = GetBaseArea(owner == 1 ? 1 : 0);
    RECT trainRect = GetTrainRect(train);
    RECT targetBaseRect = { targetBase.x, targetBase.y, targetBase.x + targetBase.w, targetBase.y + targetBase.h };

    if (RectsOverlap(trainRect, targetBaseRect)) {
        train->finished = true;
        game.gameOver = true;

        if (train->bombCargo > 0) {
            AddBaseExplosion(targetBase);
            train->bombCargo = 0;
            game.winner = owner;
            game.baseExplosionActive = true;
            game.baseExplosionStartTime = GetTickCount64();
        }
        else {
            AddExplosion(train->pos.x + TRAIN_WIDTH / 2.0f, train->pos.y + TRAIN_HEIGHT / 2.0f);
            game.winner = (owner == 1) ? 2 : 1;
            game.baseExplosionActive = false;
        }
        return;
    }

    if (HasObstacle(tileX, tileY)) {
        train->finished = true;
        game.gameOver = true;
        game.winner = (owner == 1) ? 2 : 1;
        game.baseExplosionActive = false;
        AddExplosion(train->pos.x + TRAIN_WIDTH / 2.0f, train->pos.y + TRAIN_HEIGHT / 2.0f);
        return;
    }

    if (HasRailOwner(rail, tileX, tileY, owner)) {
        UpdateTrainDirection(train, GetRailDir(rail, tileX, tileY));
        train->pos.x += train->speed * speedMultiplier * train->dirX * deltaTime;
        train->pos.y += train->speed * speedMultiplier * train->dirY * deltaTime;
    }
    else {
        train->finished = true;
        game.gameOver = true;
        game.winner = (owner == 1) ? 2 : 1;
        game.baseExplosionActive = false;
        AddExplosion(train->pos.x + TRAIN_WIDTH / 2.0f, train->pos.y + TRAIN_HEIGHT / 2.0f);
        return;
    }

    // 진행 방향(헤딩)으로 회전 각도를 부드럽게 보간
    float targetAngle = atan2f(train->dirY, train->dirX) * 180.0f / 3.14159265f;
    float diff = targetAngle - train->renderAngle;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    float maxStep = TRAIN_ROT_SPEED * deltaTime;
    if (diff > maxStep) train->renderAngle += maxStep;
    else if (diff < -maxStep) train->renderAngle -= maxStep;
    else train->renderAngle = targetAngle;

    if (game.infiniteResourceMode) {
        train->heat = 0.0f;
    }
    else {
        train->heat += HEAT_RATE * deltaTime * 60.0f;
        if (train->heat > MAX_HEAT) train->heat = MAX_HEAT;
    }
}

void DrawHeatBar(Train* train, Graphics* g) {
    float ratio = train->heat / MAX_HEAT;
    SolidBrush bgBrush(Color(255, 0, 0, 0));
    g->FillRectangle(&bgBrush, train->pos.x, train->pos.y - 12.0f, TRAIN_WIDTH, 7.0f);

    BYTE r = (BYTE)(255 * ratio);
    BYTE green = (BYTE)(255 * (1.0f - ratio));
    SolidBrush heatBrush(Color(255, r, green, 0));
    g->FillRectangle(&heatBrush, train->pos.x, train->pos.y - 12.0f, TRAIN_WIDTH * ratio, 7.0f);
}

void DrawTrain(Train* train, Graphics* g, Camera cam, int viewW, int viewH) {
    const float padding = 120.0f;
    if (train->pos.x > cam.x + viewW + padding ||
        train->pos.x + TRAIN_WIDTH < cam.x - padding ||
        train->pos.y > cam.y + viewH + padding ||
        train->pos.y + TRAIN_HEIGHT < cam.y - padding) {
        return;
    }

    if (train->bombCargo > 0) {
        float backX = -train->dirX;
        float backY = -train->dirY;
        if (backX == 0.0f && backY == 0.0f) backX = -1.0f;

        float trainHalfLength = (train->dirY != 0.0f) ? TRAIN_HEIGHT / 2.0f : TRAIN_WIDTH / 2.0f;
        float cargoW = TILE_SIZE + 8.0f;
        float cargoH = TILE_SIZE + 2.0f;
        float dist = trainHalfLength + 18.0f;
        float centerX = train->pos.x + TRAIN_WIDTH / 2.0f;
        float centerY = train->pos.y + TRAIN_HEIGHT / 2.0f;
        float cargoCenterX = centerX + backX * dist;
        float cargoCenterY = centerY + backY * dist;
        float cargoX = cargoCenterX - cargoW / 2.0f;
        float cargoY = cargoCenterY - cargoH / 2.0f;
        float linkX = centerX + backX * trainHalfLength;
        float linkY = centerY + backY * trainHalfLength;

        Pen linkPen(Color(255, 55, 45, 35), 3.0f);
        Pen cartEdge(Color(255, 55, 35, 25), 2.0f);
        SolidBrush cartBrush(Color(255, 124, 82, 48));
        SolidBrush cartTop(Color(255, 166, 112, 62));

        g->DrawLine(&linkPen, linkX, linkY, cargoCenterX, cargoCenterY);
        g->FillRectangle(&cartBrush, cargoX, cargoY, cargoW, cargoH);
        g->FillRectangle(&cartTop, cargoX + 4.0f, cargoY + 4.0f, cargoW - 8.0f, 6.0f);
        g->DrawRectangle(&cartEdge, cargoX, cargoY, cargoW, cargoH);
        DrawBombIcon(g, cargoX + 6.0f, cargoY + 4.0f, 18.0f, 255);
    }

    // 진행 방향에 맞춰 기차 회전 (UpdateTrain에서 부드럽게 보간된 각도 사용)
    // train1 이미지는 오른쪽(+x), train2 이미지는 왼쪽(-x)을 보고 있음
    float baseAngle = (train == &game.train1) ? 0.0f : 180.0f;
    float rot = train->renderAngle - baseAngle;

    float cx = train->pos.x + TRAIN_WIDTH / 2.0f;
    float cy = train->pos.y + TRAIN_HEIGHT / 2.0f;

    Matrix savedTransform;
    g->GetTransform(&savedTransform);
    g->TranslateTransform(cx, cy);
    g->RotateTransform(rot);
    g->DrawImage(train->image, -TRAIN_WIDTH / 2.0f, -TRAIN_HEIGHT / 2.0f, TRAIN_WIDTH, TRAIN_HEIGHT);
    if (IsTrainOverheated(train) && g_overHeatBrush) {
        g->FillRectangle(g_overHeatBrush, -TRAIN_WIDTH / 2.0f, -TRAIN_HEIGHT / 2.0f, TRAIN_WIDTH, TRAIN_HEIGHT);
    }
    g->SetTransform(&savedTransform);

    DrawHeatBar(train, g);

    // 탈선 위기(앞에 남은 레일이 1개 이하)이면 기차 상단에 경고 이미지 깜빡임
    if (!train->finished && !game.gameOver &&
        CountRailsAhead(train, &game.rail, 3) <= 1 &&
        g_emergencyImage && g_emergencyImage->GetLastStatus() == Ok) {
        bool blinkOn = ((GetTickCount64() / 300) % 2) == 0; // 0.3초 간격 깜빡임
        if (blinkOn) {
            const float warnW = 64.0f;
            const float warnH = 64.0f * 369.0f / 677.0f; // 원본 비율 유지
            float wx = train->pos.x + (TRAIN_WIDTH - warnW) / 2.0f;
            float wy = train->pos.y - 18.0f - warnH; // 열 바 위쪽
            g->DrawImage(g_emergencyImage, wx, wy, warnW, warnH);
        }
    }

}

void InitResource(Resource* resource, ResourceType type, float x, float y) {
    resource->type = type;
    resource->pos = { x, y };
    resource->spawnPos = { x, y };
    resource->size = RESOURCE_SIZE;
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

void DrawResource(Resource* resource, Graphics* g, Camera cam, int viewW, int viewH) {
    if (!resource->active) return;
    if (!IsWorldRectVisible(resource->pos.x, resource->pos.y, resource->size, resource->size, cam, viewW, viewH, 16.0f)) return;

    float scale = 1.0f - resource->harvestProgress * 0.6f;
    float drawSize = resource->size * scale;
    float drawX = resource->pos.x + (resource->size - drawSize) / 2.0f;
    float drawY = resource->pos.y + (resource->size - drawSize) / 2.0f;

    static SolidBrush trunk(Color(255, 120, 72, 32));
    static SolidBrush leaves(Color(255, 32, 150, 76));
    static SolidBrush rock(Color(255, 115, 120, 130));
    static Pen edge(Color(255, 75, 80, 90), 2.0f);

    if (resource->type == RESOURCE_TREE) {
        g->FillRectangle(&trunk, drawX + drawSize * 0.42f, drawY + drawSize * 0.48f, drawSize * 0.16f, drawSize * 0.42f);
        g->FillEllipse(&leaves, drawX + drawSize * 0.12f, drawY + drawSize * 0.02f, drawSize * 0.76f, drawSize * 0.62f);
    }
    else {
        RectF rect(drawX + drawSize * 0.08f, drawY + drawSize * 0.22f, drawSize * 0.84f, drawSize * 0.58f);
        g->FillEllipse(&rock, rect);
        g->DrawEllipse(&edge, rect);
    }
}

bool CanPlaceResourceAt(float x, float y) {
    float padding = 8.0f;
    float size = RESOURCE_SIZE;
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
    float elapsed = game.baseExplosionActive
        ? (float)(GetTickCount64() - game.baseExplosionStartTime) / 1000.0f
        : BASE_EXPLOSION_FLASH_TIME;

    if (game.baseExplosionActive) {
        DrawBaseExplosionOverlay(g, elapsed);
    }
    else {
        SolidBrush overlay(Color(180, 0, 0, 0));
        g->FillRectangle(&overlay, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, 80, FontStyleBold, UnitPixel);
    BYTE textAlpha = 255;
    if (game.baseExplosionActive) {
        float t = (elapsed - 0.45f) / 0.65f;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        textAlpha = (BYTE)(255.0f * t);
    }
    SolidBrush textBrush(Color(textAlpha, 255, 245, 120));
    SolidBrush shadowBrush(Color((BYTE)(textAlpha * 0.75f), 60, 8, 0));
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);

    std::wstring msg;
    if (game.winner == 1) msg = L"Player 1 Win";
    else if (game.winner == 2) msg = L"Player 2 Win";
    else msg = L"DRAW!";

    RectF rect(0.0f, 0.0f, (REAL)SCREEN_WIDTH, (REAL)SCREEN_HEIGHT);
    RectF shadowRect(4.0f, 5.0f, (REAL)SCREEN_WIDTH, (REAL)SCREEN_HEIGHT);
    g->DrawString(msg.c_str(), -1, &font, shadowRect, &sf, &shadowBrush);
    g->DrawString(msg.c_str(), -1, &font, rect, &sf, &textBrush);
}

void DrawBaseExplosionOverlay(Graphics* g, float elapsed) {
    float t = elapsed / BASE_EXPLOSION_FLASH_TIME;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    float animTime = elapsed;
    if (animTime > BASE_EXPLOSION_FLASH_TIME) animTime = BASE_EXPLOSION_FLASH_TIME;

    BYTE flashAlpha = (BYTE)(230.0f - 70.0f * t);
    SolidBrush heat(Color(flashAlpha, 155, 12, 0));
    g->FillRectangle(&heat, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    int pulse = (int)(sinf(animTime * 18.0f) * 18.0f);
    SolidBrush core(Color((BYTE)(230.0f * (1.0f - t * 0.35f)), 255, 210, 35));
    SolidBrush mid(Color((BYTE)(210.0f * (1.0f - t * 0.25f)), 255, 95, 8));
    SolidBrush smoke(Color((BYTE)(115.0f + 60.0f * t), 35, 22, 20));

    for (int i = 0; i < 9; i++) {
        float cx = (float)((i * 173 + 120) % (SCREEN_WIDTH + 240) - 120);
        float cy = (float)((i * 97 + 70) % (SCREEN_HEIGHT + 160) - 80);
        float radius = 180.0f + (float)((i * 41) % 130) + animTime * 170.0f + pulse;
        g->FillEllipse((i % 2 == 0) ? &core : &mid,
            cx - radius * 0.65f, cy - radius * 0.45f,
            radius * 1.3f, radius * 0.9f);
    }

    for (int i = 0; i < 7; i++) {
        float cx = (float)((i * 211 + 90) % (SCREEN_WIDTH + 300) - 150);
        float cy = (float)(SCREEN_HEIGHT - 120 - ((i * 53) % 220));
        float radius = 210.0f + (float)((i * 37) % 90) + animTime * 95.0f;
        g->FillEllipse(&smoke,
            cx - radius * 0.55f, cy - radius * 0.35f,
            radius * 1.1f, radius * 0.7f);
    }

    BYTE dimAlpha = (BYTE)(60.0f + 80.0f * t);
    SolidBrush dim(Color(dimAlpha, 0, 0, 0));
    g->FillRectangle(&dim, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

// ─────────────────────────────────────────────────────────────
//  Menu helpers
// ─────────────────────────────────────────────────────────────
void DrawMenuButton(Graphics* g, const wchar_t* text, int x, int y, int w, int h) {
    SolidBrush btnBg(Color(220, 60, 60, 60));
    Pen borderPen(Color(255, 200, 200, 200), 1.5f);
    SolidBrush txtBrush(Color(255, 255, 255, 255));
    g->FillRectangle(&btnBg, x, y, w, h);
    g->DrawRectangle(&borderPen, x, y, w, h);
    FontFamily ff(L"Arial");
    Font btnFont(&ff, 18, FontStyleBold, UnitPixel);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);
    RectF rect((REAL)x, (REAL)y, (REAL)w, (REAL)h);
    g->DrawString(text, -1, &btnFont, rect, &sf, &txtBrush);
}

// Pause menu layout  (bx=470, by=190, bw=340, bh=300)
//   계속하기  → bx+20, by+65,  w=300, h=50
//   설정      → bx+20, by+130, w=300, h=50
//   게임 종료 → bx+20, by+195, w=300, h=50
void DrawPauseMenu(Graphics* g) {
    SolidBrush dim(Color(160, 0, 0, 0));
    g->FillRectangle(&dim, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    const int bx = 470, by = 190, bw = 340, bh = 300;
    SolidBrush boxBg(Color(230, 30, 30, 30));
    Pen boxBorder(Color(255, 180, 180, 180), 2.0f);
    g->FillRectangle(&boxBg, bx, by, bw, bh);
    g->DrawRectangle(&boxBorder, bx, by, bw, bh);

    FontFamily ff(L"Arial");
    Font titleFont(&ff, 24, FontStyleBold, UnitPixel);
    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    RectF titleRect((REAL)bx, (REAL)(by + 15), (REAL)bw, 35.0f);
    g->DrawString(L"일시정지", -1, &titleFont, titleRect, &sfCenter, &whiteBrush);

    DrawMenuButton(g, L"계속하기",  bx + 20, by + 65,  300, 50);
    DrawMenuButton(g, L"설정",      bx + 20, by + 130, 300, 50);
    DrawMenuButton(g, L"게임 종료", bx + 20, by + 195, 300, 50);
}

// Settings layout  (bx=440, by=230, bw=400, bh=260)
//   volume slider track: sx=bx+80=520, sy=by+75=305, sw=290, sh=10
//   뒤로 button:         bx+20, by+160, bw-40=360, h=50
void DrawSettingsMenu(Graphics* g) {
    SolidBrush dim(Color(160, 0, 0, 0));
    g->FillRectangle(&dim, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    const int bx = 440, by = 230, bw = 400, bh = 260;
    SolidBrush boxBg(Color(230, 30, 30, 30));
    Pen boxBorder(Color(255, 180, 180, 180), 2.0f);
    g->FillRectangle(&boxBg, bx, by, bw, bh);
    g->DrawRectangle(&boxBorder, bx, by, bw, bh);

    FontFamily ff(L"Arial");
    Font titleFont(&ff, 24, FontStyleBold, UnitPixel);
    Font labelFont(&ff, 18, FontStyleRegular, UnitPixel);
    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    StringFormat sfLeft;

    // title
    RectF titleRect((REAL)bx, (REAL)(by + 15), (REAL)bw, 35.0f);
    g->DrawString(L"설정", -1, &titleFont, titleRect, &sfCenter, &whiteBrush);

    // "볼륨" label
    g->DrawString(L"볼륨", -1, &labelFont,
        PointF((REAL)(bx + 20), (REAL)(by + 68)), &sfLeft, &whiteBrush);

    // slider track
    const int sx = bx + 80, sy = by + 75, sw = 290, sh = 10;
    SolidBrush trackBg(Color(255, 80, 80, 80));
    g->FillRectangle(&trackBg, sx, sy, sw, sh);

    // filled portion
    int filled = sw * game.volume / 100;
    SolidBrush fillBrush(Color(255, 100, 180, 255));
    g->FillRectangle(&fillBrush, sx, sy, filled, sh);

    // thumb
    int tx = sx + filled - 8;
    SolidBrush thumbBrush(Color(255, 230, 230, 230));
    g->FillRectangle(&thumbBrush, tx, sy - 5, 16, 20);

    // percentage text
    wchar_t volStr[16];
    swprintf_s(volStr, L"%d%%", game.volume);
    g->DrawString(volStr, -1, &labelFont,
        PointF((REAL)(bx + bw - 55), (REAL)(by + 68)), &sfLeft, &whiteBrush);

    // back button
    DrawMenuButton(g, L"뒤로", bx + 20, by + 160, bw - 40, 50);
}

// 조작법 한 줄: [키]  [설명]
static void DrawCtrlLine(Graphics* g, Font* keyFont, Font* descFont,
    int x, int y, const wchar_t* key, const wchar_t* desc) {
    static SolidBrush keyBrush(Color(255, 255, 215, 70));
    static SolidBrush descBrush(Color(255, 235, 235, 235));
    StringFormat sf;
    g->DrawString(key, -1, keyFont, PointF((REAL)x, (REAL)y), &sf, &keyBrush);
    g->DrawString(desc, -1, descFont, PointF((REAL)(x + 135), (REAL)y), &sf, &descBrush);
}

void DrawControlsScreen(Graphics* g) {
    // 시작 이미지를 어둡게 깔고 그 위에 조작법 표시
    g->Clear(Color(255, 18, 22, 28));
    if (game.startScreenImage && game.startScreenImage->GetLastStatus() == Ok) {
        g->DrawImage(game.startScreenImage, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        SolidBrush dim(Color(210, 0, 0, 0));
        g->FillRectangle(&dim, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    FontFamily ff(L"맑은 고딕");
    Font titleFont(&ff, 40, FontStyleBold, UnitPixel);
    Font headFont(&ff, 24, FontStyleBold, UnitPixel);
    Font keyFont(&ff, 18, FontStyleBold, UnitPixel);
    Font descFont(&ff, 18, FontStyleRegular, UnitPixel);

    SolidBrush white(Color(255, 255, 255, 255));
    SolidBrush cyan(Color(255, 120, 200, 255));
    SolidBrush gray(Color(255, 200, 200, 200));
    StringFormat sfL;
    StringFormat sfC; sfC.SetAlignment(StringAlignmentCenter);

    // 제목
    RectF titleRect(0.0f, 24.0f, (REAL)SCREEN_WIDTH, 56.0f);
    g->DrawString(L"조작법", -1, &titleFont, titleRect, &sfC, &white);

    const int lx = 130, rx = 690;
    int y;

    // 플레이어 1
    g->DrawString(L"플레이어 1", -1, &headFont, PointF((REAL)lx, 110.0f), &sfL, &cyan);
    y = 160;
    DrawCtrlLine(g, &keyFont, &descFont, lx, y, L"W A S D", L"이동");          y += 40;
    DrawCtrlLine(g, &keyFont, &descFont, lx, y, L"Q",       L"설치물 종류 변경"); y += 40;
    DrawCtrlLine(g, &keyFont, &descFont, lx, y, L"R",       L"레일 방향 전환");   y += 40;
    DrawCtrlLine(g, &keyFont, &descFont, lx, y, L"E",       L"설치하기");        y += 40;
    DrawCtrlLine(g, &keyFont, &descFont, lx, y, L"F",       L"양동이 줍기 / 놓기"); y += 40;
    DrawCtrlLine(g, &keyFont, &descFont, lx, y, L"F4",      L"기차 가속");

    // 플레이어 2
    g->DrawString(L"플레이어 2", -1, &headFont, PointF((REAL)rx, 110.0f), &sfL, &cyan);
    y = 160;
    DrawCtrlLine(g, &keyFont, &descFont, rx, y, L"방향키", L"이동");           y += 40;
    DrawCtrlLine(g, &keyFont, &descFont, rx, y, L"1",     L"설치물 종류 변경"); y += 40;
    DrawCtrlLine(g, &keyFont, &descFont, rx, y, L"2",     L"레일 방향 전환");   y += 40;
    DrawCtrlLine(g, &keyFont, &descFont, rx, y, L"3",     L"설치하기");        y += 40;
    DrawCtrlLine(g, &keyFont, &descFont, rx, y, L"0",     L"양동이 줍기 / 놓기"); y += 40;
    DrawCtrlLine(g, &keyFont, &descFont, rx, y, L"F5",    L"기차 가속");

    // 공통 / 시스템
    g->DrawString(L"공통 / 시스템", -1, &headFont, PointF((REAL)lx, 400.0f), &sfL, &cyan);
    y = 448;
    DrawCtrlLine(g, &keyFont, &descFont, lx, y, L"ESC", L"일시정지 / 뒤로"); y += 40;
    DrawCtrlLine(g, &keyFont, &descFont, lx, y, L"F2",  L"무한 레일 모드");  y += 40;
    DrawCtrlLine(g, &keyFont, &descFont, lx, y, L"F3",  L"무한 자원 (과열 없음)");

    // 자동 동작
    g->DrawString(L"자동 동작", -1, &headFont, PointF((REAL)rx, 400.0f), &sfL, &cyan);
    y = 448;
    g->DrawString(L"• 물가에 가까이 가면 양동이 자동 충전", -1, &descFont, PointF((REAL)rx, (REAL)y), &sfL, &gray); y += 36;
    g->DrawString(L"• 물 양동이로 기차에 닿으면 열 냉각",   -1, &descFont, PointF((REAL)rx, (REAL)y), &sfL, &gray); y += 36;
    g->DrawString(L"• 레일이 거의 떨어지면 기차에 경고 표시", -1, &descFont, PointF((REAL)rx, (REAL)y), &sfL, &gray);

    // 뒤로 버튼
    DrawMenuButton(g, L"뒤로", CTRL_BACK_LEFT, CTRL_BACK_TOP,
        CTRL_BACK_RIGHT - CTRL_BACK_LEFT, CTRL_BACK_BOTTOM - CTRL_BACK_TOP);
}

void DrawGameStartOverlay(Graphics* g) {
    // 반투명 어두운 배경
    SolidBrush dim(Color(160, 0, 0, 0));
    g->FillRectangle(&dim, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    FontFamily ff(L"Arial");
    Font bigFont(&ff, 90, FontStyleBold, UnitPixel);
    SolidBrush yellowBrush(Color(255, 255, 220, 0));
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);

    RectF rect(0.0f, 0.0f, (REAL)SCREEN_WIDTH, (REAL)SCREEN_HEIGHT);
    g->DrawString(L"Game Start!", -1, &bigFont, rect, &sf, &yellowBrush);
}

