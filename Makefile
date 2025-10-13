

# Build optimized release versions (for distribution)
#make

# Build debug versions (for development)
#make debug

# Run debug version with sanitizers
#./build/Fractal-linux-debug

# Clean only debug builds
#make clean-debug

# Clean only release builds  
#make clean-release

# Clean everything
#make clean


# Nom des exécutables
VERSION = v1.24
LINUX_OUTPUT = build/Fractal-linux-x86_64-$(VERSION)
WIN_OUTPUT = build/Fractal-win64-$(VERSION).exe
LINUX_DEBUG = build/Fractal-linux-debug
WIN_DEBUG = build/Fractal-win64-debug.exe

# Cibles
all: linux windows

linux: $(LINUX_OUTPUT)
windows: $(WIN_OUTPUT)
debug: $(LINUX_DEBUG) $(WIN_DEBUG)

# Dossiers contenant les sources et les en-têtes
SRCDIR = src
OBJDIR = obj
OBJDEBUG = obj_debug
INCDIR = include

ICON_RES = icon.res
TARGET = $(LINUX_OUTPUT) $(WIN_OUTPUT) $(ICON_RES)

# Fichiers source
SRCS = $(wildcard $(SRCDIR)/*.c)

# Fichiers objets
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
WIN_OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.win.o,$(SRCS))
DEBUG_OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDEBUG)/%.o,$(SRCS))
WIN_DEBUG_OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDEBUG)/%.win.o,$(SRCS))

# Compilateur Linux
CC = gcc
BASE_CFLAGS = -I$(INCDIR) -I./libs/SDL2-linux/include -Wall -Winline
BASE_LDFLAGS = -L./libs/SDL2-linux/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lmpfr -lgmp -lm

# Flags de release (optimisé)
RELEASE_CFLAGS = -O3 -march=native -ffast-math -flto -DNDEBUG
RELEASE_LDFLAGS = -flto -s -Wl,--gc-sections -fno-ident

# Flags de debug
DEBUG_CFLAGS = -O0 -g3 -DDEBUG -fsanitize=address -fsanitize=undefined
DEBUG_LDFLAGS = -fsanitize=address -fsanitize=undefined

# Flags finaux
CFLAGS = $(BASE_CFLAGS) $(RELEASE_CFLAGS)
LDFLAGS = $(BASE_LDFLAGS) $(RELEASE_LDFLAGS)

DEBUG_CFLAGS_FULL = $(BASE_CFLAGS) $(DEBUG_CFLAGS)
DEBUG_LDFLAGS_FULL = $(BASE_LDFLAGS) $(DEBUG_LDFLAGS)

# Compilation de l'icone
$(ICON_RES): icon.rc assets/icon.ico
	x86_64-w64-mingw32-windres icon.rc -O coff -o $(ICON_RES)

# Compilateur Windows (cross-compilation)
WIN_CC = x86_64-w64-mingw32-gcc
WIN_BASE_CFLAGS = -I$(INCDIR) -I./libs/SDL2-win/include -I./libs/SDL2-win/include/SDL2
WIN_BASE_LDFLAGS = -L./libs/SDL2-win/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lm -static \
                   -lsetupapi -lole32 -lcomdlg32 -limm32 -lversion -lwinmm -lgdi32 \
                   -ldinput8 -luser32 -ladvapi32 -lshell32 -loleaut32 -lrpcrt4 -mwindows

# Flags Windows release
WIN_RELEASE_CFLAGS = -O3 -ffast-math -flto -DNDEBUG
WIN_RELEASE_LDFLAGS = -flto -s -Wl,--gc-sections -fno-ident

# Flags Windows debug
WIN_DEBUG_CFLAGS = -O0 -g -DDEBUG
WIN_DEBUG_LDFLAGS = 

# Flags Windows finaux
WIN_CFLAGS = $(WIN_BASE_CFLAGS) $(WIN_RELEASE_CFLAGS)
WIN_LDFLAGS = $(WIN_BASE_LDFLAGS) $(WIN_RELEASE_LDFLAGS)

WIN_DEBUG_CFLAGS_FULL = $(WIN_BASE_CFLAGS) $(WIN_DEBUG_CFLAGS)
WIN_DEBUG_LDFLAGS_FULL = $(WIN_BASE_LDFLAGS) $(WIN_DEBUG_LDFLAGS)

# Cibles release
$(LINUX_OUTPUT): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@
	
$(WIN_OUTPUT): $(WIN_OBJS) $(ICON_RES)
	$(WIN_CC) $^ $(WIN_LDFLAGS) -o $@

# Cibles debug
$(LINUX_DEBUG): $(DEBUG_OBJS)
	$(CC) $^ $(DEBUG_LDFLAGS_FULL) -o $@

$(WIN_DEBUG): $(WIN_DEBUG_OBJS) $(ICON_RES)
	$(WIN_CC) $^ $(WIN_DEBUG_LDFLAGS_FULL) -o $@

# Compilation Linux release
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) -c $< $(CFLAGS) -o $@

# Compilation Windows release
$(OBJDIR)/%.win.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(WIN_CC) -c $< $(WIN_CFLAGS) -o $@

# Compilation Linux debug
$(OBJDEBUG)/%.o: $(SRCDIR)/%.c | $(OBJDEBUG)
	$(CC) -c $< $(DEBUG_CFLAGS_FULL) -o $@

# Compilation Windows debug
$(OBJDEBUG)/%.win.o: $(SRCDIR)/%.c | $(OBJDEBUG)
	$(WIN_CC) -c $< $(WIN_DEBUG_CFLAGS_FULL) -o $@

# Dossiers objets
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDEBUG):
	mkdir -p $(OBJDEBUG)

# Nettoyage
clean:
	rm -rf $(OBJDIR) $(OBJDEBUG) $(TARGET) $(LINUX_DEBUG) $(WIN_DEBUG)

clean-release:
	rm -rf $(OBJDIR) $(TARGET)

clean-debug:
	rm -rf $(OBJDEBUG) $(LINUX_DEBUG) $(WIN_DEBUG)

# Installation
install-icon-linux:
	sed "s|__DIR__|$(realpath .)|g" dist/Fractal.desktop | sed "s|__EXEC__|$(LINUX_OUTPUT)|g" > /tmp/fractal.Desktop
	sudo cp /tmp/fractal.Desktop /usr/share/applications/fractal.desktop

.PHONY: all linux windows debug clean clean-release clean-debug install-icon-linux
