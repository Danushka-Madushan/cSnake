#include <iostream>
#include <conio.h>
#include <windows.h>
#include <csignal>
#include <string>
#include <iomanip>
#include <random>
#include <chrono>
#include <vector>
#include <algorithm>

enum Directions
{
  Left,
  Right,
  Up,
  Down
};

/* width must be a odd number */
static const int width = 61;
static const int height = 20;
static const bool use_two_jumps_for_x_axis = true;
static const int initialTailLength = 3;
/* refresh rate & game speed - milliseconds */
static const int refresh_rate = 100;

int tailLength = initialTailLength;
struct Point
{
  int x, y;
};
std::vector<Point> tailMap;
std::vector<Directions> tailDirections;

/**
 * BUG FIX: Replaced complex nested maps with a clear Struct.
 * The 'remainingPasses' counter tracks how many tail segments still need to use this turn.
 * This prevents the turn from vanishing if the snake grows before the last piece reaches it.
 */
struct TurningPoint
{
  int x;
  int y;
  Directions direction;
  int remainingPasses;
};
std::vector<TurningPoint> turningPoints;

int snake_head_grid_location_x = ((width - 2) / 2) + 1;
int snake_head_grid_location_y = tailLength + 2;
Directions current_direction = Down;
std::pair<int, int> appleLocation;

/**
 * BUG FIX: Removed endOfTailLocation and endOfTailDirection entirely.
 * Calculating global "last state" variables at the wrong time in the loop caused the timing mismatch.
 */
std::string padWithZeros(int value, unsigned int fieldWidth)
{
  std::string s = std::to_string(value);
  if (s.size() < fieldWidth)
  {
    s.insert(s.begin(), fieldWidth - s.size(), '0');
  }
  return s;
}

int getRandomPoint(int from, int to, bool isEven = false)
{
  /* Static ensures the engine is seeded only once for the lifetime of the program */
  static std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());

  if (isEven)
  {
    /* Generate directly in even range — zero retries, guaranteed O(1) */
    int evenFrom = (from % 2 == 0) ? from : from + 1;
    int evenTo = (to % 2 == 0) ? to : to - 1;
    std::uniform_int_distribution<> distrib(0, (evenTo - evenFrom) / 2);
    return evenFrom + distrib(gen) * 2;
  }

  std::uniform_int_distribution<> distrib(from, to);
  return distrib(gen);
}

void generateAppleLocation()
{
  bool is_new_location_found = false;
  while (!is_new_location_found)
  {
    int x_axis_location = getRandomPoint(2, (width - 2), use_two_jumps_for_x_axis);
    int y_axis_location = getRandomPoint(0, (height - 1));

    /* check if generated locations are not in heaed pos or tail poses */
    if (x_axis_location == snake_head_grid_location_x && y_axis_location == snake_head_grid_location_y)
    {
      continue;
    }

    bool onTail = false;
    for (auto &tail : tailMap)
    {
      if (tail.x == x_axis_location && tail.y == y_axis_location)
      {
        onTail = true;
        break;
      }
    }
    /* retry */
    if (onTail)
    {
      continue;
    }

    appleLocation = {x_axis_location, y_axis_location};
    is_new_location_found = true;
    break;
  }
}

