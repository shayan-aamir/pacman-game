#include "headers/effects.h"
#include <bits/stdc++.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <raylib.h>
#include <raymath.h>
#define INF INT_MAX
using namespace std;

enum STATE
{
  SCATTER,
  FRIGHTENED,
  CHASE,
  EATEN
};

// Make it Private and not globally Available
vector<vector<char>> prop(22, vector<char>(19, ' '));
int score = 0;
int Lives = 2;
int Power = 3;
bool lose = false;

void saveVector(const vector<vector<char>> &vector, // loading grid from file
                const std::string &filename)
{
  std::ofstream file(filename, std::ios::out | std::ios::binary);
  if (!file.is_open())
  {
    std::cerr << "Error opening file: " << filename << std::endl;
    return;
  }

  size_t rows = vector.size();
  size_t cols = vector[0].size();

  // Write the dimensions of the vector
  file.write(reinterpret_cast<const char *>(&rows), sizeof(size_t));
  file.write(reinterpret_cast<const char *>(&cols), sizeof(size_t));

  // Write the vector data
  for (const auto &row : vector)
  {
    file.write(row.data(), cols);
  }

  file.close();
}

// Function to load the 2D vector from a binary file
vector<vector<char>> loadVector(const std::string &filename)
{
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file.is_open())
  {
    std::cerr << "Error opening file: " << filename << std::endl;
    return {};
  }

  size_t rows, cols;

  // Read the dimensions of the vector
  file.read(reinterpret_cast<char *>(&rows), sizeof(size_t));
  file.read(reinterpret_cast<char *>(&cols), sizeof(size_t));

  vector<vector<char>> vector(rows, std::vector<char>(cols));

  // Read the vector data
  for (auto &row : vector)
  {
    file.read(row.data(), cols);
  }

  file.close();

  return vector;
}
struct Point
{
  int x, y;
  Point(int _x, int _y) : x(_x), y(_y) {}
};

bool operator>(const Point &a, const Point &b) // compare both x and y at same time
{
  return make_pair(a.x, a.y) > make_pair(b.x, b.y);
}

bool operator<(const Point &a, const Point &b)
{
  return make_pair(a.x, a.y) < make_pair(b.x, b.y);
}

bool isNotValid(int x, int y, int rows, int cols)
{
  return x < 0 || x >= cols || y < 0 || y >= rows || prop[y][x] == '#';
}

const int dx[] = {0, +1, 0, -1};
const int dy[] = {-1, 0, 1, 0};

void DrawMap(Texture2D map)
{
  for (int i = 0; i < 22; i++)
  {
    for (int j = 0; j < 19; j++)
    {
      // cout << prop[i][j] << " ";
      if (prop[i][j] == ' ')
      {
        Rectangle src = {0, 8, 8, 8};
        DrawTexturePro(map, src, {(float)j * 48 + 200, (float)i * 48, 48, 48},
                       {0, 0}, 0, WHITE);
      }
      else if (prop[i][j] == '0')
      {
        Rectangle src = {8, 8, 8, 8};
        DrawTexturePro(map, src, {(float)j * 48 + 200, (float)i * 48, 48, 48},
                       {0, 0}, 0, WHITE);
      }
      else if (prop[i][j] == 'P')
      {
        Rectangle src = {8 + 8 * 2, 8, 8, 8};
        DrawTexturePro(map, src, {(float)j * 48 + 200, (float)i * 48, 48, 48},
                       {0, 0}, 0, WHITE);
      }

      if (prop[i][j] != '#')
        continue;
      // Now check how many sides are attached
      int loc = 0;
      for (int k = 0; k < 4; k++)
      {
        int xx = j + dx[k];
        int yy = i + dy[k];

        if (xx < 0 || xx >= 19 || yy < 0 || yy >= 22)
          continue;

        if (prop[yy][xx] == '#')
          loc |= (int)pow(2, k);
      }

      // Draw the appropriate sprite
      Rectangle src = {(float)loc * 8, 0, 8, 8};
      DrawTexturePro(map, src, {(float)j * 48 + 200, (float)i * 48, 48, 48},
                     {0, 0}, 0, WHITE);
    }
    // cout << endl;
  }
}

