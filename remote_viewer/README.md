# What

A python app for viewing framebuffers sent over serial connection.

# Install

Install like:

`python -m venv venv`

# Run

Run like:
```
venv/Scripts/activate
python viewer.py COM16
```

# Use

- 'd' to send a 'knob left'
- 'f' to send a 'knob right'
- 'a' to send a 'back button'
- 'b' to send a 'encoder click'
- 'e' to request a new frame
- '[' to request previous page
- ']' to request next page
- '?' to request a summary block
- 's' to save current frame
- 'z' to toggle double-size display window
- 'q' to quit

## Automation

The viewer also has a page-sweep mode for documentation and regression capture.

Example:

```bash
python viewer.py COM16 --auto-sweep-pages --page-dwell-ms 2500 --screenshot-dir screenshots
```

That mode steps through every page, requests a frame, and writes one PNG per page to the output directory.
It also writes a `sweep_report.md` file in the same directory with embedded images, device summary data, and FPS samples.
The report also includes git branch/commit/dirty status for the viewer checkout and a build-flags line from the device hello block.
If you omit `--screenshot-dir`, the viewer will create a folder named with the current date and time.

During the sweep, live streaming is paused between captures so the device can run normally while the dwell timer runs.

## Headless observation and replay

The normal viewer window still works as before. For automated observation without
a window, run a bounded capture (close other programs using the same COM port first):

```powershell
.\venv\Scripts\python.exe .\viewer.py COM3 --headless --duration-seconds 10 --screenshot-dir C:\Temp\viewer-observation --record-stream C:\Temp\viewer-stream.bin
```

The output directory contains sampled `frame_000001.png` images and `latest.png`.
The last valid frame is saved on completion. The console reports valid-frame and
rejected-frame counts and bytes skipped outside accepted packets. Skipped bytes
can be debug text or remnants of rejected frames; they are not proof of logging.
Use `--snapshot-interval-seconds 0.5` to sample more often. Headless mode does not
send navigation commands; it only requests discovery/live streaming and stops
streaming when the port closes. No valid frames is an error, not a successful run.

`--record-stream` records received bytes, including debug messages and capability
blocks, starting before discovery. It refuses to overwrite an existing file.
Its parent directory must already exist. It also works with the normal viewer.
Recordings can contain device debug output; keep them local unless reviewed.

Replay a recording without connecting to hardware:

```powershell
.\venv\Scripts\python.exe .\viewer.py --headless --replay C:\Temp\viewer-stream.bin --screenshot-dir C:\Temp\viewer-replay
```

Replay runs as fast as possible, up to the duration limit; it does not reproduce
original timing or send commands to a device. Snapshots can be opened directly by
an assistant for visual inspection. This allows a capture, inspect, fix, replay
loop without screen-sharing the OpenCV window.

The normal window can also be launched for a bounded session:

```powershell
.\venv\Scripts\python.exe .\viewer.py COM3 --duration-seconds 6
```

Without a duration, the normal window remains open until you quit. Timed viewing
cannot be combined with a page sweep.

## Packet validation

The reader shares one receive buffer between discovery and frame parsing, so bytes
after a capability block are retained. It consumes frame trailers, reports normal
console traffic separately, and rejects invalid headers, trailers, and incomplete
or overflowing RLE data instead of displaying partially initialized pixels.
Raw words are decoded explicitly as little-endian. Supported dimensions are
1 to 1024 pixels on each axis; unknown encodings and unreasonable lengths are
rejected before reading a payload. After rejection, buffered data is scanned for
the next packet marker.

This is still the existing firmware protocol: there is no checksum. Damage inside
a correctly sized raw payload cannot necessarily be detected, nor can this
prevent framebuffer changes during transmission.

The ILI9341 and Bodmer senders now use foundling's checked `SerialPacket` writer
instead of assuming a large serial write succeeds completely. It sends at most
512 bytes per call, retries positive partial writes, and suppresses the frame
trailer after failure. Capability blocks use the same packet guard and expose
`serial_output` counters for dropped log bytes, short writes, and completed/failed
packets. These counters measure driver acceptance, not host receipt.

Enable `ENABLE_SERIAL_OUTPUT_QUEUE` project-wide to queue the existing shared
debug/serial macros during packet output, and drain the queue in the main loop.
Direct `Serial.*` logging still bypasses that protection. See foundling's README
for queue limits, interrupt-context support, and failure reporting.

Run the focused regression tests from this directory:

```powershell
.\venv\Scripts\python.exe -m unittest -v test_viewer
```


