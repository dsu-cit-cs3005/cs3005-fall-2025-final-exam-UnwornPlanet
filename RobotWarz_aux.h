#ifndef _ROBOTWARZ_H_
#define _ROBOTWARZ_H_
#include "Arena.h"
#include "RobotBase.h"
#include <map>
#include <string>
struct GameSetup
{
    int height;
    int width;
    int numObstacles;
    int maxRounds;
    bool watchLive;
};
GameSetup promptGameSetup();
std::map<std::string, RobotBase*> loadRobotsFromDirectory(const std::string& directory);
Arena buildArena(const GameSetup& setup, std::map<std::string, RobotBase*>& robots);
void runGame(Arena& arena, const std::map<std::string, RobotBase*>& robots, int maxRounds, bool watchLive);
void runInteractiveGame();
#endif
