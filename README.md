# LIVE NATURE

An interactive 3D natural-world **game** built with **C++, OpenGL and FreeGLUT**.

You control a person, not a camera. Walk (or run) around a small living island: climb
up into the treehouse, sit on the rope swing and pump it higher, cast a line at the
lake and actually catch fish, and warm up by a campfire where nine villagers sit and
talk, dance, wave and walk the paths. Trees and grass sway in the wind, clouds drift
overhead, the sun crosses the sky through a full day/night cycle, and rain falls (with
real rain audio) when the weather turns.

Nothing in the world is static. Leave the character still and the island keeps going.
Full environmental audio is on by default: layered ambience that reacts to the time of
day, the weather and how close you are to the fire (see section 7).

---

## 1. Quick start

### Windows (Code::Blocks)

1. Open `LiveNature.cbp` in Code::Blocks.
2. Build and run (`F9`).

That's it, provided FreeGLUT is installed for your compiler — see section 3 if the
build fails on `GL/freeglut.h`.

### Linux / macOS

```bash
make
./LiveNature
```

Dependencies on Debian/Ubuntu:

```bash
sudo apt install build-essential freeglut3-dev libglu1-mesa-dev
```

---

## 2. Controls

### Movement

| Key | Action |
|---|---|
| `W` `A` `S` `D` | Walk forward / left / back / right (relative to where you're looking) |
| `SHIFT` | Run (2×) |
| `SPACE` | Jump |
| Mouse | Look around |
| `V` | Toggle first-person / third-person camera |
| `C` | Toggle player ground collision (debug) |

### Interact — `E`

`E` is the one interaction key. What it does depends on what you're standing next to;
walk up to a landmark until its prompt appears at the top of the screen, then press
`E`:

| Where | What `E` does |
|---|---|
| The treehouse | Climb up onto the platform. Press `E` again to climb back down. |
| The rope swing (near the trees) | Sit down and start swinging. Hold `W`/`S` to pump for height, press `E` again to hop off. |
| The lake shore | A visible `Press E to fish` prompt appears near the water. Cast a line, wait for `Fish on the line!`, then press `E` in time to reel it in — too slow and it gets away. Walking away from the water cancels the cast. |
| The campfire | Just a warm, lit place to stand — no prompt action, but it's a real positional light and the source of the campfire audio layer. |

### World

| Key | Action |
|---|---|
| `1` | Clear weather |
| `2` | Rain |
| `3` | Heavy rain |
| `9` | Cloudy |
| `4` | Jump to midday |
| `5` | Jump to night |
| `6` | Go to the campfire |
| `7` | Go to the treehouse |
| `8` | Trigger a character emote |
| `[` `]` | Time speed down / up |
| `P` | Pause / resume time |
| `M` | Mute audio |

### Interface

| Key | Action |
|---|---|
| `TAB` | Release the mouse to use the sliders (press again to look around) |
| `F1` | Debug panel |
| `F2` | Hide the HUD |
| `F3` | On-screen control list |
| `ESC` | Quit |

**A note on `TAB`.** Pointer-locked mouse-look and draggable sliders cannot both own
the cursor. `TAB` swaps between them; clicking anywhere in the view while the pointer
is free gives the camera back. The sliders are only hit-testable while the pointer is
free, which removes the ambiguity entirely.

### Sliders

Time of day, time speed, rain intensity, wind and cloud density. Drag a handle and
the world responds immediately. When you are not dragging, the panel mirrors the
world — so the time slider moves on its own as the clock advances.

---

## 3. Build configuration in detail

| Setting | Value |
|---|---|
| Language standard | C++11 (`-std=c++11`) |
| Include directory | `include` |
| Linker (Windows) | `freeglut`, `opengl32`, `glu32`, `winmm`, `gdi32` |
| Linker (Linux) | `glut`, `GLU`, `GL`, `m` |
| Application type | Console (so the `[INFO]` startup log stays visible) |

### Installing FreeGLUT on Windows (MinGW)

1. Download the FreeGLUT MinGW package.
2. Copy `include/GL/*.h` into your compiler's `include/GL` folder.
3. Copy `lib/libfreeglut.a` into your compiler's `lib` folder.
4. Copy `freeglut.dll` next to the built `.exe` — **or** into `C:\Windows\System32`.

The most common build failure is a missing `freeglut.dll` at *runtime*: the program
compiles and links, then silently fails to start. Put the DLL next to the executable.

### Library order matters

If you get undefined references to `glutInit` or similar, check that `freeglut` comes
**before** `opengl32` in the linker list. The `.cbp` already has this right.

---

## 4. Project layout

```
LiveNature/
├── main.cpp                 entry point — deliberately tiny
├── LiveNature.cbp            Code::Blocks project
├── Makefile                  Linux / macOS build
├── build.bat                 Windows/MinGW build script
├── include/                  24 headers
│   └── third_party/          miniaudio.h (audio backend, already included)
├── src/                      21 implementation files
├── tools/
│   └── generate_placeholder_audio.py   regenerates the six .wav loops below
└── assets/
    ├── textures/  models/  skybox/
    └── audio/                6 generated .wav loops, audio is on by default
```

### Module map

| File | Responsibility |
|---|---|
| `Application` | The only place GLUT's C callbacks touch. Owns the main loop. |
| `World` | Owns every system; drives update and render order. |
| `Config` | Every tunable number in the project. |
| `Utilities` | Vec3, deterministic RNG, value noise, clock, logging. |
| `Primitives` | Cylinders, spheres, blobs, boxes — the whole world's vocabulary. |
| `Frustum` | View-frustum extraction for culling. |
| `RenderContext` | Everything a renderer needs to know about the current frame. |
| `Camera` `Input` | Smooth free camera and polled input state. |
| `Terrain` `Water` | Procedural island heightfield, path network, animated sea. |
| `Sky` `Lighting` | Dome, stars, sun/moon, clouds; the sun and the colour palette. |
| `Weather` `ParticleSystem` | Weather state machine; pooled rain, sparks and fireflies. |
| `Tree` `Vegetation` | Three tree species; grass, flowers and rocks. |
| `Environment` `Campfire` | Treehouse, benches, lantern posts; fire, embers and sparks. |
| `Character` `CharacterManager` | Animation system; conversation and emote behaviour. |
| `AudioManager` | Environmental audio mixing with distance attenuation. |
| `UI` | HUD, control panel, sliders, debug panel, help overlay. |

---

## 5. How the systems work

### Terrain

Three layers of value-noise fbm — broad hills, secondary ridges, surface texture —
multiplied by a radial falloff that drops the land below the waterline near the edges.
The middle is flattened into a plateau so the campfire and treehouse have sane ground
to sit on.

> **Tuning note.** Value-noise fbm clusters tightly around 0.5, so the amplitudes in
> `computeHeight` are deliberately large. A first pass using "reasonable-looking"
> numbers produced 7 units of relief across a 140-unit island — technically not flat,
> visually flat. The current values were tuned against a height histogram.

The mesh is baked once into a display list. `heightAt()` bilinearly samples the *same*
stored grid the mesh was built from, so queries and visible geometry agree exactly —
which is why nothing in the world floats or sinks.

### Paths

Defined analytically in `Terrain::pathInfluence()` — a ring around the village plus
spurs to the treehouse and the gathering area — and baked into the terrain's vertex
colours. Every scatterer consults the same function, so grass and rocks can never
grow through a walkway. Drawing paths as separate decal geometry would mean fighting
the depth buffer for no gain.

### Day/night and lighting

One continuous value (time of day in hours) produces the sun direction, and the sun's
*elevation* produces every colour in the scene: sun tint, ambient, sky gradient, fog,
water. Nothing switches between discrete day and night states, so the transitions are
smooth for free. Stars fade in on a smoothstep of the night factor rather than
appearing.

### Wind

`Weather` integrates a wind *phase* rather than multiplying time by the current wind
strength. This matters: multiplying means a change in wind strength makes the entire
world snap to a new sway position. Integrating means it never does.

Gusts are two out-of-phase sine waves plus drifting noise, so the whole island gusts
together while each tree keeps its own phase offset.

### Trees

A unit tree of each species and detail level is compiled once into a display list with
no colour commands inside. Each of the ~90 instances pushes a matrix, sets its own
colour, and calls the list. Trunk and foliage are separate lists so the canopy rotates
in the wind while the trunk stays planted.

Three LOD levels by distance, plus frustum culling — the cheapest tree is the one
never submitted.

### Grass

Crossed tapered quads in shared display lists, scattered procedurally, aggressively
distance-culled, and switched to a cheaper single-cross version past 38 units.

> **Lighting note.** The blade normals point mostly *up*, not outward along the quad's
> facing. A physically correct outward normal makes every blade catch only grazing
> light and read as a dark floating card — this was a real bug during development.
> Biasing the normal upward makes the tuft light like the ground it grows from, which
> is what the eye expects. Cheating foliage normals is standard practice.

### Campfire

The campfire now combines layered animated flame, ember cores, additive glow, sparks, smoke and a warm local light. Rain dampens the flame and spark emission.

The flames are rebuilt every frame from stacked rings whose radius and centre are
displaced by drifting value noise, so the silhouette genuinely changes shape rather
than spinning. Three nested layers — a wide dull red base, an orange body, a hot
yellow core — each on a different time offset. Cost is trivial: 3 layers × 7 rings ×
9 slices.

The fire is a real positional GL light with quadratic attenuation, and rain visibly
dampens both its glow and its spark emission.

### Particles

Fixed-size pools are allocated once at startup, so a steady-state frame does zero heap work. Fireflies are preallocated and drift around the treehouse, campfire meadow and other flower clearings; their emissive glow becomes strongest at night.
Rain spawns in a box that travels with the camera, so you always stand inside the
shower without simulating rain on the far side of the island. Drops are drawn as line
segments stretched along their velocity — that is what makes them read as *falling*
rather than as floating dots.

### Characters

The animation system keeps the "don't duplicate animation code" rule honest:

- a `Pose` is a plain bag of joint angles
- each state is a pure function `(state, time, phase) -> Pose`
- a character stores only its state, its clock and a personal phase offset
- switching state captures the current pose and blends toward the new one over 0.4s,
  so nothing ever snaps

Adding an animation means adding a case to `evaluatePose` — never touching the
renderer.

The conversation logic is what sells the campfire circle: a single "speaker" token
passed around on an irregular timer, with everyone else turned toward whoever has the
floor, plus occasional silent pauses. That reads as a conversation in a way random
gesturing never does.

### Lighting budget

Exactly three lights: sun (`GL_LIGHT0`), campfire (`GL_LIGHT1`), lantern
(`GL_LIGHT2`). Fixed-function lighting is per-vertex, so every extra light costs on
every vertex in the scene. The lantern is disabled outright in daylight.

### Render order

Sky backdrop (no depth, no fog, no lighting) → lights and fog → all opaque geometry →
emissive surfaces → transparent passes (water, clouds, flames, rain). The order is
load-bearing: everything opaque must be in the depth buffer before a single
transparent fragment is drawn, or rain shows through trees.

---

## 6. Performance

Scale the world from `include/Config.h`:

```cpp
TREE_COUNT            = 90;
GRASS_CLUSTER_COUNT   = 9000;
ROCK_COUNT            = 55;
FLOWER_COUNT          = 520;
CHARACTER_COUNT       = 9;
CLOUD_COUNT           = 18;
MAX_RAIN_PARTICLES    = 1800;
DRAW_DISTANCE         = 320.0f;
LOD_NEAR_DISTANCE     = 45.0f;
LOD_FAR_DISTANCE      = 130.0f;
```

If it runs slowly, lower `GRASS_CLUSTER_COUNT` first — it is by far the largest
instance count, and halving it roughly doubles the frame rate. Then `TREE_COUNT`
and `DRAW_DISTANCE`.

### What makes this run on modest hardware

Grass and flowers are indexed into a 16-unit spatial grid (`Vegetation::m_cells`).
Instead of testing every instance on the island against the view frustum every
frame, the renderer walks only the grid cells within draw distance of the
camera, does one cheap sphere test per cell, and skips every instance inside a
rejected cell in one step. This is what makes "only render what you're looking
at" actually cheap: cost scales with what's on screen, not with the size of the
world. The same principle already applied to trees, rocks and characters via
per-instance frustum and distance culling; the grid is what extends it to
grass, which is the one category with enough instances (thousands) that a
linear per-instance scan would show up in the frame time.

Grass renders in three LOD tiers — 10 blades, 5 blades, 3 blades — swapped by
distance, with segment count dropping too so the far tier is both fewer blades
and cheaper blades. Trees dropped from up to 26 individual leaves per canopy
mass to a tuned 10-14, and lost roughly a third of their sphere/blob
tessellation, all with no visible loss of shape at normal viewing distance.
LOD distance bands were tightened (`LOD_NEAR_DISTANCE` 45→32, `LOD_FAR_DISTANCE`
130→95) so full-detail trees are rarer and the simplified tier carries more of
the visible forest.

### A real bug this pass fixed

`Sky::renderCelestialBody()` switches to **additive** blending
(`glBlendFunc(GL_SRC_ALPHA, GL_ONE)`) to draw the sun and moon's glow, and
originally wrapped that in `glPushAttrib(GL_ENABLE_BIT)`. That bit saves
whether blending is *on or off* — it does **not** save *which blend function is
active*, which is `GL_COLOR_BUFFER_BIT`. So the additive function survived
`glPopAttrib()` and leaked straight into the terrain draw that follows it in
the same frame: every opaque surface's colour got *added* to the sky behind it
instead of replacing it, which is exactly what "the whole world is white" looks
like. Four other places in the project set an additive or blended function the
same way (campfire flames, embers, the lantern halo). All five now include
`GL_COLOR_BUFFER_BIT` in their guard, and `World::render()` additionally forces
`glDisable(GL_BLEND)` for the entire opaque pass and re-establishes the normal
blend function explicitly before the transparent pass — so the bug class is
closed at the architecture level, not just patched at each call site.

### Water

The sea now shades itself per vertex rather than through a flat vertex colour:
a Blinn-Phong specular term computed by hand gives the sun a visible glitter
streak across the water (strongest at low sun angles, exactly like a real
lake), a Fresnel term brightens and cools the colour at grazing angles so the
sea reads as reflective rather than as painted glass, and the wave amplitude
and speed were both raised so the surface visibly *flows* rather than idling.
The surface normal is computed analytically from the derivative of the wave
function, not by sampling neighbouring vertices, which is what keeps the
highlight smooth on a coarse, cheap grid.

Press `F1` for live counts of visible trees, grass, props, people and particles.

---

## 7. Adding assets

The project draws a complete world from procedural geometry alone and has **no
required assets**. Everything below is optional enhancement.

### Audio

Audio is on by default — nothing to set up. `include/third_party/miniaudio.h` is
already in the tree, `LIVINGISLAND_USE_MINIAUDIO` is already defined in the Makefile,
`build.bat` and `LiveNature.cbp`, and `assets/audio/` already has six looping .wav
files:

| File | Layer |
|---|---|
| `ambient_day.wav` | Birds and daytime nature |
| `ambient_night.wav` | Crickets and owls |
| `rain.wav` | Rain — fades in/out with rain intensity, dominates in a downpour |
| `wind.wav` | Wind — only audible once it's actually gusting |
| `campfire.wav` | Fire crackle (positional — gets quieter as you walk away, and dies down with the fire) |
| `music.wav` | Gentle background pad |

Those six loops are procedurally synthesized placeholders (filtered noise for rain/
wind/fire, synthesized bird and cricket calls, a generated ambient pad) — see
`tools/generate_placeholder_audio.py` to inspect or tweak them, or just drop your own
recordings over them using the same filenames. Every file is independently optional —
a missing or broken one disables just that layer, logs a warning and changes nothing
else, so swapping in real recordings is a drop-in replacement, not a code change.

To build silently instead, remove `-DLIVINGISLAND_USE_MINIAUDIO` from whichever build
file you're using; `AudioManager` degrades to a no-op automatically.

Why not `PlaySound`? It ships with Windows, but plays one sound at a time with no
volume control, which makes a layered reactive mix impossible. The mixing logic in
`AudioManager` is always compiled and always correct; only the backend is swappable.

### Textures and models

`Terrain`, `Tree`, `Vegetation`, `Environment` and `Character` each build their
geometry in one clearly marked place. Texture loading (e.g. via `stb_image.h`, same
single-header pattern as miniaudio) can be added there without touching anything else.

---

## 8. Known limitations

- **No shadows.** Fixed-function OpenGL has no cheap shadow path. The most visible
  consequence is a missing ground-contact cue for tall objects — the treehouse needed
  an explicit earth mound at its base to stop it *reading* as floating, even though it
  is geometrically buried 1.5 units below the lowest nearby ground.
- **Per-vertex lighting.** Light falloff is only as smooth as the mesh is dense. The
  campfire's warm pool is visible because the terrain grid is fine enough there.
- **Display lists are deprecated** in modern OpenGL. They are the right call here:
  maximum compatibility with a typical GLUT setup, and no shader pipeline to
  maintain — exactly the constraint the brief sets.
- **`SHIFT` detection.** GLUT has no event for modifier keys pressed alone, and
  `glutGetModifiers()` is only legal inside an input callback. It is polled from every
  input callback; since mouse-look generates motion events continuously, `SHIFT`
  registers within a frame or two.
- **Rain is camera-local.** You will not see a distant curtain of rain across the
  island, only the shower you are standing in.
- **Characters do not avoid obstacles.** Walkers follow a fixed ring-path loop.

---

## 9. Extending it

The architecture is built for additions. Each is one member, one update call and one
render call in `World`:

- **Birds / butterflies** — reuse `ParticlePool`, or instance a small animated mesh
- **Snow** — a second particle system; copy `RainSystem` and change the integration
- **More weather** — add to the `WeatherType` enum and its target table
- **Waterfalls** — a `SparkSystem` variant with downward velocity
- **NPC navigation** — `Character::setPatrol()` already takes an arbitrary waypoint list
- **Save/load** — `World::controls()` and `applyControls()` already round-trip the
  entire environment state

---

## 10. Architecture notes for extending this further

- **Never call `glBlendFunc` (or any state-changing call) without owning its
  cleanup.** If a `glPushAttrib`/`glPopAttrib` pair wraps a blend function
  change, the mask must include `GL_COLOR_BUFFER_BIT` or the change survives
  the pop. This project's one real bug this round came from exactly that
  omission, five times over.
- **Opaque and transparent passes should each open by asserting their own
  blend state**, not by trusting whatever the previous draw call left behind.
  `World::render()` does this explicitly now; any new render pass should
  follow the same pattern rather than assuming inherited state is correct.
- **Spatial indexing pays for itself past a few hundred instances.** Trees and
  rocks (tens to low hundreds) get away with a linear scan plus a per-instance
  frustum test. Grass (thousands) does not — the grid is the difference
  between "cost scales with what's visible" and "cost scales with the size of
  the island". The same grid could carry flowers-only queries, decorative
  clutter, or anything else scattered in similar numbers.

## 11. Verification

Built and tested with **g++ 13.3** and **FreeGLUT 3.4**:

- all 22 translation units compile with `-Wall -Wextra` and **zero warnings**
- links clean, including the miniaudio backend (`-lpthread -ldl` on Linux)
- verified running under software rasterization (llvmpipe, no GPU) in both a clear
  midday scene and a night scene with rain — day/night, weather, fire light, lantern
  glow, stars, particles, characters and the full UI all confirmed rendering
- the audio backend initializes and logs **"Audio ready: 6 of 6 layers loaded"** on
  startup; a machine with a real audio device plays the layered mix. A container with
  no sound card logs ALSA warnings at startup, but initialization, file loading and
  the rest of the game all still run correctly regardless.
- climbing the treehouse, riding the swing and the full fishing cycle (cast → bite →
  reel → catch/miss) were each exercised via the debug shortcuts and confirmed to
  drive the player, camera and HUD prompt correctly

The island is generated from a fixed seed (`Config::WORLD_SEED`), using a deterministic
xorshift RNG rather than `std::rand()` — so it looks identical on every machine and
every run. That matters for a demonstration.
