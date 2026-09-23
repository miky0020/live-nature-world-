# 🌿 LIVE NATURE

> ✨ A cozy little 3D LiveNature, built with C++, OpenGL & FreeGLUT ✨

You're not just floating around with a camera anymore — you're in the
world. 🚶 Walk the paths, 🏡 climb into the treehouse, 🪢 ride the swing, 🎣
fish at the lake, and 🔥 warm up by the campfire. Even if you stand still,
the island keeps living: 🌬️ wind sways the trees, ☁️ clouds drift, 🌞🌙 a full
day/night cycle turns, 🌧️ weather rolls in, ✨ fireflies blink near the trees
after dark, and 9 little villagers 💬 talk, 💃 dance and 🚶 wander the paths.

---

## 🚀 1. Quick Start

### 🪟 Windows — Code::Blocks
1. Open LiveNature.cbp
2. Press F9 to build & run 🎮

Make sure FreeGLUT is installed for your compiler — see §3 if GL/freeglut.h isn't found.

### 🧩 Windows — VS Code
This project ships with .vscode/tasks.json + launch.json, so it's ready to go:
1. Open the LiveNature/ folder in VS Code
2. Ctrl+Shift+B → builds via build.bat 🔨
3. F5 → builds + launches with the debugger attached 🐞

⚠️ launch.json points at Code::Blocks' bundled gdb.exe — if yours lives
somewhere else, just update miDebuggerPath.

### 🐧🍎 Linux / macOS
make
./LiveNature

Dependencies (Debian/Ubuntu):
sudo apt install build-essential freeglut3-dev libglu1-mesa-dev

---

## 🎮 2. Controls

### 🚶 Movement

| Key | Action |
| --------------- | ------------------------- |
| W A S D | Walk |
| SHIFT | Run 🏃 |
| SPACE | Jump ⬆️ |
| 🖱️ Mouse | Look around |
| V | Toggle 1st / 3rd person 🎥 |
| C | Toggle ground collision 🧱 |

### 🤝 Interact — E

One key, does whatever you're standing near:

| Near 📍 | What happens |
| ---------------- | ---------------------------------------- |
| 🏡 Treehouse | Climb up · E again to climb down |
| 🪢 Swing | Sit · W/S to pump · E to hop off |
| 🎣 Lake shore | Cast · E again when it bites to reel in |
| 🔥 Campfire | Just vibes — warmth, glow & sound 🎶 |

### 🌍 World

| Key | Action |
| ----------- | -------------------------------------------------- |
| 1/2/3/9 | ☀️ clear / 🌧️ rain / ⛈️ heavy rain / ☁️ cloudy |
| 4 / 5 | 🌞 midday / 🌙 night |
| 6 / 7 | ✨ teleport to campfire / treehouse |
| 8 | 🎭 villager emote |
| [ / ] | ⏪ / ⏩ time speed |
| P | ⏸️ pause time |
| M | 🔇 mute audio |

### 🖥️ Interface

| Key | Action |
| ----- | ------------------------------ |
| TAB | 🖱️ free the mouse for sliders |
| F1 | 🐛 debug panel |
| F2 | 👁️ hide HUD |
| F3 | 📋 control list |
| ESC | 🚪 quit |

### 🎚️ Sliders
☀️ time of day · ⏱️ time speed · 🌧️ rain · 🌬️ wind · ☁️ clouds — all live-applied.

---

## 🛠️ 3. Build Configuration

| Setting | Value |
| -------- | --------------------------------------------------- |
| Standard | C++11 |
| Include | include |
| 🪟 Windows link | freeglut, opengl32, glu32, winmm, gdi32 |
| 🐧 Linux link | glut, GLU, GL, m |

💡 Undefined refs to glutInit? Put freeglut before opengl32 in the linker order.
💡 Builds fine but won't launch? You're probably missing freeglut.dll next to the .exe.

---

## 📁 4. Project Structure

LiveNature/
├── main.cpp
├── LiveNature_fixed.cbp
├── Makefile
├── build.bat
├── .vscode/            🧩 tasks.json + launch.json
├── include/
│   └── third_party/    🔊 miniaudio.h goes here
├── src/
└── assets/
    ├── textures/
    ├── models/
    ├── skybox/
    └── audio/

### 🧩 Module Map