Point moveToDirection(int x, int y, Directions direction)
{
  Point directionMap;
  directionMap.x = x;
  directionMap.y = y;

  switch (direction)
  {
  case Directions::Up:
  {
    if (directionMap.y >= 1)
    {
      directionMap.y--;
    }
    break;
  }
  case Directions::Left:
  {
    if (directionMap.x > (use_two_jumps_for_x_axis ? 3 : 1))
    {
      directionMap.x -= use_two_jumps_for_x_axis ? 2 : 1;
    }
    break;
  }
  case Directions::Down:
  {
    if (directionMap.y < (height - 1))
    {
      directionMap.y++;
    }
    break;
  }
  case Directions::Right:
  {
    if (directionMap.x < (use_two_jumps_for_x_axis ? (width - 2) - 1 : (width - 2)))
    {
      directionMap.x += use_two_jumps_for_x_axis ? 2 : 1;
    }
    break;
  }
  }

  if (directionMap.x == appleLocation.first && directionMap.y == appleLocation.second)
  {
    /**
     * BUG FIX: "Inherit from Parent" logic.
     * Instead of relying on out-of-sync global variables, we look at the CURRENT last piece
     * and mathematically spawn the new piece exactly one block directly behind it.
     */
    int new_tail_x = tailMap.back().x;
    int new_tail_y = tailMap.back().y;
    Directions new_tail_dir = tailDirections.back();

    switch (new_tail_dir)
    {
    case Directions::Up:
    {
      new_tail_y++;
      break;
    }
    case Directions::Down:
    {
      new_tail_y--;
      break;
    }
    case Directions::Left:
    {
      new_tail_x += (use_two_jumps_for_x_axis ? 2 : 1);
      break;
    }
    case Directions::Right:
    {
      new_tail_x -= (use_two_jumps_for_x_axis ? 2 : 1);
      break;
    }
    }

    /**
     * New tail piece can be spawned outside the grid boundaries
     * BUG FIX: Ensure new tail piece doesn't spawn outside the left/right boundaries
     */
    new_tail_x = std::clamp(new_tail_x, 2, width - 2);
    new_tail_y = std::clamp(new_tail_y, 0, height - 1);

    tailMap.push_back({new_tail_x, new_tail_y});
    tailDirections.push_back(new_tail_dir);
    tailLength++;

    /**
     * BUG FIX: Because the snake grew, every active turning point on the map
     * needs to stay open for 1 extra pass so the new piece doesn't miss it.
     */
    for (int i = 0; i < turningPoints.size(); i++)
    {
      turningPoints[i].remainingPasses++;
    }

    generateAppleLocation();
  }

  return directionMap;
}

void calculateTailMovingDirection()
{
  /**
   * BUG FIX: Removed endOfTailLocation calculations.
   * We now loop through segments and let them "consume" the turning points.
   */
  for (int i = 0; i < tailLength; i++)
  {
    auto tailPart = tailMap[i];
    for (int j = 0; j < turningPoints.size(); j++)
    {
      if (tailPart.x == turningPoints[j].x && tailPart.y == turningPoints[j].y)
      {
        tailDirections[i] = turningPoints[j].direction;

        /**
         * BUG FIX: Decrease the turn's lifespan when ANY segment passes it.
         * We no longer rely on if (i == tailLength - 1) which was breaking upon growth.
         */
        turningPoints[j].remainingPasses--;
        break;
      }
    }
  }

  /**
   * BUG FIX: Safely erase turning points ONLY when their counter hits 0.
   * This guarantees every tail piece has successfully made the turn before it gets deleted.
   */
  turningPoints.erase(
    std::remove_if(turningPoints.begin(), turningPoints.end(),
    [](const TurningPoint &tp){ return tp.remainingPasses <= 0; }),
    turningPoints.end()
  );
}

void addTurningPoint()
{
  /* Don't add if one already exists at this exact position */
  for (auto &tp : turningPoints)
  {
    if (tp.x == snake_head_grid_location_x && tp.y == snake_head_grid_location_y)
    {
      /* overwrite with latest */
      tp.direction = current_direction;
      tp.remainingPasses = tailLength;
      return;
    }
  }
  turningPoints.push_back({snake_head_grid_location_x, snake_head_grid_location_y, current_direction, tailLength});
}

void moveTail()
{
  for (int i = 0; i < tailLength; i++)
  {
    switch (tailDirections[i])
    {
    case Directions::Up:
    {
      tailMap[i].y--;
      break;
    }
    case Directions::Left:
    {
      tailMap[i].x -= use_two_jumps_for_x_axis ? 2 : 1;
      break;
    }
    case Directions::Down:
    {
      tailMap[i].y++;
      break;
    }
    case Directions::Right:
    {
      tailMap[i].x += use_two_jumps_for_x_axis ? 2 : 1;
      break;
    }
    }
  }
}

