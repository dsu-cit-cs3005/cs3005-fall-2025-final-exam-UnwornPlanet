#ifndef TESTARENA_H
#define TESTARENA_H

#include "Arena.h"
#include "RobotBase.h"
#include <vector>
#include <string>
#include <iostream>

class TestArena {
public:
    void test_initialize_board();
    void test_handle_move();
    void test_handle_collision();
    void test_handle_shot_with_fake_radar();
    void test_robot_creation();
    void test_robot_with_all_weapons();
    void test_grenade_damage();
    void test_radar();
    void test_radar_local();
	void print_summary();

private:
	
	std::vector<std::string> test_log;

    bool print_test_result(const std::string& test_name, bool condition);
};

class TestRobot : public RobotBase {
public:
    int move_attempt = 0; // Tracks the current move cycle
    TestRobot(int move, int armor, WeaponType weapon, const std::string& name)
        : RobotBase(move, armor, weapon) {
        m_name = name;
    }

    void get_radar_direction(int& radar_direction) override {
        radar_direction = move_attempt % 9; // Cycle through valid and invalid directions
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        (void)radar_results; // Avoid unused parameter warning
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        int current_row, current_col;
        get_current_location(current_row, current_col);
        if (move_attempt % 2 == 0) {
            shot_row = -1; // Invalid negative
            shot_col = -1; // Invalid negative
        } else {
            shot_row = current_row + 20; // Outside arena bounds
            shot_col = current_col + 20; // Outside arena bounds
        }
        return true; // Always try to shoot
    }

    void get_move_direction(int& direction, int& distance) override {
        direction = move_attempt % 9; // Cycle through all directions
        distance = (move_attempt % 3 == 0) ? 7 : -1; // Invalid distances
        move_attempt++;
    }
};

class RobotOutOfBounds : public RobotBase {
public:
    int move_attempt = 0;

    RobotOutOfBounds()
        : RobotBase(2, 5, hammer) {
        m_name = "OutOfBoundsBot";
    }

    void get_move_direction(int& direction, int& distance) override {
        // Cycle through test cases
        switch (move_attempt++) {
            case 0: direction = 4; distance = 2; break;  // Valid move down and right
            case 1: direction = 3; distance = 2; break; // Out of bounds (right)
            case 2: direction = 5; distance = 2; break; // Out of bounds (down)
            case 3: direction = 1; distance = 2; break; // Out of bounds (up)
            case 4: direction = 7; distance = 2; break; // Out of bounds (left)
            default: direction = 0; distance = 0; break; // No movement
        }
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        (void)radar_results; // Do nothing
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        shot_row = shot_col = 0; // No shot
        return false;
    }

    void get_radar_direction(int& radar_direction) override {
        radar_direction = 0; // No radar use
    }
};

class BadMovesRobot : public RobotBase {
public:
    int move_attempt = 0;

    BadMovesRobot()
        : RobotBase(2, 5, grenade) {
        m_name = "BadMovesBot";
    }

    void get_move_direction(int& direction, int& distance) override {
        // Test invalid move scenarios
        switch (move_attempt++) {
            case 0: direction = 3; distance = 10; break; // Too far
            case 1: direction = 1; distance = -2; break; // Negative distance
            case 2: direction = 9; distance = 3; break;  // Invalid direction
            case 3: direction = 0; distance = 3; break;  // Direction 0
            default: direction = 0; distance = 0; break; // No movement
        }
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        (void)radar_results; // Do nothing
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        shot_row = shot_col = 0; // No shot
        return false;
    }

    void get_radar_direction(int& radar_direction) override {
        radar_direction = 0; // No radar use
    }
};

class JumperRobot : public RobotBase {
public:
    JumperRobot()
        : RobotBase(5, 2, flamethrower) {
        m_name = "JumperBot";
    }

    void get_move_direction(int& direction, int& distance) override {
        direction = 3; // Always move right
        distance = 5;  // Try to jump over an obstacle
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        (void)radar_results; // Do nothing
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        shot_row = shot_col = 0; // No shot
        return false;
    }

    void get_radar_direction(int& radar_direction) override {
        radar_direction = 0; // No radar use
    }
};

class ShooterRobot : public RobotBase {
public:
    ShooterRobot(WeaponType weapon, const std::string& name)
        : RobotBase(5, 3, weapon) {
        m_name = name;
    }

    void get_move_direction(int& direction, int& distance) override {
        direction = 0; // This robot doesn't move
        distance = 0;  // No movement
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        if (m_has_target) {
            shot_row = m_target_row;
            shot_col = m_target_col;
            return true;
        }
        return false;
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        if (!radar_results.empty()) {
            m_target_row = radar_results[0].m_row;
            m_target_col = radar_results[0].m_col;
            m_has_target = true;
        } else {
            m_has_target = false;
        }
    }

    void get_radar_direction(int& radar_direction) override {
        radar_direction = 0; // Local radar (not directional)
    }

private:
    int m_target_row = -1, m_target_col = -1;
    bool m_has_target = false;
};

#endif // TESTARENA_H