class Entity
{
protected:
  int x;
  int y;
  int speed;
  int dX;
  int dY;
  int pdX;
  int pdY;
  int frameC = 0;
  int frameS = 8;
  int currentFrame = 0;

public:
  Vector2 getPos() { return {(float)x, (float)y}; }
  void Check()
  {
    if (dX == pdX && dY == pdY)
      return;

    // Check all 4 corners are not in a wall
    // 47 47, 0 47, 47 0
    int dx[] = {0, 1, 0, 1};
    int dy[] = {0, 1, 1, 0};

    for (int i = 0; i < 4; i++)
    {
      int xx = x + 3 + pdX * speed;
      int yy = y + 3 + pdY * speed;

      int grid_x = (xx + 42 * dx[i]) / 48;
      int grid_y = (yy + 42 * dy[i]) / 48;
      // DrawCircle(xx + 42 * dx[i] + 200, yy + 42 * dy[i], 5, RED);
      // cout << grid_x << " " << grid_y << endl;
      if (prop[grid_y][grid_x] == '#')
        return;
    }

    dX = pdX;
    dY = pdY;
  }
  void virtual Update() = 0;
};

// Capitalize this class
class Pacman : public Entity
{
  Image pac_left;
  Texture2D pacLeft;
  Rectangle pacRect = {0, 0, 16, 16};
  Sound chomp;
  bool eaten = false;
  bool kill = false;
  bool set = false;
  int coordX;
  int coordY;

public:
  bool getEaten() { return eaten; }
  void setEaten(bool eaten) { this->eaten = eaten; }
  bool killMode() { return kill; }
  void killMode(bool kill) { this->kill = kill; }
  Pacman() : Entity()
  {
    x = 9 * 48;
    y = 16 * 48;
    chomp = LoadSound("./assets/sound/chomp.wav");
    SetSoundVolume(chomp, 0.5);
    speed = 5;
    dX = 0;
    pdX = -1;
    dY = 0;
    pdY = 0;
    pac_left = LoadImage("./assets/Sprite-0001.png");
    pacLeft = LoadTextureFromImage(pac_left);
  }
  Rectangle getRect() { return Rectangle{(float)x, (float)y, 48, 48}; }
  Vector2 getDir() { return Vector2{(float)dX, (float)dY}; };
  void Reset()
  {
    x = 9 * 48;
    y = 16 * 48;
    dX = -1;
    pdX = -1;
    dY = 0;
    pdY = 0;
  }
  void Update()
  {
    Check();
    int centered_x = x + 24;
    int centered_y = y + 24;
    int i = (y + 24) / 48.0;
    int j = (x + 24) / 48.0;
    // If Right or bottom then floor

    if (prop[i][j] == ' ')
    {
      score += 20;
      prop[i][j] = 'X';
      PlaySound(chomp);
    }
    else if (prop[i][j] == '0')
    {
      score += 40;
      prop[i][j] = 'X';
      PlaySound(chomp);
      eaten = true;
    }
    else if (prop[i][j] == 'P')
    {
      score += 10;
      prop[i][j] = 'X';
      PlaySound(chomp);
      killMode(true);
    }

    // DrawCircle(x + 24 + 200, y + 24, 5, RED);
    // cout << i + dY << " " << j + dX << endl;
    if (prop[(centered_y + 26 * dY) / 48][(centered_x + 26 * dX) / 48] != '#')
    {
      // DrawCircle(centered_x + 25 * dX + 200, centered_y + 25 * dY, 5, RED);

      // Increase frame
      Animate();
      x += dX * speed;
      y += dY * speed;
    }
    i = (y + 24) / 48.0;
    j = (x + 24) / 48.0;
    // Update on grid
    // cout << y / 48 << " " << x / 48 << endl;
  }

  void Animate()
  {
    frameC++;
    if (frameC >= (60 / frameS))
    {
      frameC = 0;
      currentFrame = (currentFrame + 1) % 3;
      pacRect.x = currentFrame * 16;
    }
  }

  void Draw()
  {
    if (dX == 0 && dY == -1)
    {
      pacRect.y = 2 * 16;
    }
    else if (dX == 0 && dY == 1)
    {
      pacRect.y = 3 * 16;
    }
    else if (dX == -1 && dY == 0)
    {
      pacRect.y = 1 * 16;
    }
    else if (dX == 1 && dY == 0)
    {
      pacRect.y = 0 * 16;
    }

    if (currentFrame == 2)
    {
      pacRect.y = 0 * 16;
    }

    DrawTexturePro(pacLeft, pacRect, {(float)x + 200, (float)y, 48, 48}, {0, 0},
                   0, WHITE);
  }

