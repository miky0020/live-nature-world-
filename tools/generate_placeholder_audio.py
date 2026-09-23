import numpy as np
from scipy.signal import butter, sosfilt
from scipy.io import wavfile
import os

SR = 44100
OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets", "audio")
os.makedirs(OUT_DIR, exist_ok=True)

def butter_filter(x, sr, low=None, high=None, order=4):
    nyq = sr * 0.5
    if low and high:
        sos = butter(order, [low/nyq, high/nyq], btype='band', output='sos')
    elif high:
        sos = butter(order, high/nyq, btype='low', output='sos')
    elif low:
        sos = butter(order, low/nyq, btype='high', output='sos')
    else:
        return x
    return sosfilt(sos, x)

def normalize(x, peak=0.9):
    m = np.max(np.abs(x))
    if m < 1e-9:
        return x
    return x / m * peak

def make_loopable(y, sr, loop_seconds, xfade_seconds):
    loop_len = int(loop_seconds * sr)
    xfade_len = int(xfade_seconds * sr)
    total_needed = loop_len + xfade_len
    assert len(y) >= total_needed, f"need {total_needed}, have {len(y)}"
    head = y[:xfade_len]
    tail = y[loop_len:loop_len + xfade_len]
    t = np.linspace(0, 1, xfade_len)
    # equal power crossfade
    fade_in = np.sin(t * np.pi / 2)
    fade_out = np.cos(t * np.pi / 2)
    blended = tail * fade_out + head * fade_in
    out = y[:loop_len].copy()
    out[:xfade_len] = blended
    return out

def write_wav(name, y, sr=SR):
    y = np.clip(y, -1.0, 1.0)
    data = (y * 32767.0).astype(np.int16)
    path = os.path.join(OUT_DIR, name)
    wavfile.write(path, sr, data)
    print(f"wrote {path}  {len(y)/sr:.1f}s  {os.path.getsize(path)/1024:.0f} KB")

rng = np.random.default_rng(20260915)

# ---------------------------------------------------------------------------
# RAIN - filtered noise hiss + randomized droplet patter clicks
# ---------------------------------------------------------------------------
def gen_rain():
    loop_s, xfade_s = 18.0, 1.5
    dur = loop_s + xfade_s
    n = int(dur * SR)
    t = np.arange(n) / SR

    white = rng.standard_normal(n).astype(np.float64)
    hiss = butter_filter(white, SR, low=1200, high=9000, order=3)
    hiss = normalize(hiss, 0.5)

    # low body of the rain (heavier low patter mass)
    body = butter_filter(rng.standard_normal(n), SR, low=200, high=1800, order=3)
    body = normalize(body, 0.35)

    # slow amplitude sway so it doesn't sound like a static tape
    sway = 0.85 + 0.15 * np.sin(2*np.pi*0.05*t + 0.7) * np.sin(2*np.pi*0.017*t)

    mix = (hiss * 0.65 + body * 0.55) * sway

    # droplet clicks: short decaying bandpass bursts at random times/pitches
    clicks = np.zeros(n)
    n_clicks = int(dur * 55)  # ~55 droplets/sec average
    click_times = rng.uniform(0, dur, n_clicks)
    for ct in click_times:
        start = int(ct * SR)
        clen = int(SR * rng.uniform(0.006, 0.02))
        if start + clen >= n:
            continue
        env = np.exp(-np.linspace(0, 1, clen) * rng.uniform(9, 16))
        burst = rng.standard_normal(clen) * env
        clicks[start:start+clen] += burst * rng.uniform(0.12, 0.30)
    clicks = butter_filter(clicks, SR, low=2500, high=11000, order=3)

    y = mix * 0.8 + clicks * 0.9
    y = normalize(y, 0.85)
    y = make_loopable(y, SR, loop_s, xfade_s)
    write_wav("rain.wav", y)

# ---------------------------------------------------------------------------
# WIND - low-passed noise with slow gusting LFOs
# ---------------------------------------------------------------------------
def gen_wind():
    loop_s, xfade_s = 26.0, 2.0
    dur = loop_s + xfade_s
    n = int(dur * SR)
    t = np.arange(n) / SR

    base = butter_filter(rng.standard_normal(n), SR, high=700, order=3)
    base = normalize(base, 0.6)

    hiss = butter_filter(rng.standard_normal(n), SR, low=700, high=3500, order=3)
    hiss = normalize(hiss, 0.25)

    gust = (0.55
            + 0.25 * np.sin(2*np.pi*0.021*t + 1.1)
            + 0.15 * np.sin(2*np.pi*0.053*t + 2.4)
            + 0.10 * np.sin(2*np.pi*0.011*t + 0.3))
    gust = np.clip(gust, 0.15, 1.0)

    y = base * gust * 0.9 + hiss * gust * 0.5
    y = normalize(y, 0.8)
    y = make_loopable(y, SR, loop_s, xfade_s)
    write_wav("wind.wav", y)

