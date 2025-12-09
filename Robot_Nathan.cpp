#include "RobotBase.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>

class Robot_Nathan : public RobotBase
{
private:
    int target_row = -1;
    int target_col = -1;

    std::vector<RadarObj> hazards;

    // Euclidean distance
    double dist2(int r1, int c1, int r2, int c2) const
    {
        int dr = r1 - r2;
        int dc = c1 - c2;
        return std::sqrt(dr * dr + dc * dc);
    }

    bool is_hazard(int row, int col) const
    {
        for (const auto& h : hazards)
        {
            if (h.m_row == row && h.m_col == col &&
               (h.m_type == 'M' || h.m_type == 'P' || h.m_type == 'F'))
            {
                return true;
            }
        }
        return false;
    }

    void remember_hazard(const RadarObj& obj)
    {
        if (obj.m_type != 'M' && obj.m_type != 'P' && obj.m_type != 'F')
            return;

        for (const auto& h : hazards)
        {
            if (h.m_row == obj.m_row && h.m_col == obj.m_col)
                return; // already remembered
        }
        hazards.push_back(obj);
    }

    int radar_sweep_dir = 1; // 1..8

public:
    // Move 4, Armor 3, Railgun
    Robot_Nathan() : RobotBase(4, 3, railgun)
    {
        m_name = "Nathan";
    }

    // -------- RADAR --------
    virtual void get_radar_direction(int& radar_direction) override
    {
        // Systematic sweep around the robot
        radar_direction = radar_sweep_dir;
        radar_sweep_dir++;
        if (radar_sweep_dir > 8)
            radar_sweep_dir = 1;
    }

    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override
    {
        target_row = -1;
        target_col = -1;

        int my_r, my_c;
        get_current_location(my_r, my_c);

        double bestDist = 1e9;

        for (const auto& obj : radar_results)
        {
            remember_hazard(obj);

            if (obj.m_type == 'R')
            {
                double d = dist2(my_r, my_c, obj.m_row, obj.m_col);
                if (d < bestDist)
                {
                    bestDist = d;
                    target_row = obj.m_row;
                    target_col = obj.m_col;
                }
            }
        }
    }

    // -------- SHOOTING --------
    virtual bool get_shot_location(int& shot_row, int& shot_col) override
    {
        if (target_row != -1 && target_col != -1)
        {
            shot_row = target_row;
            shot_col = target_col;
            return true;
        }
        return false;
    }

    // -------- MOVEMENT --------
    virtual void get_move_direction(int& direction, int& distance) override
    {
        int r, c;
        get_current_location(r, c);
        int maxMove = get_move_speed();

        // Helper lambda: check if step in direction d is in-bounds and not a known hazard
        auto ok_step = [&](int d) -> bool
        {
            int nr = r + directions[d].first;
            int nc = c + directions[d].second;

            if (nr < 0 || nr >= m_board_row_max ||
                nc < 0 || nc >= m_board_col_max)
                return false;

            if (is_hazard(nr, nc))
                return false;

            return true;
        };

        // 1) If we have a target, try to kite away (but don't get stuck at border)
        if (target_row != -1 && target_col != -1)
        {
            int bestDir = 0;
            double bestScore = -1e9;

            for (int d = 1; d <= 8; ++d)
            {
                int nr = r + directions[d].first;
                int nc = c + directions[d].second;

                if (nr < 0 || nr >= m_board_row_max ||
                    nc < 0 || nc >= m_board_col_max)
                    continue;

                if (is_hazard(nr, nc))
                    continue;

                double newDist = dist2(nr, nc, target_row, target_col);
                if (newDist > bestScore)
                {
                    bestScore = newDist;
                    bestDir = d;
                }
            }

            if (bestDir != 0)
            {
                direction = bestDir;
                distance = std::min(2, maxMove);  // small controlled kite steps
                return;
            }
            // If no safe kite direction, fall through to exploration
        }

        // 2) Exploration / no target:
        // Try all directions in a shuffled order, pick the first valid one
        int start = 1 + (rand() % 8);
        for (int k = 0; k < 8; ++k)
        {
            int d = 1 + ((start - 1 + k) % 8);
            if (ok_step(d))
            {
                direction = d;
                distance  = 1;   // cautious step, but keeps us from getting stuck
                return;
            }
        }

        // 3) If absolutely everything is blocked or unknown, still move
        //    so we *never* just sit forever:
        direction = 1 + (rand() % 8);
        distance  = 1;
    }
};

// Factory function
extern "C" RobotBase* create_robot()
{
    return new Robot_Nathan();
}

