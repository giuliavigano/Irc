# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: mchiaram <mchiaram@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/10/09 14:06:35 by mchiaram          #+#    #+#              #
#    Updated: 2025/11/20 14:06:09 by mchiaram         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME		= ircserv
CXX			= c++
CXXFLAGS	= -Wextra -Werror -Wall -std=c++98
SRC_DIR 	= src
HPP_DIR		= hpp
BUILD_DIR 	= .build
DIR_DUP 	= @mkdir -p $(@D)

SRCS		=	$(SRC_DIR)/main.cpp \
				$(SRC_DIR)/Channel.cpp \
				$(SRC_DIR)/Client.cpp \
				$(SRC_DIR)/Server.cpp \
				$(SRC_DIR)/InputParse.cpp \
				$(SRC_DIR)/HandleBuffer.cpp 

HEADERS		=	$(HPP_DIR)/Channel.hpp \
				$(HPP_DIR)/Client.hpp \
				$(HPP_DIR)/Server.hpp \
				$(HPP_DIR)/InputParse.hpp \
				$(HPP_DIR)/HandleBuffer.hpp \
				$(HPP_DIR)/Replies.hpp

OBJS 		= $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

all: header $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS) && echo "${BOLD}${GREEN}Done!${NO_COLOR}" \
	|| echo "${BOLD}${RED}Error compiling $(C_NAME)${NO_COLOR}";

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	$(DIR_DUP)
	$(CXX) $(CXXFLAGS) -I$(HPP_DIR) -c $< -o $@

header: 
	${info }
	${info ${BOLD}Creating -> ${YELLOW}$(NAME)${NO_COLOR}}
	@if $(MAKE) -q $(NAME) ; then \
		echo "${BOLD}${YELLOW}No changes detected, not rebuilding $(NAME)${NO_COLOR}"; \
	fi

clean:
	@if [ -d "$(BUILD_DIR)" ]; then \
		rm -rf $(BUILD_DIR) && \
		echo "${BOLD}${GREEN}Deleted build files${NO_COLOR}"; \
	else \
		echo "${BOLD}${YELLOW}No build files to delete${NO_COLOR}"; \
	fi;

fclean: clean
	@if [ -f "$(NAME)" ]; then \
		rm -f $(NAME) && \
		echo "${BOLD}${GREEN}Deleted binary file: $(NAME)${NO_COLOR}"; \
	else \
		echo "${BOLD}${YELLOW}No binary file to delete: $(NAME)${NO_COLOR}"; \
	fi
	@for file in $(TESTER_FILES); do \
		if [ -f "$$file" ]; then \
			rm -f $$file && \
			echo "${BOLD}${GREEN}Deleted $$file${NO_COLOR}";\
		fi; \
	done; \
# 	if [ -f $(TESTER_FILES) ]; then \
# 		echo && \
# 		echo "${BOLD}${YELLOW}[Tester]${NO_COLOR}" && \
# 		rm -f $(TESTER_FILES) && \
# 		echo "${BOLD}${GREEN}Deleted $(TESTER_FILES)${NO_COLOR}"; \
# 	fi;

re: fclean all

TESTER_FILES	:=
TESTER_DIR_URL	:=

test:
	@if [ "$(TESTER_DIR_URL)" ]; then \
		echo "${BOLD}${YELLOW}Downloading test files...${NO_COLOR}"; \
		for file in $(TESTER_FILES); do \
			if [ ! -f "$$file" ]; then \
				curl -sSfL $(TESTER_DIR_URL)/$$file -o $$file && \
				if [ -f "$$file" ]; then \
					chmod +x $$file && \
					echo "${BOLD}${GREEN}Downloaded $$file${NO_COLOR}"; \
				else \
					echo "${BOLD}${RED}Not a valid URL: ${NO_COLOR}$(TESTER_DIR_URL)"; \
				fi; \
			else \
				echo "${BOLD}${YELLOW}$$file already exists${NO_COLOR}"; \
			fi; \
		done; \
	else \
		echo "${BOLD}${RED}missing URL${NO_COLOR}"; \
	fi;

.PHONY: all re clean fclean header test
.SILENT:

YELLOW		:= ${shell tput setaf 3}
GREEN		:= ${shell tput setaf 2}
RED			:= ${shell tput setaf 1}
NO_COLOR	:= ${shell tput sgr0}
BOLD		:= ${shell tput bold}