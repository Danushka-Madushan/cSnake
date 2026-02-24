#include <iostream>
#include <conio.h>
#include <windows.h>
#include <csignal>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <map>
#include <vector>

enum Directions
{
  Left,
  Right,
  Up,
  Down
};

enum Keys
{
  XAxis,
  YAxis
};

/* width must be a odd number */
static const int width = 61;
static const int height = 20;
static const bool use_two_jumps_for_x_axis = true;

int tailLength = 3;
std::vector<std::map<Keys, int>> tailMap;
std::vector<Directions> tailDirections;
std::vector<std::pair<std::map<Keys, int>, Directions>> turningPoints;
int snake_head_grid_location_x = ((width - 2) / 2) + 1;
int snake_head_grid_location_y = tailLength + 2;
Directions current_direction = Down;
std::pair<int, int> appleLocation;
std::pair<int, int> endOfTailLocation;
Directions endOfTailDirection;

std::string padWithZeros(int value, unsigned int width)
{
  std::ostringstream oss;
  oss << std::setw(width) << std::setfill('0') << value;
  return oss.str();
}

int getRandomPoint(int from, int to, bool isEven = false)
{
  /* Static ensures the engine is seeded only once for the lifetime of the program */
  static std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());

  std::uniform_int_distribution<> distrib(from, to);

  if (isEven)
  {
    int num;
    /* Keep generating until we hit an even number */
    do
    {
      num = distrib(gen);
    } while (num % 2 != 0);
    return num;
  }

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
    if (x_axis_location == snake_head_grid_location_x || y_axis_location == snake_head_grid_location_y)
    {
      continue;
    }

    for (int i = 0; i < tailMap.size(); i++)
    {
      auto tail = tailMap[i];
      if (tail.at(Keys::XAxis) == x_axis_location && tail.at(Keys::YAxis) == y_axis_location)
      {
        continue;
      }
    }
    appleLocation = {x_axis_location, y_axis_location};
    is_new_location_found = true;
    break;
  }
}

std::map<Keys, int> moveToDirection(int x, int y, Directions direction)
{
  std::map<Keys, int> directionMap;
  directionMap[Keys::XAxis] = x;
  directionMap[Keys::YAxis] = y;

  switch (direction)
  {
  case Directions::Up:
  {
    if (directionMap.at(Keys::YAxis) >= 1)
    {
      directionMap[Keys::YAxis]--;
    }
    break;
  }
  case Directions::Left:
  {
    if (directionMap.at(Keys::XAxis) > (use_two_jumps_for_x_axis ? 3 : 1))
    {
      directionMap[Keys::XAxis] -= use_two_jumps_for_x_axis ? 2 : 1;
    }
    break;
  }
  case Directions::Down:
  {
    if (directionMap.at(Keys::YAxis) < (height - 1))
    {
      directionMap[Keys::YAxis]++;
    }
    break;
  }
  case Directions::Right:
  {
    if (directionMap.at(Keys::XAxis) < (use_two_jumps_for_x_axis ? (width - 2) - 1 : (width - 2)))
    {
      directionMap[Keys::XAxis] += use_two_jumps_for_x_axis ? 2 : 1;
    }
    break;
  }
  }

  if (directionMap[Keys::XAxis] == appleLocation.first && directionMap[Keys::YAxis] == appleLocation.second)
  {
    /* append new tail */
    tailMap.push_back({{Keys::XAxis, endOfTailLocation.first}, {Keys::YAxis, endOfTailLocation.second}});
    tailDirections.push_back(endOfTailDirection);
    tailLength++;
    generateAppleLocation();
  }

  return directionMap;
}

void calculateTailMovingDirection()
{
  endOfTailLocation = {tailMap[tailLength - 1].at(Keys::XAxis), tailMap[tailLength - 1].at(Keys::YAxis)};
  endOfTailDirection = tailDirections[tailLength - 1];
  for (int i = 0; i < tailLength; i++)
  {
    auto tailPart = tailMap[i];
    for (int j = 0; j < turningPoints.size(); j++)
    {
      auto turningPoint = turningPoints[j];
      if (tailPart.at(Keys::XAxis) == turningPoint.first.at(Keys::XAxis) && tailPart.at(Keys::YAxis) == turningPoint.first.at(Keys::YAxis))
      {
        tailDirections[i] = turningPoint.second;
        if (i == tailLength - 1)
        {
          turningPoints.erase(turningPoints.begin() + j);
        }
        break;
      }
    }
  }
}

