CC := clang++
CFLAGS := -O0 -g -std=c++11 -Isrc/ -I../unicorn/include
LDFLAGS := -g
LDLIBS  := -lunicorn -lpthread -L../unicorn/build
TARGET := vios

BINEXT := 

BUILDDIR := bin
SOURCEDIR := src
OBJDIR := obj

include src/makefile

OBJS := $(addprefix $(OBJDIR)/, $(OBJS))

ifeq ($(OS),Windows_NT)
	
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		
    endif
    ifeq ($(UNAME_S),Darwin)
		CFLAGS += -I/opt/homebrew/include
		LDFLAGS += -L/opt/homebrew/lib
    endif
endif

# The main target
$(BUILDDIR)/$(TARGET)$(BINEXT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

clean:
	rm -f $(OBJS) $(DEPS) $(BUILDDIR)/$(TARGET)$(BINEXT) 

# makes sure the target dir exists
MKDIR = if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi

$(OBJDIR)/%.o: $(SOURCEDIR)/%.c
	@$(MKDIR)
	@echo Compiling $<
	@$(CC) $(CFLAGS) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

$(OBJDIR)/%.o: $(SOURCEDIR)/%.cpp
	@$(MKDIR)
	@echo Compiling $<
	@$(CC) $(CFLAGS) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@