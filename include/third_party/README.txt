miniaudio.h is already dropped in here, and LIVINGISLAND_USE_MINIAUDIO is
already enabled in the Makefile, build.bat and LiveNature.cbp, so audio is
ON by default - no setup needed.

miniaudio (https://github.com/mackron/miniaudio) is public domain / MIT-0,
single-header, and talks to the OS audio device (WASAPI on Windows, ALSA/
PulseAudio on Linux) entirely through runtime dynamic loading, so nothing
extra needs to be linked.

To go back to a silent build, remove -DLIVINGISLAND_USE_MINIAUDIO from
whichever build file you're using - AudioManager degrades to a no-op and
nothing else changes (see include/AudioManager.h).
