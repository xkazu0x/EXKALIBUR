CC := g++
CFLAGS := -std=c++17 -Wall -Wextra -Wno-cast-function-type -Wno-unused-parameter
INCLUDES := -I.
LIBS := -lkernel32 -luser32
DEFINES := -D_DEBUG

MKDIR := mkdir
RMDIR := rmdir /s /q

SRC_DIR := src
OBJ_DIR := obj
BUILD_DIR := bin

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
EXEC := excalibur.exe

all: $(BUILD_DIR)/$(EXEC)

$(BUILD_DIR)/$(EXEC): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $< $(DEFINES) $(INCLUDES)

$(BUILD_DIR) $(OBJ_DIR):
	$(MKDIR) $@

run: all
	$(BUILD_DIR)/$(EXEC)

clean:
	$(RMDIR) $(BUILD_DIR) $(OBJ_DIR)
