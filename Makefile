NAME        = taskmaster
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror  -I.
LDFLAGS     = -ljsoncpp
SRC_DIR     = src
TASK_DIR    = $(SRC_DIR)/task
OBJ_DIR     = obj

SRC         = $(SRC_DIR)/main.cpp \
			$(SRC_DIR)/taskmaster.cpp \
			$(SRC_DIR)/colors.cpp \
			$(TASK_DIR)/log.cpp \
			$(TASK_DIR)/pid.cpp \
			$(TASK_DIR)/prog.cpp \
			$(TASK_DIR)/reload.cpp \
			$(TASK_DIR)/shell.cpp \
			$(TASK_DIR)/status.cpp \
			$(TASK_DIR)/supervision.cpp

OBJ 	= $(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(OBJ) -o $(NAME) $(LDFLAGS)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/$(SRC_DIR)
	@mkdir -p $(OBJ_DIR)/$(SRC_DIR)/task
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
