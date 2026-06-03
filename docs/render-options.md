# mymenu Render and Viewer Options

This document explains the build-time options added for display throughput, RAM usage, selective redraw behavior, and remote viewer behavior.

## Where To Set Options

Set options in your project build flags (usually in `platformio.ini`), not inside `mymenu`.

Example:

```ini
build_flags =
  -DDISPLAY_RGB332_FB_MODE
  -DDISPLAY_RGB332_DMA_PINGPONG=1
  -DDISPLAY_RGB332_DMA_CHUNK_LINES=16
  -DDISPLAY_RGB332_DIRTY_FLUSH=1
  -DMENU_SELECTIVE_STATIC_REDRAW=1
  -DREMOTE_VIEWER_LIVE_MAX_FPS=10
```

## Display Pipeline Options

### DISPLAY_RGB332_FB_MODE

What it does:
- Uses an internal 8-bit RGB332 framebuffer when rendering with Bodmer sprite mode.
- Converts to RGB565 when pushing to panel.

Benefits:
- Large RAM reduction vs internal RGB565 sprite.

Costs and risks:
- Extra conversion work per flush.
- Slight color precision reduction.
- Requires `BODMER_SPRITE`.

Notes:
- Compile-time guard exists in `display_bodmer.h` and will error if `BODMER_SPRITE` is missing.

### DISPLAY_RGB332_DMA_CHUNK_LINES

What it does:
- Controls how many scanlines are converted/pushed per DMA chunk.

Benefits:
- Lets you tune smoothness vs memory pressure.

Costs and risks:
- Larger chunk size: fewer DMA submissions, but larger stage buffers.
- Smaller chunk size: lower transient memory per chunk, but more overhead.

Practical guidance:
- Start at `16`.
- If RAM is tight, reduce.
- If tearing/stutter increases, try increasing cautiously.

### DISPLAY_RGB332_DMA_PINGPONG

What it does:
- Uses two stage buffers to overlap conversion and DMA.

Benefits:
- Better throughput on some targets.

Costs and risks:
- Extra RAM for second stage buffer.
- When debugging memory regressions, disable first to isolate impact.

### DISPLAY_RGB332_DIRTY_FLUSH

What it does:
- Tracks dirty region and flushes only changed area (RGB332 path).

Benefits:
- Can reduce transferred pixels when UI changes are localized.

Costs and risks:
- UI patterns with broad redraws may see little gain.
- Dirty-rectangle bugs can create stale artifacts if bounds tracking is wrong.

Practical guidance:
- Keep enabled only if measured benefit exists on your real workload.

## Menu Redraw Option

### MENU_SELECTIVE_STATIC_REDRAW

What it does:
- Adds redraw policy checks so static rows are skipped when state is unchanged.
- Enables message-row and pinned-panel cache/dirty behavior.

Benefits:
- Can reduce draw work for dense static menus.

Costs and risks:
- More stateful rendering paths.
- New controls must integrate redraw state correctly.
- Incorrect cache invalidation can cause stale rows, visual gaps, or scroll-state issues.

Integration checklist for new controls:
1. Choose a redraw policy if the control is mostly static.
2. If value/time-driven, trigger redraw on true value changes.
3. For pinned controls, request redraw when displayed content changes.

## Remote Viewer Options

### REMOTE_VIEWER_LIVE_MAX_FPS

What it does:
- Caps live frame send rate for serial viewer streaming.

Benefits:
- Prevents viewer traffic from dominating loop time.

Costs and risks:
- Too low: remote UI feels laggy.
- Too high: serial traffic can impact responsiveness.

### REMOTE_VIEWER_HELLO_RESEND_MS and REMOTE_VIEWER_HELLO_MAX_AUTO_SENDS

What they do:
- Control auto-resend behavior of viewer capability hello packets.

Benefits:
- Helps host reconnect/discovery reliability.

Costs and risks:
- Aggressive resend settings can spam serial and interfere with other serial tooling.

Practical guidance:
- Keep auto sends conservative.
- Let explicit host `^` requests drive re-announcement when possible.

## Suggested Presets

### Safe Baseline

```ini
-DDISPLAY_RGB332_FB_MODE
-DDISPLAY_RGB332_DMA_PINGPONG=0
-DDISPLAY_RGB332_DMA_CHUNK_LINES=16
-DDISPLAY_RGB332_DIRTY_FLUSH=0
-DMENU_SELECTIVE_STATIC_REDRAW=0
```

### Throughput-Focused (validated carefully on device)

```ini
-DDISPLAY_RGB332_FB_MODE
-DDISPLAY_RGB332_DMA_PINGPONG=1
-DDISPLAY_RGB332_DMA_CHUNK_LINES=16
-DDISPLAY_RGB332_DIRTY_FLUSH=1
-DMENU_SELECTIVE_STATIC_REDRAW=1
```

## Known Pitfalls

- Scroll cache corruption can occur if absolute panel-bottom caches are written from partial redraw paths.
- Message row and header row rendering must reserve stable row height to avoid overlap/flicker.
- Shared cache experiments across pages can break long-list navigation if page-local assumptions leak.

## Automation Modes

### Page Sweep + Screenshot Capture

What it does:
- Walks through pages one by one using the viewer/page navigation commands.
- Pauses on each page for a fixed dwell time.
- Saves a screenshot for each page automatically.

Why it helps:
- Fast documentation capture.
- Regression testing against layout changes.
- Repeatable visual comparison sets.

Trade-offs:
- This is only as good as the current frame being presented when the capture happens.
- If remote live streaming is enabled, the benchmark reflects viewer traffic as well as UI work.
- If you want a pure UI timing number, keep remote framebuffer streaming off during the timing pass and use the device summary/profile block instead.

### Console Summary Request

What it does:
- Prints a summary block with board, CPU speed, page info, profile string, and key feature flags.

Why it helps:
- Lets you attach a reproducible hardware/config snapshot to benchmark logs.

Useful fields to record:
- board
- cpu_mhz
- pages
- page_index
- page_title
- profile
- flags

## Benchmark Report Template

Use this as a lightweight log format when comparing runs:

```text
run: 2026-05-31
board: seeed_xiao_rp2350
cpu_mhz: 150
pages: 24
page_sweep_mode: enabled
framebuffer_streaming: off
profile: 18fps | RAM2 143K free
flags: DISPLAY_RGB332_FB_MODE=1;DISPLAY_RGB332_DMA_PINGPONG=1;DISPLAY_RGB332_DIRTY_FLUSH=0;MENU_SELECTIVE_STATIC_REDRAW=1;
notes: viewer disconnected, local timing only
```

## New Control Checklist

When adding a new `MenuItem` subclass:
1. Keep the default redraw policy `REDRAW_ALWAYS` unless the control is provably static.
2. Use `REDRAW_ON_SELECTION` or `REDRAW_ON_OPEN_STATE` only when redraw state is truly bounded by those transitions.
3. If the control reads live state every frame, do not try to force selective redraw without a value-change hook.
4. For pinned controls, call `request_redraw()` whenever the visible content can change.
5. If the control renders labels with varying lengths, cache label length or another stable size signal instead of recomputing it every frame.
6. Test the control on both a quiet page and a mixed page with headers/messages/tabs before enabling selective redraw.

## Validation Workflow

After changing any option:
1. Test page switching, long scroll lists, and opened-item transitions.
2. Verify pinned panel behavior and message row stability.
3. Verify remote viewer reconnect and live toggle behavior.
4. Compare FPS/latency and RAM delta on hardware, not simulator only.
