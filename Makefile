NAME        = taskmaster
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -g -I.
LDFLAGS     = -ljsoncpp

SRC_DIR     = src
TASK_DIR    = $(SRC_DIR)/task
OBJ_DIR     = obj

SRC = \
	src/bonus/client_ctl/server.cpp \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/taskmaster.cpp \
	$(SRC_DIR)/colors.cpp \
	$(TASK_DIR)/log.cpp \
	$(TASK_DIR)/pid.cpp \
	$(TASK_DIR)/prog.cpp \
	$(TASK_DIR)/reload.cpp \
	$(TASK_DIR)/shell.cpp \
	$(TASK_DIR)/status.cpp \
	$(TASK_DIR)/supervision.cpp

OBJ = $(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME) client

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME) $(LDFLAGS)

client:
	$(MAKE) -C src/bonus/client_ctl

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/$(SRC_DIR)
	@mkdir -p $(OBJ_DIR)/$(SRC_DIR)/task
	@mkdir -p $(OBJ_DIR)/$(SRC_DIR)/bonus/client_ctl
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	$(MAKE) -C src/bonus/client_ctl clean

fclean: clean
	rm -f $(NAME)
	$(MAKE) -C src/bonus/client_ctl fclean

re: fclean all

.PHONY: all clean fclean re
