
# Nom des exécutables
VERSION = v1.0
LINUX_OUTPUT = Fractal-linux-x86_64-$(VERSION)
WIN_OUTPUT = Fractal-win64-$(VERSION).exe

# Dossiers contenant les sources et les en-têtes
SRCDIR = src
OBJDIR = obj
INCDIR = include

# Fichiers source
SRCS = $(wildcard $(SRCDIR)/*.c)

# Fichiers objets
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
WIN_OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.win.o,$(SRCS))

# Compilateur Linux
CC = gcc
CFLAGS = -I$(INCDIR) -I./libs/SDL2-linux/include -L./libs/SDL2-linux/lib \
         -lSDL2 -lSDL2_image -lSDL2_ttf -g -Wall -lmpfr -lgmp -lm -Winline 

# Compilateur Windows (cross-compilation)
WIN_CC = x86_64-w64-mingw32-gcc
WIN_CFLAGS = -I$(INCDIR) -I./libs/SDL2-win/include -I./libs/SDL2-win/include/SDL2 -L./libs/SDL2-win/lib \
             -lSDL2 -lSDL2_image -lSDL2_ttf -lm -static \
             -lsetupapi -lole32 -lcomdlg32 -limm32 -lversion -lwinmm -lgdi32 -ldinput8 -luser32 -ladvapi32 -lshell32 -loleaut32 -lrpcrt4 -mwindows

# Cibles
all: linux windows

linux: $(LINUX_OUTPUT)
windows: $(WIN_OUTPUT)

# Linux target
$(LINUX_OUTPUT): $(OBJS)
	$(CC) $^ $(CFLAGS) -o $@

# Windows target
$(WIN_OUTPUT): $(WIN_OBJS)
	$(WIN_CC) $^ $(WIN_CFLAGS) -o $@

# Compilation Linux
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) -c $< $(CFLAGS) -o $@

# Compilation Windows
$(OBJDIR)/%.win.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(WIN_CC) -c $< $(WIN_CFLAGS) -o $@

# Dossier objets
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Nettoyage
clean:
	rm -rf $(OBJDIR) $(TARGET) $(WIN_TARGET)
