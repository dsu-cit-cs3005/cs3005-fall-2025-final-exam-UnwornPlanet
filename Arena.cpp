#include "Arena.h"
#include "RobotBase.h"
#include <cstdlib>
#include "RadarObj.h"
#include <cmath>
Arena::Arena(int height, int width, std::map<std::string, RobotBase*> robots, int num_of_obstacles):
	mHeight(height),
	mWidth(width),
	mRobots(robots),
	mObstacles(num_of_obstacles),
	mAlive(static_cast<int>(robots.size())){
		mGrid.resize(height, std::vector<std::string>(width, "."));
		placeItems();
};
void Arena::placeItems(){
	std::vector<std::string> itemtypes={"P", "M", "F"};
	for(int i=0;i<mObstacles;i++){
		int row=rand() % (mHeight-2) +1;
		int col=rand() % (mWidth-2) +1;
		while(mGrid[row][col]!="."){
			row=rand() % (mHeight-2) +1;
                	col=rand() % (mWidth-2) +1;
		}
		int type=rand() % 3;
		mGrid[row][col]=itemtypes[type];
		
	}; 
	for(auto& [id, robot] : mRobots){
		int row=rand() % mHeight;
                int col=rand() % mWidth;
		while(mGrid[row][col]!="."){
			row=rand() % mHeight;
                        col=rand() % mWidth;
		}
		mGrid[row][col]=id;
		robot->move_to(row,col);
		robot->set_boundaries(mHeight,mWidth);
	};
};
int Arena::getAlive(){
	int result=0;
	for(auto& [name,robot] : mRobots){
		int health=robot->get_health();
		if(health>0){
			result+=1;
		};
	};
	mAlive=result;
	return mAlive;
};
std::string Arena::getWinner(){
	if(mAlive>1 || mAlive==0){
		return "none";
	};
	std::string result;
	for(auto& [name,robot] : mRobots){
		int health = robot->get_health();
		if(health>0){
			result= name;
		};
	};
	return result;
};
void Arena::printState(std::ostream& os){
	// ---- PRINT TOP COLUMN NUMBERS ----
    os << "    ";   // space for row numbers on the left

    for (int col = 0; col < mWidth; col++) {
        os << col << "  ";
    }
    os << "\n";

    // ---- PRINT EACH ROW WITH ROW NUMBER ----
    for (int row = 0; row < mHeight; row++) {

        // Print the row number on the left
        os << row << "  ";

        // Print the actual arena row
        for (int col = 0; col < mWidth; col++) {
            os << mGrid[row][col] << "  ";
        }

        os << "\n";
    }
	
};
std::vector<std::pair<int, int>> Arena::grenadeRadius(int x, int y){
	std::vector<std::pair<int, int>> coords;
	for(int dx = -1; dx <= 1; dx++){
        	for(int dy = -1; dy <= 1; dy++){

            		int nx = x + dx;
            		int ny = y + dy;

            // ✅ BOUNDS CHECK (critical)
            		if(nx >= 0 && nx < mHeight &&
               			ny >= 0 && ny < mWidth)
            		{
                		coords.push_back({nx, ny});
            		}
        	}
    	}
	return coords;
};
std::vector<std::pair<int, int>> Arena::flamePath(int sx, int sy, int tx, int ty){
	std::vector<std::pair<int,int>> coords;

    // ---- TRUE DIRECTION VECTOR ----
    double dx = tx - sx;
    double dy = ty - sy;

    double length = std::sqrt(dx*dx + dy*dy);

    // No direction = no flame
    if(length == 0.0)
        return coords;

    // ---- NORMALIZED FORWARD DIRECTION ----
    double dirX = dx / length;
    double dirY = dy / length;

    // ---- PERPENDICULAR DIRECTION FOR WIDTH ----
    double perpX = -dirY;
    double perpY =  dirX;

    // ---- STEP 4 UNITS FORWARD ----
    for(int step = 1; step <= 4; step++)
    {
        double centerX = sx + dirX * step;
        double centerY = sy + dirY * step;

        // ---- 3-WIDE STRIP ----
        for(int w = -1; w <= 1; w++)
        {
            double fx = centerX + perpX * w;
            double fy = centerY + perpY * w;

            int gridX = static_cast<int>(std::round(fx));
            int gridY = static_cast<int>(std::round(fy));

            // ---- BOUNDS CHECK ----
            if(gridX >= 0 && gridX < mHeight &&
               gridY >= 0 && gridY < mWidth)
            {
                coords.push_back({gridX, gridY});
            }
        }
    }

    return coords;
};
std::vector<std::pair<int, int>> Arena::railgunPath(int sx, int sy, int tx, int ty){
	std::vector<std::pair<int,int>> coords;

    double dx = tx - sx;
    double dy = ty - sy;

    double length = std::sqrt(dx*dx + dy*dy);

    // No direction = no shot
    if (length == 0.0)
        return coords;

    // ---- NORMALIZED DIRECTION ----
    double dirX = dx / length;
    double dirY = dy / length;

    // ---- CURRENT FLOAT POSITION ----
    double x = sx;
    double y = sy;

    // ---- STEP SIZE (small enough to not skip tiles) ----
    double stepSize = 0.25;

    // ---- MARCH UNTIL WE EXIT THE ARENA ----
    while (true)
    {
        x += dirX * stepSize;
        y += dirY * stepSize;

        int gridX = static_cast<int>(std::round(x));
        int gridY = static_cast<int>(std::round(y));

        // ---- STOP AT BOUNDARY ----
        if (gridX < 0 || gridX >= mHeight ||
            gridY < 0 || gridY >= mWidth)
        {
            break;
        }

        // ---- PREVENT DUPLICATE INSERTS ----
        if (coords.empty() || coords.back() != std::make_pair(gridX, gridY))
        {
            coords.push_back({gridX, gridY});
        }
    }

    return coords;
};
void Arena::handle_movement(const std::string& name,
                            RobotBase* robot,
                            int direction,
                            int distance)
{
	static std::map<std::string, bool> robotOnFlame;
    int r, c;
    robot->get_current_location(r, c);

    int dr = directions[direction].first;
    int dc = directions[direction].second;

    for (int step = 0; step < distance; ++step)
    {
        int nr = r + dr;
        int nc = c + dc;

        // ---- BOUNDS CHECK ----
        if (nr < 0 || nr >= mHeight || nc < 0 || nc >= mWidth)
            return;

        std::string& cell = mGrid[nr][nc];

        // ---- MOUND: BLOCK ONLY ----
        if (cell == "M")
            return;

        // ---- PIT: ENTER + DISABLE FOREVER ----
        if (cell == "P")
        {
            // Clear old position
            mGrid[r][c] = ".";

            // Mark robot as dead/trapped visually (X + symbol)
            std::string deadID = "X";
            deadID += name[1];   // carry over the robot's symbol

            mGrid[nr][nc] = deadID;

            robot->move_to(nr, nc);
            robot->disable_movement();
            return;
        }

        // ---- FLAMETHROWER: DAMAGE BUT DO NOT REMOVE ----
	bool steppingOnFlame = false;   // ✅ NEW

	if (cell == "F")
	{
    		int dmg = 30 + rand() % 21;
    		robot->take_damage(dmg);
    		steppingOnFlame = true;     // ✅ NEW
	}
        // ---- ROBOT COLLISION: BLOCK ----
        if (!cell.empty() && (cell[0] == 'R' || cell[0] == 'X'))
            return;

        // ---- APPLY MOVE ----
	if (robotOnFlame[name]){
    		mGrid[r][c] = "F";
	}else{
    		mGrid[r][c] = ".";
	};
        // Even if cell == "F", we visually place the robot there,
        // but the flame logically still exists under it.
        mGrid[nr][nc] = name;
	robotOnFlame[name] = steppingOnFlame;

        r = nr;
        c = nc;
        robot->move_to(r, c);
    }
}
void Arena::handle_shot(WeaponType weapon, RobotBase* robot, int shot_row, int shot_col){
	if (!robot) return;

    int sx, sy;
    robot->get_current_location(sx, sy);

    std::vector<std::pair<int,int>> coords;

    switch (weapon)
    {
        case railgun:
        {
            coords = railgunPath(sx, sy, shot_row, shot_col);

            for (auto& [r, c] : coords)
            {
                applyDamageToCell(r, c, 10, 20);
            }
            break;
        }

        case flamethrower:
        {
            coords = flamePath(sx, sy, shot_row, shot_col);

            for (auto& [r, c] : coords)
            {
                applyDamageToCell(r, c, 30, 50);
            }
            break;
        }

        case grenade:
        {
            // No grenades left = no shot
            if (robot->get_grenades() <= 0)
                return;

            robot->decrement_grenades();

            coords = grenadeRadius(shot_row, shot_col);

            for (auto& [r, c] : coords)
            {
                applyDamageToCell(r, c, 10, 40);
            }
            break;
        }

        case hammer:
        {
            // Hammer must hit a tile ADJACENT to the robot
            int dr = std::abs(shot_row - sx);
            int dc = std::abs(shot_col - sy);

            // Must be 1 tile away (not at same tile)
            if (dr > 1 || dc > 1 || (dr == 0 && dc == 0))
                return;

            // Bounds check
            if (shot_row < 0 || shot_row >= mHeight ||
                shot_col < 0 || shot_col >= mWidth)
                return;

            applyDamageToCell(shot_row, shot_col, 50, 60);
            break;
        }
    }	
};
void Arena::applyDamageToCell(int row, int col, int minDmg, int maxDmg)
{
	std::string& cell = mGrid[row][col];

    // Only robots can be damaged
    if (cell.empty() || cell[0] != 'R')
        return;

    // Look up robot by its grid token, e.g. "R@", "R$"
    auto it = mRobots.find(cell);
    if (it == mRobots.end() || !it->second)
        return;

    RobotBase* target = it->second;

    // ---- BASE DAMAGE ----
    int baseDamage = minDmg + (rand() % (maxDmg - minDmg + 1));

    int armor = target->get_armor();

    // ---- ARMOR REDUCTION ----
    double reductionFactor = 1.0 - (armor * 0.10);
    if (reductionFactor < 0.0) {
        reductionFactor = 0.0;
    }

    int finalDamage = static_cast<int>(baseDamage * reductionFactor);

    // ---- APPLY DAMAGE ----
    target->take_damage(finalDamage);

    // ---- ARMOR ALWAYS DROPS BY 1 ----
    target->reduce_armor(1);

    // ---- CHECK FOR DEATH ----
    if (target->get_health() <= 0) {
        // Robot is dead: disable movement and relabel on the grid.

        target->disable_movement();

        // Keep the special character but change the leading 'R' to 'X'
        // e.g. "R@" -> "X@", "R!" -> "X!".
        if (!cell.empty()) {
            cell[0] = 'X';
        }

        // From now on:
        // - this tile will NOT be treated as a robot (cell[0] != 'R')
        // - you still know *which* robot died by looking at "X@" etc
    }
};
std::vector<std::pair<int,int>> Arena::radarPath(int sx, int sy, int direction) const
{
    std::vector<std::pair<int,int>> coords;
	
    if (direction == 0)
    {
        for (int dr = -1; dr <= 1; dr++)
        {
            for (int dc = -1; dc <= 1; dc++)
            {
                // Skip the robot's own cell
                if (dr == 0 && dc == 0)
                    continue;

                int r = sx + dr;
                int c = sy + dc;

                // Bounds check
                if (r >= 0 && r < mHeight &&
                    c >= 0 && c < mWidth)
                {
                    coords.push_back({r, c});
                }
            }
        }

        return coords;  // ✅ IMPORTANT: stop here for direction 0
    }

    // Direction must be 1–8
    if (direction < 1 || direction > 8){
        return coords;
    };
    int stepR = directions[direction].first;
    int stepC = directions[direction].second;

    // Perpendicular direction for 3-wide sweep
    int perpR = -stepC;
    int perpC =  stepR;

    int curR = sx;
    int curC = sy;

    // ---- March forward until we exit the arena ----
    while (true)
    {
        curR += stepR;
        curC += stepC;

        // Stop when we leave the arena
        if (curR < 0 || curR >= mHeight ||
            curC < 0 || curC >= mWidth)
        {
            break;
        }

        // ---- 3-wide scan strip ----
        for (int w = -1; w <= 1; w++)
        {
            int rr = curR + perpR * w;
            int cc = curC + perpC * w;

            if (rr >= 0 && rr < mHeight &&
                cc >= 0 && cc < mWidth)
            {
                coords.push_back({rr, cc});
            }
        }
    }

    return coords;
};
void Arena::get_radar_results(RobotBase* robot, int radar_dir, std::vector<RadarObj>& radar_results)
{
    if (!robot) return;

    // Clear any previous scan data
    radar_results.clear();

    int sx, sy;
    robot->get_current_location(sx, sy);

    // Get all coordinates the radar touches
    std::vector<std::pair<int,int>> scanCoords =
        radarPath(sx, sy, radar_dir);

    // Scan each coordinate
    for (const auto& [r, c] : scanCoords)
    {
        const std::string& cell = mGrid[r][c];

        // Skip empty tiles
        if (cell == ".")
            continue;

        // First character defines RadarObj type
        char type = cell[0];  // 'R', 'X', 'M', 'F', 'P'

        // Only accept valid radar types
        if (type == 'R' || type == 'X' ||
            type == 'M' || type == 'F' || type == 'P')
        {
            radar_results.emplace_back(type, r, c);
        }
    }
}
void Arena::iterate()
{
    // Loop through each robot in the arena
    for (auto& [name, robot] : mRobots)
    {
        if (!robot)
            continue;

        // ---- SKIP DEAD ROBOTS ----
        if (robot->get_health() <= 0)
            continue;

        int sx, sy;
        robot->get_current_location(sx, sy);

        // ---- RADAR PHASE ----
        int radar_dir = 0;
        robot->get_radar_direction(radar_dir);   // ✅ reference output

        std::vector<RadarObj> radar_results;
        get_radar_results(robot, radar_dir, radar_results);

        robot->process_radar_results(radar_results);

        // ---- ACTION PHASE ----
        int shot_row = 0;
        int shot_col = 0;

        // If robot chooses to shoot
        if (robot->get_shot_location(shot_row, shot_col))   // ✅ bool return + ref outputs
        {
            WeaponType weapon = robot->get_weapon();
            handle_shot(weapon, robot, shot_row, shot_col);
        }
        else
        {
            // Otherwise the robot moves
            int move_dir = 0;
            int move_dist = 0;

            robot->get_move_direction(move_dir, move_dist);  // ✅ both by reference

            handle_movement(name, robot, move_dir, move_dist);
        }
    }
}