# ---------------------------------------------------------------------------
# CAMPFIRE - low rumble body + random crackle/pop bursts
# ---------------------------------------------------------------------------
def gen_campfire():
    loop_s, xfade_s = 14.0, 1.2
    dur = loop_s + xfade_s
    n = int(dur * SR)
    t = np.arange(n) / SR

    rumble = butter_filter(rng.standard_normal(n), SR, low=90, high=450, order=3)
    flicker = 0.75 + 0.25 * np.sin(2*np.pi*3.1*t + 0.4) * np.sin(2*np.pi*0.6*t)
    rumble = normalize(rumble, 0.55) * flicker

    hiss = butter_filter(rng.standard_normal(n), SR, low=600, high=4000, order=3)
    hiss = normalize(hiss, 0.12)

    crackle = np.zeros(n)
    n_pops = int(dur * 9)
    pop_times = rng.uniform(0, dur, n_pops)
    for pt in pop_times:
        start = int(pt * SR)
        plen = int(SR * rng.uniform(0.01, 0.05))
        if start + plen >= n:
            continue
        env = np.exp(-np.linspace(0, 1, plen) * rng.uniform(6, 12))
        burst = rng.standard_normal(plen) * env
        crackle[start:start+plen] += burst * rng.uniform(0.35, 0.9)
    crackle = butter_filter(crackle, SR, low=900, high=7500, order=3)

    y = rumble * 0.85 + hiss * 0.5 + crackle * 0.8
    y = normalize(y, 0.85)
    y = make_loopable(y, SR, loop_s, xfade_s)
    write_wav("campfire.wav", y)

# ---------------------------------------------------------------------------
# AMBIENT DAY - soft breeze bed + scattered synthetic bird chirps
# ---------------------------------------------------------------------------
def chirp(n_samples, sr, f0, f1, shape='up'):
    t = np.linspace(0, 1, n_samples)
    if shape == 'up':
        freq = f0 + (f1 - f0) * t
    elif shape == 'warble':
        freq = f0 + (f1 - f0) * (0.5 - 0.5*np.cos(2*np.pi*t*3))
    else:
        freq = f0 + (f1 - f0) * (1 - t)
    phase = 2*np.pi*np.cumsum(freq) / sr
    env = np.sin(np.pi * t) ** 0.6
    return np.sin(phase) * env

def gen_ambient_day():
    loop_s, xfade_s = 40.0, 2.5
    dur = loop_s + xfade_s
    n = int(dur * SR)

    bed = butter_filter(rng.standard_normal(n), SR, low=250, high=4500, order=3)
    bed = normalize(bed, 0.18)

    birds = np.zeros(n)
    safe_end = dur - xfade_s - 0.4
    n_calls = int(dur * 0.9)
    call_times = rng.uniform(0.3, safe_end, n_calls)
    shapes = ['up', 'warble', 'down']
    for ctime in call_times:
        n_notes = rng.integers(1, 4)
        cursor = ctime
        base_f0 = rng.uniform(1800, 3600)
        for _ in range(n_notes):
            clen = int(SR * rng.uniform(0.08, 0.18))
            start = int(cursor * SR)
            if start + clen >= n:
                break
            f0 = base_f0 * rng.uniform(0.9, 1.1)
            f1 = f0 * rng.uniform(1.15, 1.6)
            shape = shapes[rng.integers(0, len(shapes))]
            note = chirp(clen, SR, f0, f1, shape) * rng.uniform(0.18, 0.34)
            birds[start:start+clen] += note
            cursor += clen / SR + rng.uniform(0.03, 0.09)

    y = bed * 0.9 + birds
    y = normalize(y, 0.75)
    y = make_loopable(y, SR, loop_s, xfade_s)
    write_wav("ambient_day.wav", y)

