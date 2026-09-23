These six loops are procedurally synthesized placeholder audio (noise-
shaped rain/wind/campfire, synthesized bird/cricket calls, a generated
ambient pad) so the game has real, working sound out of the box with no
extra downloads. See tools/generate_placeholder_audio.py to see/tweak how
they were made, or regenerate them with different parameters.

Every file below is independently optional - AudioManager (see
include/AudioManager.h) disables just that layer and logs a warning if a
file is missing or fails to load, so dropping in your own recordings is a
drop-in replacement, not a code change:

  ambient_day.wav    birds + a soft daytime bed        (loops ~40s)
  ambient_night.wav  crickets, an owl, a quiet bed      (loops ~40s)
  rain.wav           rain hiss + droplet patter         (loops ~18s)
  wind.wav           gusting low-passed noise           (loops ~26s)
  campfire.wav       rumble + crackle/pop bursts        (loops ~14s)
  music.wav          gentle ambient pad + arpeggio      (loops ~48s)

All are 44.1kHz / 16-bit mono WAV, streamed from disk (not fully loaded
into RAM), and looped seamlessly (crossfaded at the loop point, or - for
music.wav - built as an exact whole number of musical bars).
