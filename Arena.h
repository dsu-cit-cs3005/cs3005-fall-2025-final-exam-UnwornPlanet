#ifndef _ARENA_H_
#define _ARENA_H_
#include <vector>
#include <string>
#include "RobotBase.h"
#include "RadarObj.h"
#include <utility>
#include <iostream>
#include <map>
#include <memory>
class Arena {
	public:
		Arena(int height, int width, std::map<std::string, RobotBase*> robots, int num_of_obstacles);
		void get_radar_results(RobotBase* robot, int radar_dir, std::vector<RadarObj>& radar_results);
		void handle_shot(WeaponType weapon, RobotBase* robot, int shot_row, int shot_col);
		void handle_movement(const std::string& name, RobotBase* robot, int direction, int distance);
		void applyDamageToCell(int row, int col, int minDmg, int maxDmg);
		std::vector<std::pair<int, int>> radarPath(int sx, int sy, int direction) const;
		std::vector<std::pair<int, int>> railgunPath(int sx, int sy, int tx, int ty);
		std::vector<std::pair<int, int>> flamePath(int sx, int sy, int tx, int ty);
		std::vector<std::pair<int, int>> grenadeRadius(int x, int y);
		void printState(std::ostream& os);
		std::string getWinner();
		int getAlive();
		void placeItems();
		void iterate();
	protected:
		std::vector<std::vector<std::string>> mGrid;
		int mHeight;
		int mWidth;
		std::map<std::string, RobotBase*> mRobots;
		int mObstacles;
		int mAlive;

};
#endif
