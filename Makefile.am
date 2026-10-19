bin_PROGRAMS = saltarello
saltarello_SOURCES = \
	src/main.cpp \
	src/game.cpp src/game.h \
	src/render.cpp src/render.h \
	src/audio.cpp src/audio.h \
	src/assets.cpp src/assets.h
saltarello_CPPFLAGS = -DPKGDATADIR='"$(pkgdatadir)"'
saltarello_CXXFLAGS = $(SDL2_CFLAGS) $(SDL2_TTF_CFLAGS) $(SDL2_MIXER_CFLAGS) -std=c++17 -Wall -Wextra
saltarello_LDADD = $(SDL2_LIBS) $(SDL2_TTF_LIBS) $(SDL2_MIXER_LIBS)

if ENABLE_ASAN
saltarello_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer -g
saltarello_LDFLAGS = -fsanitize=address
endif

# Installed as $(pkgdatadir)/assets/...
nobase_dist_pkgdata_DATA = \
	assets/background.mp3 \
	assets/collision.wav \
	assets/font.ttf \
	assets/jump.wav \
	assets/shoot.wav
