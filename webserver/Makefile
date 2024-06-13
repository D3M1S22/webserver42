# Compiler
CC = c++

# Compiler flags
CFLAGS = -Wall -Wextra -Werror -std=c++98

# Source files
SRCS = ./src/Config.cpp ./src/Server.cpp ./src/Location.cpp ./src/ARules.cpp ./src/Error.cpp ./src/RequestHandler.cpp ./src/ServerManager.cpp ./main.cpp
# Header files
HDRS = ./includes/*.hpp

# Build directory
BUILD_DIR = ./build

# Object files
OBJS = $(patsubst ./src/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Executable name
EXEC = webserver

# Build rule
all: $(EXEC)

# Target for executable
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Target for object files
$(BUILD_DIR)/%.o: ./src/%.cpp $(HDRS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -rf $(BUILD_DIR)/

fclean: clean
	rm -rf $(EXEC)

re: fclean all

.PHONY: all re clean fclean
