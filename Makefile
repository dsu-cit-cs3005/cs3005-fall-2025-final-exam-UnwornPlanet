# Compiler
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic

# Executable
TARGET = RobotWarz

# Source files
SRCS = Arena.cpp RobotWarz_aux.cpp RobotWarz.cpp

# Object files
OBJS = Arena.o RobotWarz_aux.o RobotWarz.o

# Default target
all: $(TARGET)

# Compile Arena
Arena.o: Arena.cpp Arena.h RobotBase.h
	$(CXX) $(CXXFLAGS) -c Arena.cpp

# Compile RobotWarz auxiliary
RobotWarz_aux.o: RobotWarz_aux.cpp RobotWarz_aux.h Arena.h RobotBase.h
	$(CXX) $(CXXFLAGS) -c RobotWarz_aux.cpp

# Compile main
RobotWarz.o: RobotWarz.cpp RobotWarz_aux.h
	$(CXX) $(CXXFLAGS) -c RobotWarz.cpp

# Link final executable (RobotBase.o already exists)
$(TARGET): $(OBJS) RobotBase.o
	$(CXX) $(CXXFLAGS) $(OBJS) RobotBase.o -ldl -o $(TARGET)

# Clean build artifacts
clean:
	rm -f *.o *.so $(TARGET)
