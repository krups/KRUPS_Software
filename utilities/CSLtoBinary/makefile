YELLOW = \033[1;33m
GREEN = \033[0;32m
RED = \033[0;31m
NC = \033[0m

SOURCES = main.cpp

OBJECTS = $(SOURCES:.cpp=.o)
SOURCEDIR = ./src
OBJECTDIR = ./objects
OUTPUTDIR = ./bin
PROGRAMNAME = CSLtoBin
OBJECT = $(addprefix $(OBJECTDIR)/,$(OBJECTS))

$(PROGRAMNAME):$(OBJECT)
	@echo
	@echo "$(YELLOW)compliling program $@...$(NC)"
	g++ -std=c++11 -g -o $(OUTPUTDIR)/$@ $^
	@echo  "$(GREEN)Success$(NC)"

$(OBJECTDIR)/%.o : $(SOURCEDIR)/%.cpp $(SOURCEDIR)/%.h
	@echo
	@echo "$(YELLOW)compliling $@ with header...$(NC)"
	g++ -std=c++11 -g -c -o $@ $<
	@echo "$(GREEN)Success$(NC)"

$(OBJECTDIR)/%.o : $(SOURCEDIR)/%.cpp 
	@echo
	@echo "$(YELLOW)compiling $@...$(NC)"
	g++ -std=c++11 -g -c -o $@ $^
	@echo "$(GREEN)Success$(NC)"

clean:
	@echo
	@echo "$(RED)Cleaning up..$(NC)"
	@rm -f $(OBJECTDIR)/*.o
	@echo "$(GREEN)Success$(NC)"