  void PowerUp()
  {
    if (Power <= 0)
      return;
    int centerX = (x + 24) / 48.0;
    int centerY = (y + 24) / 48.0;
    if (IsKeyDown(KEY_SPACE))
    {
      while (prop[centerY + dY][centerX + dX] != '#')
      {
        centerY += dY;
        centerX += dX;
        cout << centerY << " " << centerX << endl;
      }
      coordX = centerX * 48 + 200;
      coordY = centerY * 48;
      DrawCircle(centerX * 48 + 24 + 200, centerY * 48 + 24, 20, YELLOW);
      set = true;
    }
    if (set && IsKeyUp(KEY_SPACE))
    {
      EffectManager::addEffect(Smoke, Vector2{(float)x + 200, (float)y});
      x = coordX - 200;
      y = coordY;
      EffectManager::addEffect(Dash, Vector2{(float)x + 200, (float)y});
      set = false;
    }
  }

  void Control()
  {
    if (IsKeyPressed(KEY_W))
    {
      pdX = 0;
      pdY = -1;
    }
    else if (IsKeyPressed(KEY_A))
    {
      pdX = -1;
      pdY = 0;
    }
    else if (IsKeyPressed(KEY_S))
    {
      pdX = 0;
      pdY = 1;
    }
    else if (IsKeyPressed(KEY_D))
    {
      pdX = 1;
      pdY = 0;
    }
    PowerUp();
  }
};

class Ghost : public Entity
{
  Image ghostI;
  Texture2D ghostT;
  Rectangle GhostRect;
  // STATE: behave according to STATE
  STATE state = CHASE;
  int timer = 0;
  Vector2 scatterLocation = {18 * 48, -48};
  Vector2 homeLocation = {9 * 48, 10 * 48};
  Vector2 start;

public:
  Vector2 scatter() { return scatterLocation; }
  void Reset()
  {
    x = 11 * 48;
    y = 8 * 48;
    start = {(float)x, (float)y};
    dX = 1;
    dY = 0;
  }
  Ghost(float Color, Vector2 scatterLocation)
  {
    x = 11 * 48 - Color * 48;
    y = 8 * 48;
    start = {(float)x, (float)y};
    this->scatterLocation = scatterLocation;
    speed = 4;
    ghostI = LoadImage("./assets/Sprite-0001.png");
    ghostT = LoadTextureFromImage(ghostI);
    GhostRect = {0, 4 * 16 + Color * 16, 16, 16};
    speed = 4;
    dX = 1;
    dY = 0;
  }

  void Draw()
  {
    // Up
    if (state == CHASE || state == SCATTER)
    {
      DrawTexturePro(ghostT, GhostRect, {(float)x + 200, (float)y, 48, 48},
                     {0, 0}, 0, WHITE);
    }
    else if (state == FRIGHTENED)
    {
      DrawTexturePro(
          ghostT, Rectangle{(float)16 * currentFrame + 8 * 16, 4 * 16, 16, 16},
          {(float)x + 200, (float)y, 48, 48}, {0, 0}, 0, WHITE);
    }
    else if (state == EATEN)
    {
      // Right
      float source_x = 0 + 16 * 8;
      if (dX == -1)
        source_x += 16;
      else if (dY == -1)
        source_x += 16 * 2;
      else if (dY == 1)
        source_x += 16 * 3;
      DrawTexturePro(ghostT, Rectangle{source_x, 5 * 16, 16, 16},
                     {(float)x + 200, (float)y, 48, 48}, {0, 0}, 0, WHITE);
    }
  }

  void setState(STATE s)
  {
    if (s == FRIGHTENED)
      timer = 5 * 60;
    else
      timer = 0;
    state = s;
  }

  virtual Vector2 chaseTarget(Pacman &pac) = 0;