std::string drawGrid(int snake_head_x, int snake_head_y, int tail_length)
{
  /* Build O(1) tail lookup — stack allocation, no heap */
  bool occupied[height][width] = {};
  for (int i = 0; i < tail_length; i++)
  {
    int col = tailMap[i].x - 1;
    int row = tailMap[i].y;
    if (row >= 0 && row < height && col >= 0 && col < width)
    {
      occupied[row][col] = true;
    }
  }

  std::string grid;
  /* The grid size is deterministic — so we pre-allocate it for exact upper bound */
  grid.reserve((width + 1) * (height + 2) + 32);
  grid = "\r";
  /* draw top border */
  for (int i = 0; i < width; i++)
  {
    if (i == 0)
    {
      grid += (char)201;
    }
    else if (i == (width - 1))
    {
      grid += (char)187;
    }
    else
    {
      grid += (char)205;
    }
  }
  grid += "\n";
  /* draw left and right border */
  for (int row = 0; row < height; row++)
  {
    /* left border */
    grid += (char)186;
    for (int column = 0; column < (width - 2); column++)
    {
      if (row == snake_head_y && column == (snake_head_x - 1))
      {
        /* draw head */
        grid += '@';
      }
      /* O(1) lookup */
      else if (occupied[row][column])
      {
        /* draw tail segments */
        grid += '*';
      }
      else if (appleLocation.first == (column + 1) && appleLocation.second == row)
      {
        /* draw apple */
        grid += (char)237;
      }
      else
      {
        grid += ' ';
      }
    }
    /* right border */
    grid += (char)186;
    grid += '\n';
  }
  /* draw bottom border */
  for (int i = 0; i < width; i++)
  {
    if (i == 0)
    {
      grid += (char)200;
    }
    else if (i == (width - 1))
    {
      grid += (char)188;
    }
    else
    {
      grid += (char)205;
    }
  }
  grid += "\n";
  return grid;
}

void signalHandler(int signum)
{
  /* clear terminal */
  std::cout << "\033[2J\033[H";

  /* Terminate program */
  exit(signum);
}

/* method to restart the game on demand */
void resetGame()
{
  /* clear terminal */
  std::cout << "\033[2J\033[H";

  tailLength = initialTailLength;
  tailMap.clear();
  tailDirections.clear();
  turningPoints.clear();

  snake_head_grid_location_x = ((width - 2) / 2) + 1;
  snake_head_grid_location_y = initialTailLength + 2;
  current_direction = Down;

  for (int i = 0; i < tailLength; i++)
  {
    int y_axis_location = (snake_head_grid_location_y - 1) - i;
    tailMap.push_back({((width - 2) / 2) + 1, y_axis_location});
    tailDirections.push_back(current_direction);
  }

  generateAppleLocation();
}

/* method to check self collision */
bool checkSelfCollision()
{
  /**
   * Start at i=1: segment 0 always trails directly behind the head
   * and physically cannot overlap it under normal direction guards
   */
  for (int i = 1; i < tailLength; i++)
  {
    if (tailMap[i].x == snake_head_grid_location_x && tailMap[i].y == snake_head_grid_location_y)
    {
      return true;
    }
  }
  return false;
}

/* game over  */
void drawGameOver(int score, const std::string &reason)
{
  /* clear terminal */
  std::cout << "\033[2J\033[H";

  const int boxInnerWidth = 30;
  std::string box;

  /* top border */
  box += ' ';
  box += (char)201;
  for (int i = 0; i < boxInnerWidth; i++)
  {
    box += (char)205;
  }
  box += (char)187;
  box += '\n';

  /* title row */
  std::string titleContent = "         GAME  OVER           ";
  box += ' ';
  box += (char)186;
  box += titleContent;
  box += (char)186;
  box += '\n';

  /* reason row */
  std::ostringstream reasonRow;
  reasonRow << "  Reason : " << std::left << std::setw(19) << reason;
  box += ' ';
  box += (char)186;
  box += reasonRow.str();
  box += (char)186;
  box += '\n';

  /* score row */
  std::ostringstream scoreRow;
  scoreRow << "  Score  : " << std::left << std::setw(19) << score;
  box += ' ';
  box += (char)186;
  box += scoreRow.str();
  box += (char)186;
  box += '\n';

  /* empty row */
  box += ' ';
  box += (char)186;
  for (int i = 0; i < boxInnerWidth; i++)
  {
    box += ' ';
  }
  box += (char)186;
  box += '\n';

  /* controls row */
  std::string controlsContent = "  [R] Restart   [Q] Quit      ";
  box += ' ';
  box += (char)186;
  box += controlsContent;
  box += (char)186;
  box += '\n';

  /* bottom border */
  box += ' ';
  box += (char)200;
  for (int i = 0; i < boxInnerWidth; i++)
  {
    box += (char)205;
  }
  box += (char)188;
  box += '\n';

  std::cout << box << std::flush;
}