# ---------------------------------------------------------------------------
# AMBIENT NIGHT - crickets (rhythmic trilling bursts) + soft noise + owl
# ---------------------------------------------------------------------------
def gen_ambient_night():
    loop_s, xfade_s = 40.0, 2.5
    dur = loop_s + xfade_s
    n = int(dur * SR)
    t = np.arange(n) / SR

    bed = butter_filter(rng.standard_normal(n), SR, low=150, high=2500, order=3)
    bed = normalize(bed, 0.10)

    # cricket carrier ~ 4.2 kHz, pulsed at ~28 Hz within trilling phrases
    carrier = np.sin(2*np.pi*4200*t) * 0.5 + np.sin(2*np.pi*6300*t) * 0.2
    pulse = (np.sin(2*np.pi*28*t) > 0.2).astype(np.float64)
    # phrase envelope: crickets trill in waves, not constantly
    phrase = 0.5 + 0.5*np.sin(2*np.pi*0.09*t + 0.6)
    phrase = np.clip((phrase - 0.15) * 1.4, 0.0, 1.0)
    crickets = carrier * pulse * phrase * 0.16

    # a second, slightly detuned cricket "voice" panned in time for texture
    carrier2 = np.sin(2*np.pi*3850*t + 1.3) * 0.4
    pulse2 = (np.sin(2*np.pi*24*t + 2.1) > 0.25).astype(np.float64)
    phrase2 = 0.5 + 0.5*np.sin(2*np.pi*0.07*t + 3.4)
    phrase2 = np.clip((phrase2 - 0.2) * 1.4, 0.0, 1.0)
    crickets2 = carrier2 * pulse2 * phrase2 * 0.11

    owl = np.zeros(n)
    safe_end = dur - xfade_s - 1.0
    n_hoots = 3
    hoot_times = rng.uniform(2.0, safe_end, n_hoots)
    for ht in sorted(hoot_times):
        for i, (f, dt) in enumerate([(520, 0.0), (400, 0.42)]):
            hlen = int(SR * 0.35)
            start = int((ht + dt) * SR)
            if start + hlen >= n:
                continue
            tt = np.linspace(0, 1, hlen)
            env = np.sin(np.pi * tt) ** 1.5
            note = np.sin(2*np.pi*f*tt*0.35*hlen/SR*SR/hlen) # placeholder unused
            freq_env = f * (1.0 - 0.08*tt)
            phase = 2*np.pi*np.cumsum(freq_env)/SR
            tone = np.sin(phase) * env * 0.22
            breath = butter_filter(rng.standard_normal(hlen), SR, low=250, high=900) * env * 0.03
            owl[start:start+hlen] += tone + breath

    y = bed + crickets + crickets2 + owl
    y = normalize(y, 0.7)
    y = make_loopable(y, SR, loop_s, xfade_s)
    write_wav("ambient_night.wav", y)

# ---------------------------------------------------------------------------
# MUSIC - gentle ambient pad loop, slow chord progression + soft arpeggio
# ---------------------------------------------------------------------------
def gen_music():
    bpm = 80.0
    bar_s = 4 * 60.0 / bpm       # 3.0s per bar
    bars_per_chord = 4
    chords_hz = [
        [220.00, 261.63, 329.63, 392.00],   # Am7  (A C E G)
        [174.61, 220.00, 261.63, 349.23],   # Fmaj7 (F A C F... approx w/ E)
        [130.81, 164.81, 196.00, 261.63],   # Cmaj7-ish (C E G C)
        [196.00, 246.94, 293.66, 349.23],   # G(add) (G B D F)
    ]
    loop_s = bar_s * bars_per_chord * len(chords_hz)   # 48s
    xfade_s = 0.08
    dur = loop_s + xfade_s
    n = int(dur * SR)
    t_full = np.arange(n) / SR

    pad = np.zeros(n)
    chord_len_s = bar_s * bars_per_chord
    for ci, chord in enumerate(chords_hz):
        c_start = ci * chord_len_s
        c_end = c_start + chord_len_s
        i0 = int(c_start * SR)
        i1 = min(int(c_end * SR) + int(0.6*SR), n)  # small overlap for natural swell tail
        seg_n = i1 - i0
        tt = np.arange(seg_n) / SR
        # slow swelling envelope across the chord's duration
        local_len = chord_len_s
        swell = np.clip(np.sin(np.pi * np.clip(tt / local_len, 0, 1)) ** 0.8, 0, 1)
        voice = np.zeros(seg_n)
        for note_i, f in enumerate(chord):
            detune = 1.0 + (note_i - 1.5) * 0.0009
            slow_vib = 1.0 + 0.0025 * np.sin(2*np.pi*0.15*tt + note_i)
            freq = f * detune * slow_vib
            phase = 2*np.pi*np.cumsum(freq)/SR
            partial = (np.sin(phase) * 0.6 +
                       np.sin(2*phase) * 0.12 +
                       np.sin(3*phase) * 0.05)
            voice += partial / len(chord)
        pad[i0:i1] += voice * swell * 0.5

    # soft plucked arpeggio notes, sparse, on top of the pad
    pluck = np.zeros(n)
    for ci, chord in enumerate(chords_hz):
        c_start = ci * chord_len_s
        step = bar_s / 4.0
        for k in range(bars_per_chord * 4):
            note_time = c_start + k * step + rng.uniform(-0.02, 0.02)
            if note_time * SR + int(0.9*SR) >= n or note_time < 0:
                continue
            if rng.uniform(0, 1) < 0.35:
                continue  # leave gaps, sparse texture
            f = chord[rng.integers(0, len(chord))] * rng.choice([1.0, 2.0])
            plen = int(SR * 0.9)
            start = int(note_time * SR)
            tt = np.arange(plen) / SR
            env = np.exp(-tt * 3.2)
            tone = (np.sin(2*np.pi*f*tt) * 0.6 + np.sin(2*np.pi*2*f*tt) * 0.15) * env
            pluck[start:start+plen] += tone * 0.09

    y = pad + pluck
    y = butter_filter(y, SR, high=6000, order=2)
    y = normalize(y, 0.55)
    y = make_loopable(y, SR, loop_s, xfade_s)
    write_wav("music.wav", y)


gen_rain()
gen_wind()
gen_campfire()
gen_ambient_day()
gen_ambient_night()
gen_music()
print("done")
