#include "RobotWarz_aux.h"
#include <iostream>
#include <limits>
#include <thread>
#include <chrono>
#include <filesystem>
#include <cstdlib>
#include <dlfcn.h>
GameSetup promptGameSetup()
{
    GameSetup setup;

    // ---- ARENA HEIGHT ----
    while (true)
    {
        std::cout << "Enter arena height (minimum 10): ";
        std::cin >> setup.height;

        if (std::cin.fail() || setup.height < 10)
        {
            std::cout << "Invalid height. Must be an integer >= 10.\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        else
        {
            break;
        }
    }

    // ---- ARENA WIDTH ----
    while (true)
    {
        std::cout << "Enter arena width (minimum 10): ";
        std::cin >> setup.width;

        if (std::cin.fail() || setup.width < 10)
        {
            std::cout << "Invalid width. Must be an integer >= 10.\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        else
        {
            break;
        }
    }

    int maxCells = setup.height * setup.width;

    // ---- NUMBER OF OBSTACLES ----
    while (true)
    {
        std::cout << "Enter number of obstacles: ";
        std::cin >> setup.numObstacles;

        if (std::cin.fail() || setup.numObstacles < 0 || setup.numObstacles >= maxCells)
        {
            std::cout << "Invalid obstacle count. Must be between 0 and "
                      << (maxCells - 1) << ".\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        else
        {
            break;
        }
    }

    // ---- MAX ROUNDS ----
    while (true)
    {
        std::cout << "Enter maximum number of rounds: ";
        std::cin >> setup.maxRounds;

        if (std::cin.fail() || setup.maxRounds <= 0)
        {
            std::cout << "Invalid round count. Must be a positive integer.\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        else
        {
            break;
        }
    }

    // ---- WATCH LIVE MODE ----
    while (true)
    {
        char choice;
        std::cout << "Watch game live? (y/n): ";
        std::cin >> choice;

        if (choice == 'y' || choice == 'Y')
        {
            setup.watchLive = true;
            break;
        }
        else if (choice == 'n' || choice == 'N')
        {
            setup.watchLive = false;
            break;
        }
        else
        {
            std::cout << "Invalid input. Enter 'y' or 'n'.\n";
        }
    }

    return setup;
}

Arena buildArena(const GameSetup& setup,
                 std::map<std::string, RobotBase*>& robots)
{
    return Arena(setup.height,
                 setup.width,
                 robots,
                 setup.numObstacles);
}
void runGame(Arena& arena,
             const std::map<std::string, RobotBase*>& robots,
             int maxRounds,
             bool watchLive)
{
    int round = 1;

    while (round <= maxRounds && arena.getAlive() > 1)
    {
        // ---- LIVE MODE OUTPUT ----
        if (watchLive)
        {
            std::cout << "=========== starting round " << round
                      << " ===========\n\n";

            arena.printState(std::cout);
            std::cout << "\n";

            // Print robot stats
            for (const auto& [name, robot] : robots)
            {
                if (!robot) continue;

                if (robot->get_health() <= 0)
                {
                    std::cout << robot->print_stats()
                              << " - is out\n\n";
                }
                else
                {
                    std::cout << robot->print_stats()
                              << "\n\n";
                }
            }
        }

        // ---- RUN ONE FULL ROUND ----
        arena.iterate();

        // ---- LIVE MODE DELAY ----
        if (watchLive)
        {
            std::this_thread::sleep_for(
                std::chrono::seconds(1));
        }

        round++;
    }

    // ---- ALWAYS PRINT FINAL RESULT ----
    std::cout << "=========== game over ===========\n\n";
    arena.printState(std::cout);
    std::cout << "\nWinner: " << arena.getWinner() << "\n";
}
std::map<std::string, RobotBase*> loadRobotsFromDirectory(const std::string& directory)
{
    std::map<std::string, RobotBase*> robots;

    // Characters used after 'R' for unique robot IDs
    const std::string symbols = "@#$%&!*+=<>?";
    int symbolIndex = 0;

    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        std::string filename = entry.path().filename().string();

        // ---- ONLY LOAD FILES NAMED Robot_*.cpp ----
        if (filename.rfind("Robot_", 0) != 0 || filename.find(".cpp") == std::string::npos)
            continue;

        // ---- BUILD SHARED LIBRARY NAME ----
        std::string baseName = filename.substr(0, filename.find(".cpp"));
        std::string sharedLib = baseName + ".so";

        // ---- COMPILE INTO SHARED OBJECT ----
        std::string compile_cmd =
            "g++ -shared -fPIC -o " + sharedLib + " " +
            filename + " RobotBase_pic.o -I. -std=c++20";

        std::cout << "Compiling " << filename << " -> " << sharedLib << "\n";

        int compile_result = std::system(compile_cmd.c_str());
        if (compile_result != 0)
        {
            std::cerr << "ERROR: Failed to compile " << filename << "\n";
            continue;
        }

        // ---- LOAD SHARED LIBRARY ----
	std::string soPath = "./" + sharedLib;
	void* handle = dlopen(soPath.c_str(), RTLD_LAZY);
        if (!handle)
        {
            std::cerr << "ERROR: Failed to load " << sharedLib
                      << ": " << dlerror() << "\n";
            continue;
        }

        // ---- GET FACTORY FUNCTION (create_robot) ----
        dlerror(); // Clear old errors
        RobotFactory create_robot =
            (RobotFactory)dlsym(handle, "create_robot");

        const char* error = dlerror();
        if (error)
        {
            std::cerr << "ERROR: Failed to find create_robot in "
                      << sharedLib << ": " << error << "\n";
            dlclose(handle);
            continue;
        }

        // ---- CREATE THE ROBOT ----
        RobotBase* robot = create_robot();
        if (!robot)
        {
            std::cerr << "ERROR: create_robot() returned null for "
                      << sharedLib << "\n";
            dlclose(handle);
            continue;
        }

        // ---- ASSIGN UNIQUE MAP KEY ("R@", "R#", ...) ----
        if (symbolIndex >= (int)symbols.size())
        {
            std::cerr << "ERROR: Too many robots for available symbols!\n";
            break;
        }

        std::string robotKey = "R";
        robotKey += symbols[symbolIndex++];
	
	robot->m_name = baseName;          // gives it a real name
	robot->m_character = robotKey[1];

        robots[robotKey] = robot;

        std::cout << "Loaded robot: " << robotKey << "\n";
    }

    // ---- SAFETY CHECK: NEED AT LEAST TWO ROBOTS ----
    if (robots.size() < 2)
    {
        std::cerr << "ERROR: Need at least two robots to play!\n";
        std::exit(1);
    }

    return robots;
}
void runInteractiveGame()
{
    GameSetup setup = promptGameSetup();

    auto robots = loadRobotsFromDirectory(".");

    Arena arena = buildArena(setup, robots);

    runGame(arena, robots,
            setup.maxRounds,
            setup.watchLive);
}