| System | Job |
|---|---|
| Application | GLUT glue, input, main loop |
| World | 🌍 owns every other system |
| Player | 🚶 avatar movement, jump, swing/platform riding |
| Camera | 🎥 1st + 3rd person |
| Interaction | 🤝 "what's the player near" resolver → HUD prompt |
| Fishing | 🎣 lake minigame |
| Swing | 🪢 pendulum-physics rope swing |
| Terrain / Water | 🏔️🌊 island heightfield, lake |
| Sky / Lighting / Weather | 🌞🌙🌧️ day-night, rain/cloud/wind |
| Tree (Forest) / Vegetation | 🌳🌸 trees, grass & flower clusters |
| Fireflies | ✨ glow near trees at night |
| Environment | 🏡 treehouse & structures |
| Campfire | 🔥 flames, sparks, smoke, glow |
| Character / CharacterManager | 🕺 shared animation + NPCs |
| AudioManager | 🔊 environmental mix |
| UI | 🖥️ HUD, sliders, debug |

---

## ⚙️ 5. How Things Work

🏔️ Terrain — procedural noise + falloff, flattened center for the village.

🌞 Day/Night — one continuous time value drives sun, sky, fog, water tint, stars. Smooth, never a hard switch.

🚶 Player & Camera — reuses the NPCs' pose-animation system. 3rd-person orbits behind/above; riding the swing or standing on the treehouse platform briefly overrides normal ground physics.

🤝 Interaction — one resolver registers campfire/treehouse/swing + a lake-shore check, returns the single nearest prompt. E reads that same result.

🎣 Fishing — cast → wait for a bite → react in time to reel it in → short cooldown.

🔥 Campfire — 3 animated flame layers + sparks + smoke + a wide warm point light that reaches further and glows brighter at night, plus a ground-glow decal.

✨ Fireflies — a handful of trees get 2-4 fireflies each, drifting on a gentle looped path, fading in from dusk onward (not just pitch black night).

🌸 Vegetation — flower/grass patches scattered island-wide, plus a guaranteed ring of small flowers & grass around a chunk of the trees so undergrowth clearly grows from them.

---

## 🚀 6. Performance

Tune in include/Config.h:

TREE_COUNT · GRASS_CLUSTER_COUNT · ROCK_COUNT · FLOWER_COUNT
CHARACTER_COUNT · CLOUD_COUNT · MAX_RAIN_PARTICLES
DRAW_DISTANCE · LOD_NEAR_DISTANCE · LOD_FAR_DISTANCE

Frustum + distance culling, multi-level tree/grass LOD, spatial grid for vegetation. 🐛 F1 for live stats.

---

## 🔊 7. Audio

Logic's 100% done (day/night, rain, wind, positional campfire, music) — but it's silent out of the box, no backend/assets ship with the repo. To switch it on:

1. Drop miniaudio.h into include/third_party/
2. Add these .wav files to assets/audio/:

| File | Layer |
|---|---|
| ambient_day.wav | ☀️ day ambience |
| ambient_night.wav | 🌙 night ambience |
| rain.wav | 🌧️ rain |
| wind.wav | 🌬️ wind |
| campfire.wav | 🔥 campfire (positional) |
| music.wav | 🎵 music |

3. Add LIVINGISLAND_USE_MINIAUDIO to your compiler defines

Skip these and the game just runs quietly and logs that audio's unavailable — no crash. 🙂

---

## ⚠️ 8. Known Limitations

- 🌑 No real-time shadows (fixed-function OpenGL lighting)
- 💡 Per-vertex lighting
- 📜 Display lists (old-school, but needed for compatibility here)
- 🌧️ Rain simulates in a volume around the camera, not the whole island
- 🧍 NPCs follow predefined paths, not full pathfinding
- 🔇 Audio needs manual setup (see §7)

---

## 🔮 9. Ideas for Later

🦋 birds/butterflies · ❄️ snow · 💦 waterfalls · 🧭 real NPC pathfinding · 💾 save/load · 🎭 more animations

---

## ✅ 10. Verified

Config::WORLD_SEED = same island every run. Movement, both cameras, treehouse climbing, swing riding, fishing, weather/day-night, campfire fx, NPCs and HUD were all built and cross-checked against the real headers — audio needs §7's setup to actually test.

---

🌿 LiveNature — built with C++ · OpenGL · FreeGLUT
✨ Explore. Interact. Vibe with nature. 🌸