  bool isHome(int centered_x_grid, int centered_y_grid)
  {
    return (centered_y_grid == 10 &&
            (centered_x_grid >= 8 && centered_x_grid <= 10));
  }
  Vector2 target(Pacman &pac)
  {
    if (CheckCollisionPointRec(Vector2{(float)x + 200, (float)y},
                               Rectangle{8 * 48 + 200, 9 * 48, 48 * 3, 48 * 2}))
      return Vector2{9 * 48, 9 * 48};
    switch (state)
    {
    case SCATTER:
      return scatterLocation;
      break;
    case CHASE:
      return chaseTarget(pac);
      break;
    case FRIGHTENED:
      return {(float)x, (float)y};
      break;
    case EATEN:
      return homeLocation;
      break;
    default:
      return {(float)x, (float)y};
      break;
    }
  }
  // The state controls the target and the texture that appears
  void next(Vector2 target)
  {
    // Of all the directions possible, Return the direction that is NOT opposite
    // to current AND allows the Ghost to be closest
    int centered_x = (x + 24);
    int centered_y = (y + 24);
    int centered_x_grid = (x + 24) / 48;
    int centered_y_grid = (y + 24) / 48;
    int target_x = (target.x + 24) / 48;
    int target_y = (target.y + 24) / 48;
    vector<pair<int, float>> distances;
    for (int i = 0; i < 4; i++)
    {
      // Make sure it is in bounds
      if (!isHome(centered_x_grid, centered_y_grid) && dx[i] == -dX &&
          dy[i] == -dY)
        continue;
      if (isNotValid((centered_x + dx[i] * 24) / 48,
                     (centered_y + dy[i] * 24) / 48, 22, 19))
        continue;
      if (prop[(centered_y + 26 * dy[i]) / 48]
              [(centered_x + 26 * dx[i]) / 48] == '#')
        continue;
      // Calculate distance
      distances.push_back(
          {i, distance(centered_x_grid + dx[i], centered_y_grid + dy[i],
                       target_x, target_y)});
    }
    sort(distances.begin(), distances.end(),
         [](pair<int, float> a, pair<int, float> b)
         {
           return a.second < b.second;
         });
    // cout << distances.size() << endl;
    if (state == FRIGHTENED)
    {
      int randomValid = GetRandomValue(0, distances.size() - 1);
      pdX = dx[distances[randomValid].first];
      pdY = dy[distances[randomValid].first];
    }
    else
    {
      pdX = dx[distances[0].first];
      pdY = dy[distances[0].first];
    }
  }
  float distance(int x1, int y1, int x2, int y2)
  {
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
  }

  void Animate()
  {
    frameC++;
    if (frameC >= (60 / frameS))
    {
      frameC = 0;
      currentFrame = (currentFrame + 1) % 2;
      GhostRect.x = currentFrame * 16;
      if (dX == 0 && dY == -1)
      {
        GhostRect.x += 2 * 32;
      }
      // Down
      else if (dX == 0 && dY == 1)
      {
        GhostRect.x += 3 * 32;
      }
      // Left
      else if (dX == -1 && dY == 0)
      {
        GhostRect.x += 1 * 32;
      }
      // Right
      else if (dX == 1 && dY == 0)
      {
        GhostRect.x += 0 * 32;
      }
    }
  }
  void Update() override {}
  void Update(Pacman &pac)
  {
    next(target(pac));
    Check();
    int centered_x = x + 24;
    int centered_y = y + 24;
    int i = (y + 24) / 48.0;
    int j = (x + 24) / 48.0;
    // If Right or bottom then floor
    // DrawCircle(x + 24 + 200, y + 24, 5, RED);
    // cout << i + dY << " " << j + dX << endl;
    if (prop[(centered_y + 26 * dY) / 48][(centered_x + 26 * dX) / 48] != '#')
    {
      // DrawCircle(centered_x + 25 * dX + 200, centered_y + 25 * dY, 5, RED);
      Animate();
      x += dX * speed;
      y += dY * speed;
    }
    i = (y + 24) / 48.0;
    j = (x + 24) / 48.0;
    // Update on grid
    // cout << y / 48 << " " << x / 48 << endl;
    // DrawRectangle(8*48 + 200, 9*48, 48*3, 48*2, RED);
    int rand = GetRandomValue(0, 1000);

    if (state == FRIGHTENED && timer != 0)
      timer--;
    else if (state == FRIGHTENED && timer == 0)
      setState(CHASE);

    if (state == CHASE && rand < 2 && !isHome(i, j))
    {
      setState(SCATTER);
    }
    else if (state == SCATTER && rand < 10)
    {
      setState(CHASE);
    }
    if (CheckCollisionPointRec(Vector2{(float)x, (float)y},
                               Rectangle{8 * 48, 9 * 48, 48 * 3, 48 * 2}))
    {
      setState(CHASE);
    }
    if (state != EATEN &&
        CheckCollisionRecs(pac.getRect(),
                           Rectangle{(float)x, (float)y, 48, 48}))
    {
      cout << "Collision" << endl;
      if (state == FRIGHTENED)
      {
        setState(EATEN);
      }
      else
      {
        pac.Reset();
        Lives--;
        if (Lives <= 0)
          lose = true;
      }
    }
  }
};

