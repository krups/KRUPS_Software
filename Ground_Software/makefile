YELLOW = \033[1;33m
GREEN = \033[1;32m
RED = \033[1;31m
NC = \033[0m

SOURCES = depack.c depacks.c

OBJECTS = $(SOURCES:.c=.o)
SOURCEDIR = ./src
OBJECTDIR = ./objects
OUTPUTDIR = ./bin
PROGRAMNAME = Ground_Software
OBJECT = $(addprefix $(OBJECTDIR)/,$(OBJECTS))

$(PROGRAMNAME):$(OBJECT) $(SOURCEDIR)/main.cpp $(SOURCEDIR)/quicksort.cpp
	@echo
	@echo "$(YELLOW)compliling program $@...$(NC)"
	g++ -std=c++11 -fno-stack-protector -g -o $(OUTPUTDIR)/$@ 	$^
	@echo  "$(GREEN)Success$(NC)"

$(OBJECTDIR)/%.o : $(SOURCEDIR)/%.c $(SOURCEDIR)/brieflzDecompress.h
	@echo
	@echo "$(YELLOW)compliling $@ with header...$(NC)"
	g++ -std=c++11 -fno-stack-protector -g -c -o $@ $<
	@echo "$(GREEN)Success$(NC)"

$(OBJECTDIR)/%.o : $(SOURCEDIR)/%.cpp 
	@echo
	@echo "$(YELLOW)compiling $@...$(NC)"
	g++ -std=c++11 -fno-stack-protector -g -c -o $@ $^
	@echo "$(GREEN)Success$(NC)"

clean:
	@echo
	@echo "$(RED)Cleaning up..$(NC)"
	@rm -f $(OBJECTDIR)/*.o
	@echo "$(GREEN)Success$(NC)"
