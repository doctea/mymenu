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