void addTurningPoint()
{
  turningPoints.push_back({{{Keys::XAxis, snake_head_grid_location_x}, {Keys::YAxis, snake_head_grid_location_y}}, current_direction});
}

void moveTail()
{
  for (int i = 0; i < tailLength; i++)
  {
    switch (tailDirections[i])
    {
    case Directions::Up:
    {
      tailMap[i].at(Keys::YAxis)--;
      break;
    }
    case Directions::Left:
    {
      tailMap[i].at(Keys::XAxis) -= use_two_jumps_for_x_axis ? 2 : 1;
      break;
    }
    case Directions::Down:
    {
      tailMap[i].at(Keys::YAxis)++;
      break;
    }
    case Directions::Right:
    {
      tailMap[i].at(Keys::XAxis) += use_two_jumps_for_x_axis ? 2 : 1;
    }
    }
  }
}

std::string drawGrid(int snake_head_x, int snake_head_y, int tail_length)
{
  std::string grid = "\r";
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
    grid += (char)186;
    for (int column = 0; column < (width - 2); column++)
    {
      if (row == snake_head_y && column == (snake_head_x - 1))
      {
        /* draw head */
        grid += "@";
      }
      else
      {
        bool isTailDraw = false;
        for (int i = 0; i < tail_length; i++)
        {
          std::map<Keys, int> tailPart = tailMap[i];
          if (row == tailPart.at(Keys::YAxis) && column == (tailPart.at(Keys::XAxis) - 1))
          {
            /* draw tail */
            grid += "*";
            isTailDraw = true;
            break;
          }
        }
        if (!isTailDraw)
        {
          if (appleLocation.first == (column + 1) && appleLocation.second == row)
          {
            /* draw apple */
            grid += (char)237;
          } else 
          {
            /* draw empty space */
            grid += " ";
          }
        }
      }
    }
    grid += (char)186;
    grid += "\n";
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

int main()
{
  /* register signal handler for SIGINT (ctrl+c) */
  signal(SIGINT, signalHandler);

  /* initialize tail */
  for (int i = 0; i < tailLength; i++)
  {
    int y_axis_location = ((snake_head_grid_location_y - 1) - i);
    tailMap.push_back({{Keys::XAxis, ((width - 2) / 2) + 1}, {Keys::YAxis, y_axis_location}});
    tailDirections.push_back(current_direction);
  }

  /* initiate apple */
  generateAppleLocation();

  /* clear terminal */
  std::cout << "\033[2J\033[H";

  while (true)
  {
    if (_kbhit())
    {
      char character = _getch();
      char movement_char = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

      if (movement_char == 'q')
      {
        std::cout << "\n * Qutting..." << std::endl;
        break;
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

    {
      /* refresh rate & game speed */
      Sleep(100);
      std::cout << " * Location : "
                << "[X=" << padWithZeros(snake_head_grid_location_x, 2)
                << ", Y=" << padWithZeros(snake_head_grid_location_y, 2) << "]"  << std::endl;
      std::cout << " * Apple    : "
                << "[X=" << padWithZeros(appleLocation.first, 2)
                << ", Y=" << padWithZeros(appleLocation.second, 2) << "]\n"  << std::endl;
      std::map<Keys, int> directionMap = moveToDirection(snake_head_grid_location_x, snake_head_grid_location_y, current_direction);
      snake_head_grid_location_x = directionMap.at(Keys::XAxis);
      snake_head_grid_location_y = directionMap.at(Keys::YAxis);
      calculateTailMovingDirection();
      moveTail();
      std::cout << drawGrid(snake_head_grid_location_x, snake_head_grid_location_y, tailLength) << std::flush;
      for (int i = 0; i < (height + 5); i++)
      {
        std::cout << "\033[A";
      }
    }
  }

  return 0;
}
