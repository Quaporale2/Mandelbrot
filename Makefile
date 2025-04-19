

# Nom des exécutables
VERSION = v1.1
LINUX_OUTPUT = Fractal-linux-x86_64-$(VERSION)
WIN_OUTPUT = Fractal-win64-$(VERSION).exe


# Cibles
all: linux windows

linux: $(LINUX_OUTPUT)
windows: $(WIN_OUTPUT)


# Dossiers contenant les sources et les en-têtes
SRCDIR = src
OBJDIR = obj
INCDIR = include

ICON_RES = icon.res


# Fichiers source
SRCS = $(wildcard $(SRCDIR)/*.c)

# Fichiers objets
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
WIN_OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.win.o,$(SRCS))


# Compilateur Linux
CC = gcc
CFLAGS = -I$(INCDIR) -I./libs/SDL2-linux/include -L./libs/SDL2-linux/lib \
         -lSDL2 -lSDL2_image -lSDL2_ttf -g -Wall -lmpfr -lgmp -lm -Winline 



# Compilation de l'icone
$(ICON_RES): icon.rc icon.ico
	x86_64-w64-mingw32-windres icon.rc -O coff -o $(ICON_RES)

# Compilateur Windows (cross-compilation)
WIN_CC = x86_64-w64-mingw32-gcc
WIN_CFLAGS = -I$(INCDIR) -I./libs/SDL2-win/include -I./libs/SDL2-win/include/SDL2 -L./libs/SDL2-win/lib \
             -lSDL2 -lSDL2_image -lSDL2_ttf -lm -static \
             -lsetupapi -lole32 -lcomdlg32 -limm32 -lversion -lwinmm -lgdi32 -ldinput8 -luser32 -ladvapi32 -lshell32 -loleaut32 -lrpcrt4 -mwindows


# Linux target
$(LINUX_OUTPUT): $(OBJS)
	$(CC) $^ $(CFLAGS) -o $@

# Windows target
$(WIN_OUTPUT): $(WIN_OBJS) $(ICON_RES)
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
	rm -rf $(OBJDIR) $(TARGET) $(WIN_TARGET) $(ICON_RES)



# Installe l'icone pour les systèmes linux
install-icon-linux:
	sed "s|__DIR__|$(realpath .)|g" Fractal.desktop | sed "s|__EXEC__|$(LINUX_OUTPUT)|g" > /tmp/fractal.Desktop
	sudo cp /tmp/fractal.Desktop /usr/share/applications/fractal.desktop



