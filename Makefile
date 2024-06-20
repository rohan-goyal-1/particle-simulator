EXE = particle_sim
CC = cc
BUILD_DIR = ./bin
SOURCE = ./src/*.c
DEFINES =
INCLUDES = -framework Cocoa -framework OpenGL -framework IOKit
LINKERS = -L ./src/vendors/GLFW/lib -lglfw3

particle_sim:
	$(CC) $(SOURCE) -o $(BUILD_DIR)/$(EXE) $(INCLUDES) $(LINKERS)

.PHONY: run all

run: 
	$(BUILD_DIR)/$(EXE)

all: 
	$(CC) $(SOURCE) -o $(BUILD_DIR)/$(EXE) $(INCLUDES) $(LINKERS)
	$(BUILD_DIR)/$(EXE)