class Blinky : public Ghost
{
public:
  Blinky() : Ghost(0.0f, Vector2{18 * 48, -48}) {}
  Vector2 chaseTarget(Pacman &pac) override { return pac.getPos(); }
};

class Pinky : public Ghost
{
public:
  Pinky() : Ghost(1, Vector2{0, -48}) {}
  Vector2 chaseTarget(Pacman &pac) override
  {
    return Vector2Add(pac.getPos(),
                      Vector2Multiply(pac.getDir(), {48 * 4, 48 * 4}));
  }
};

class Clyde : public Ghost
{
public:
  Clyde() : Ghost(3, Vector2{0, 22 * 48}) {}
  Vector2 chaseTarget(Pacman &pac) override
  {
    if (distance(getPos().x, getPos().y, pac.getPos().x, pac.getPos().y) <
        8 * 48)
      return scatter();
    return pac.getPos();
  }
};

class Inky : public Ghost
{
public:
  Inky() : Ghost(2, Vector2{18 * 48, 22 * 48}) {}
  Vector2 chaseTarget(Pacman &pac) override
  {
    return Vector2Subtract(pac.getPos(),
                           Vector2Multiply(pac.getDir(), {48 * 4, 48 * 4}));
  }
};

// Initialize the map
list<Effect> EffectManager::effectList;
map<EffectType, Texture2D> EffectMap::effectMap;

int main()
{
  const int screenWidth = 912 + 400;
  const int screenHeight = 1056;
  InitWindow(screenWidth, screenHeight, "Pac-Man");
  InitAudioDevice();
  Image Maze_img = LoadImage("./assets/map.png");
  Color *img = LoadImageColors(Maze_img);
  Texture2D Maze = LoadTexture("./assets/map.png");
  Texture2D map = LoadTexture("./assets/tileSheet.png");
  // cout << Maze.height << " " << Maze.width << endl;
  SetTargetFPS(60);
  Music bg_music = LoadMusicStream("./assets/sound/pacman_remix.mp3");
  SetMusicVolume(bg_music, 0.7);
  PlayMusicStream(bg_music);
  prop = loadVector("./assets/defaultMap.bin");
  prop[1][1] = 'P';
  saveVector(prop, "./assets/defaultMap.bin");
  Pacman pac;
  Blinky Blinky;
  Pinky Pinky;
  Inky Inky;
  Clyde Clyde;
  EffectMap::Init();
  STATE states[] = {CHASE, FRIGHTENED, EATEN, SCATTER};
  Ghost *ghosts[] = {&Blinky, &Pinky, &Inky, &Clyde};
  int i = 0;
  bool win = false;
  while (!WindowShouldClose())
  {
    BeginDrawing();
    UpdateMusicStream(bg_music);
    ClearBackground(BLACK);
    DrawMap(map);
    // DrawTexture(Maze, 0, 0, WHITE);
    // Convert score to char*
    // if (IsKeyPressed(KEY_SPACE))
    // {
    //   Blinky.setState(FRIGHTENED);
    // }
    string score_str = "Score:" + to_string(score);
    string live_str = "Lives:" + to_string(Lives);
    DrawText(score_str.c_str(), 8, 5, 30, WHITE);
    DrawText(live_str.c_str(), 8, 30, 30, WHITE);
    pac.Draw();
    pac.Control();
    if (!win && !lose)
    {
      pac.Update();
      for (int i = 0; i < 4; i++)
      {
        ghosts[i]->Draw();
        ghosts[i]->Update(pac);
        if (pac.getEaten())
        {
          ghosts[i]->setState(FRIGHTENED);
        }
        else if (pac.killMode())
        {
          ghosts[i]->setState(EATEN);
        }
      }
      if (pac.getEaten())
        pac.setEaten(false);
      else if (pac.killMode())
        pac.killMode(false);
      if (score == 3730)
        win = true;
    }
    if (win)
    {
      DrawText("YOU WIN", (screenWidth - 120) / 2, (screenHeight / 2), 30,
               WHITE);
    }
    if (lose)
    {
      DrawText("YOU LOSE", (screenWidth - 120) / 2, (screenHeight / 2), 30,
               WHITE);
    }
    EffectManager::updateEffects();
    EndDrawing();
  }

  UnloadMusicStream(bg_music);

  CloseWindow();
  return 0;
}