int main()
{
  /* register signal handler for SIGINT (ctrl+c) */
  signal(SIGINT, signalHandler);

  bool shouldRestart = true;

  while (shouldRestart)
  {
    /* reset or initialize the game at first attempt */
    resetGame();
    shouldRestart = false;

    bool gameOver = false;
    std::string gameOverReason;

    while (true)
    {
      if (_kbhit())
      {
        char character = _getch();
        char movement_char = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

        if (movement_char == 'q')
        {
          /* clear terminal */
          std::cout << "\033[2J\033[H";
          return 0;
        }

        /* change moving direction */
        switch (movement_char)
        {
        case 'w':
        {
          if (current_direction != Directions::Down)
          {
            current_direction = Directions::Up;
            addTurningPoint();
          }
          break;
        }
        case 'a':
        {
          if (current_direction != Directions::Right)
          {
            current_direction = Directions::Left;
            addTurningPoint();
          }
          break;
        }
        case 's':
        {
          if (current_direction != Directions::Up)
          {
            current_direction = Directions::Down;
            addTurningPoint();
          }
          break;
        }
        case 'd':
        {
          if (current_direction != Directions::Left)
          {
            current_direction = Directions::Right;
            addTurningPoint();
          }
          break;
        }
        }
      }

      /* refresh rate & game speed */
      Sleep(refresh_rate);

      /* wall collision: O(1) */
      Point newPos = moveToDirection(snake_head_grid_location_x, snake_head_grid_location_y, current_direction);
      if (newPos.x == snake_head_grid_location_x && newPos.y == snake_head_grid_location_y)
      {
        gameOver = true;
        gameOverReason = "Wall";
      }

      snake_head_grid_location_x = newPos.x;
      snake_head_grid_location_y = newPos.y;
      calculateTailMovingDirection();
      moveTail();

      /* self collision: O(N) */
      if (!gameOver && checkSelfCollision())
      {
        gameOver = true;
        gameOverReason = "Self";
      }

      /* HUD */
      std::string hud;
      hud.reserve(80);
      hud += " * Location : [X=";
      hud += padWithZeros(snake_head_grid_location_x, 2);
      hud += ", Y=";
      hud += padWithZeros(snake_head_grid_location_y, 2);
      hud += "] | Apple : [X=";
      hud += padWithZeros(appleLocation.first, 2);
      hud += ", Y=";
      hud += padWithZeros(appleLocation.second, 2);
      hud += "]\n * Score    : ";
      hud += std::to_string(tailLength - initialTailLength);
      hud += '\n';
      std::cout << hud;

      if (gameOver)
      {
        drawGameOver(tailLength - initialTailLength, gameOverReason);

        /* block and wait for R or Q */
        while (true)
        {
          char c = static_cast<char>(std::tolower(static_cast<unsigned char>(_getch())));
          if (c == 'r')
          {
            shouldRestart = true;
            break;
          }
          if (c == 'q')
          {
            /* clear terminal */
            std::cout << "\033[2J\033[H";
            return 0;
          }
        }

        /* exit inner game loop */
        break;
      }
      else
      {
        /* draw grid */
        std::cout << drawGrid(snake_head_grid_location_x, snake_head_grid_location_y, tailLength) << std::flush;
        /* move cursor back up for next frame */
        static const std::string CURSOR_UP = "\033[" + std::to_string(height + 5) + "A";
        std::cout << CURSOR_UP;
      }
    }

    /* clear screen before restarting */
    if (shouldRestart)
    {
      std::cout << "\033[2J\033[H";
    }
  }

  return 0;
}
