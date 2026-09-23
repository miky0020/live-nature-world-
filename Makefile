CXX      := g++
# LIVINGISLAND_USE_MINIAUDIO turns on the real audio backend (miniaudio.h,
# already dropped into include/third_party/). Remove the define below to
# build silently without linking any audio backend.
CXXFLAGS := -std=c++11 -O2 -Wall -Wextra -Iinclude -DLIVINGISLAND_USE_MINIAUDIO
# miniaudio talks to ALSA/PulseAudio on Linux via dlopen() at runtime, so it
# only needs -lpthread -ldl -lm at link time, no ALSA dev headers required.
LDFLAGS  := -lglut -lGLU -lGL -lm -lpthread -ldl

SRC := main.cpp \
       src/Application.cpp \
       src/AudioManager.cpp \
       src/Camera.cpp \
       src/Campfire.cpp \
       src/Character.cpp \
       src/CharacterManager.cpp \
       src/Environment.cpp \
       src/Frustum.cpp \
       src/Input.cpp \
       src/Lighting.cpp \
       src/ParticleSystem.cpp \
       src/Player.cpp \
       src/Interaction.cpp \
       src/Fishing.cpp \
       src/Swing.cpp \
       src/Tent.cpp \
       src/Primitives.cpp \
       src/Sakura.cpp \
       src/Sky.cpp \
       src/Terrain.cpp \
       src/Tree.cpp \
       src/UI.cpp \
       src/Utilities.cpp \
       src/Vegetation.cpp \
       src/Water.cpp \
       src/Weather.cpp \
       src/World.cpp

OBJ := $(SRC:.cpp=.o)
BIN := LiveNature

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $(BIN) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
