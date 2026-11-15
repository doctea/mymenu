# viewer.py
import serial
import struct
from PIL import Image
import numpy as np
import cv2
import time
import win32clipboard
from pathlib import Path
import subprocess
from datetime import datetime

from io import BytesIO

PORT = "COM16"           # change to your port, e.g., /dev/ttyACM0
BAUD = 2000000

def send_to_clipboard(image):
    output = BytesIO()
    image.convert('RGB').save(output, 'BMP')
    data = output.getvalue()[14:]
    output.close()

    win32clipboard.OpenClipboard()
    win32clipboard.EmptyClipboard()
    win32clipboard.SetClipboardData(win32clipboard.CF_DIB, data)
    win32clipboard.CloseClipboard()

START_MARKER = b'==START-FRAME=='
END_MARKER = b'==END-FRAME=='
HELLO_MARKER = b'==VIEWER-HELLO=='
HELLO_END_MARKER = b'==END-VIEWER-HELLO=='
FRAME_HEADER = struct.Struct('<HHBI')
MAX_FRAME_DIMENSION = 1024
MAX_HELLO_BYTES = 16384


class FrameProtocolError(ValueError):
    pass


class ReplayTransport:
    def __init__(self, source):
        self.source = source
        self.size = source.seek(0, 2)
        source.seek(0)

    @property
    def in_waiting(self):
        return self.size - self.source.tell()

    def read(self, size):
        chunk = self.source.read(size)
        if not chunk:
            raise EOFError("End of recorded viewer stream")
        return chunk


class ViewerStream:
    def __init__(self, transport, recording=None):
        self.transport = transport
        self.recording = recording
        self.buffer = bytearray()
        self.rejected_frames = 0
        self.skipped_bytes = 0

    @property
    def in_waiting(self):
        return len(self.buffer) + self.transport.in_waiting

    def write(self, data):
        written = self.transport.write(data)
        if written != len(data):
            raise IOError(f"Short command write: {written}/{len(data)} bytes")
        return written

    def flush(self):
        self.transport.flush()

    def _receive(self, deadline):
        while time.monotonic() < deadline:
            chunk = self.transport.read(min(self.transport.in_waiting or 1, 65536))
            if chunk:
                if self.recording is not None:
                    self.recording.write(chunk)
                self.buffer.extend(chunk)
                return
        raise TimeoutError("Timed out waiting for viewer data")

    def _ensure(self, size, deadline):
        while len(self.buffer) < size:
            self._receive(deadline)

    def _skip_non_packet_data(self, size):
        if size:
            self.skipped_bytes += size
            preview = bytes(self.buffer[:min(size, 160)])
            print(f"Non-packet data ({size} bytes): {preview!r}")
            del self.buffer[:size]

    def _reject_frame(self, message):
        self.rejected_frames += 1
        # Keep subsequent bytes: a truncated packet may contain the next marker.
        del self.buffer[:1]
        raise FrameProtocolError(message)

    def read_packet(self, deadline=None):
        if deadline is None:
            deadline = time.monotonic() + 2.0
        while True:
            positions = [
                (position, kind)
                for marker, kind in ((START_MARKER, 'frame'), (HELLO_MARKER, 'hello'))
                if (position := self.buffer.find(marker)) >= 0
            ]
            if not positions:
                keep = max(len(START_MARKER), len(HELLO_MARKER)) - 1
                self._skip_non_packet_data(max(0, len(self.buffer) - keep))
                self._receive(deadline)
                continue
            position, kind = min(positions)
            self._skip_non_packet_data(position)

            if kind == 'hello':
                while True:
                    end = self.buffer.find(HELLO_END_MARKER, len(HELLO_MARKER))
                    if end >= 0:
                        if end > MAX_HELLO_BYTES:
                            del self.buffer[:1]
                            raise FrameProtocolError("Capability block exceeds size limit")
                        body = bytes(self.buffer[len(HELLO_MARKER):end])
                        del self.buffer[:end + len(HELLO_END_MARKER)]
                        # The hello trailer has a newline, unlike frame trailers.
                        if self.buffer[:2] == b'\r\n':
                            del self.buffer[:2]
                        elif self.buffer[:1] == b'\n':
                            del self.buffer[:1]
                        return 'hello', body
                    if len(self.buffer) > MAX_HELLO_BYTES:
                        del self.buffer[:1]
                        raise FrameProtocolError("Capability block exceeds size limit")
                    self._receive(deadline)

            header_size = len(START_MARKER) + FRAME_HEADER.size
            try:
                self._ensure(header_size, deadline)
                w, h, encoding, size = FRAME_HEADER.unpack_from(self.buffer, len(START_MARKER))
                if not (0 < w <= MAX_FRAME_DIMENSION and 0 < h <= MAX_FRAME_DIMENSION):
                    self._reject_frame(f"Invalid frame dimensions: {w}x{h}")
                if encoding not in (0, 1):
                    self._reject_frame(f"Unknown encoding: {encoding}")
                if encoding == 0 and size != w * h * 2:
                    self._reject_frame(f"Invalid raw payload size: {size} for {w}x{h}")
                if encoding == 1 and (size == 0 or size % 4 or size > w * h * 4):
                    self._reject_frame(f"Invalid RLE payload size: {size} for {w}x{h}")
                packet_size = header_size + size + len(END_MARKER)
                self._ensure(packet_size, deadline)
            except TimeoutError:
                self._reject_frame("Incomplete frame: timed out waiting for header/payload/trailer")
            except EOFError:
                self._reject_frame("Incomplete frame: end of recorded stream")

            if self.buffer[header_size + size:packet_size] != END_MARKER:
                self._reject_frame("Invalid frame trailer")
            raw = bytes(self.buffer[header_size:header_size + size])
            del self.buffer[:packet_size]
            return 'frame', (w, h, encoding, raw)


def parse_capabilities(body):
    caps = {}
    for line in body.decode('ascii', errors='replace').splitlines():
        if ':' in line:
            k, v = line.split(':', 1)
            caps[k.strip()] = v.strip()
    controls = {}
    for pair in caps.get('controls', '').split(','):
        if '=' in pair:
            char, action = pair.split('=', 1)
            controls[char.strip()] = action.strip()
    caps['controls'] = controls
    caps['width'] = int(caps.get('width', 320))
    caps['height'] = int(caps.get('height', 240))
    caps['prefix'] = int(caps.get('prefix', 1))
    caps['page_index'] = int(caps.get('page_index', -1))
    caps['pages'] = int(caps.get('pages', 0))
    caps['pixel_format'] = caps.get('pixel_format', 'RGB565')
    return caps

def read_capabilities_block(ser, timeout=2.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            kind, packet = ser.read_packet(deadline)
        except FrameProtocolError as error:
            print("Rejected packet during discovery:", error)
            continue
        if kind == 'hello':
            return parse_capabilities(packet)
    raise TimeoutError("No ==VIEWER-HELLO== received within timeout")

def try_read_capabilities_block(ser, timeout=0.25):
    try:
        return read_capabilities_block(ser, timeout=timeout)
    except TimeoutError:
        return None

import argparse

def discover_capabilities(ser):
    """Send '^', wait for ==VIEWER-HELLO== block, return parsed dict."""
    ser.write(b'^')
    ser.flush()
    caps = read_capabilities_block(ser)
    caps['pixel_format'] = caps.get('pixel_format', 'RGB565')
    return caps


def send_cmd(ser, prefix, cmd_char):
    ser.write(bytes([prefix, ord(cmd_char)]))

def read_frame_packet(ser, pixel_format, deadline=None):
    if deadline is None:
        deadline = time.monotonic() + 2.0
    while True:
        kind, packet = ser.read_packet(deadline)
        if kind == 'frame':
            break
        caps = parse_capabilities(packet)
        print(f"Device summary: page={caps['page_index'] + 1}/{caps['pages']} "
              f"title={caps.get('page_title', '')} profile={caps.get('profile', '')}")
        if caps.get('serial_output'):
            print("Serial output:", caps['serial_output'])
    w, h, encoding, raw = packet
    if encoding == 0:
        data16 = np.frombuffer(raw, dtype='<u2')
    else:
        total = w * h
        pixels = np.empty(total, dtype=np.uint16)
        filled = 0
        for cnt, val in struct.iter_unpack('<HH', raw):
            if cnt == 0 or filled + cnt > total:
                ser.rejected_frames += 1
                raise FrameProtocolError(f"Invalid RLE run: count={cnt}, filled={filled}, total={total}")
            pixels[filled:filled+cnt] = val
            filled += cnt
        if filled != total:
            ser.rejected_frames += 1
            raise FrameProtocolError(f"Incomplete RLE frame: expected {total} pixels, got {filled}")
        data16 = pixels

    if pixel_format == 'BGR565':
        d = data16.byteswap()
        r16 = ((d >> 11) & 0x1F).astype(np.uint32)
        g16 = ((d >> 5)  & 0x3F).astype(np.uint32)
        b16 = ( d        & 0x1F).astype(np.uint32)
    else:
        r16 = ((data16 >> 11) & 0x1F).astype(np.uint32)
        g16 = ((data16 >> 5)  & 0x3F).astype(np.uint32)
        b16 = ( data16        & 0x1F).astype(np.uint32)
    r8 = (r16 * 255 // 31).astype(np.uint8).reshape(h, w)
    g8 = (g16 * 255 // 63).astype(np.uint8).reshape(h, w)
    b8 = (b16 * 255 // 31).astype(np.uint8).reshape(h, w)
    return np.dstack([b8, g8, r8])

def save_frame_png(frame, output_dir, page_index, page_title=None):
    output_dir.mkdir(parents=True, exist_ok=True)
    title_part = ""
    if page_title:
        safe_title = ''.join(ch if ch.isalnum() or ch in ('-', '_') else '_' for ch in page_title)[:24]
        title_part = f"_{safe_title}"
    fname = output_dir / f"page_{page_index:03d}{title_part}.png"
    cv2.imwrite(str(fname), frame)
    print("Saved", fname)
    return fname

def find_git_root(start_path):
    current = Path(start_path).resolve()
    if current.is_file():
        current = current.parent
    for candidate in [current] + list(current.parents):
        if (candidate / '.git').exists():
            return candidate
    return None

def get_git_summary(start_path):
    root = find_git_root(start_path)
    if root is None:
        return {
            'root': '',
            'branch': 'unknown',
            'commit': 'unknown',
            'dirty': 'unknown',
        }

    def run_git(*args):
        result = subprocess.run(['git', '-C', str(root), *args], capture_output=True, text=True)
        if result.returncode != 0:
            return ''
        return result.stdout.strip()

    status = run_git('status', '--porcelain')
    return {
        'root': str(root),
        'branch': run_git('rev-parse', '--abbrev-ref', 'HEAD') or 'unknown',
        'commit': run_git('rev-parse', '--short', 'HEAD') or 'unknown',
        'dirty': 'dirty' if status else 'clean',
    }

def parse_profile_fps(profile_text):
    if not profile_text:
        return None
    token = profile_text.strip().split('fps', 1)[0].strip()
    digits = ''
    for ch in token:
        if ch.isdigit() or ch == '.':
            digits += ch
        elif digits:
            break
    try:
        return float(digits) if digits else None
    except ValueError:
        return None

def write_sweep_report(report_path, run_info, page_entries):
    report_lines = []
    report_lines.append(f"# Remote Viewer Sweep Report\n")
    report_lines.append(f"- port: {run_info.get('port', '')}")
    report_lines.append(f"- board: {run_info.get('board', 'unknown')}")
    report_lines.append(f"- cpu_mhz: {run_info.get('cpu_mhz', '?')}")
    report_lines.append(f"- pixel_format: {run_info.get('pixel_format', 'RGB565')}")
    report_lines.append(f"- pages: {run_info.get('pages', 0)}")
    report_lines.append(f"- git_branch: {run_info.get('git_branch', 'unknown')}")
    report_lines.append(f"- git_commit: {run_info.get('git_commit', 'unknown')}")
    report_lines.append(f"- git_dirty: {run_info.get('git_dirty', 'unknown')}")
    report_lines.append(f"- build_flags: {run_info.get('build_flags', '')}")
    report_lines.append(f"- feature_flags: {run_info.get('flags', '')}")
    if run_info.get('sweep_elapsed_sec'):
        elapsed = float(run_info['sweep_elapsed_sec'])
        report_lines.append(f"- sweep_elapsed_sec: {elapsed:.2f}")
        if run_info.get('pages', 0):
            report_lines.append(f"- capture_rate: {run_info['pages'] / elapsed:.2f} pages/s")
    if run_info.get('fps_values'):
        avg_fps = sum(run_info['fps_values']) / len(run_info['fps_values'])
        report_lines.append(f"- average_fps: {avg_fps:.2f}")
    report_lines.append("")

    for entry in page_entries:
        report_lines.append(f"## Page {entry['page_index']:03d}: {entry.get('page_title', '')}")
        report_lines.append(f"- fps: {entry.get('fps', 'n/a')}")
        report_lines.append(f"- profile: {entry.get('profile', '')}")
        report_lines.append(f"- screenshot: {entry['image_name']}")
        report_lines.append("")
        report_lines.append(f"![Page {entry['page_index']:03d}]({entry['image_name']})")
        report_lines.append("")

    report_path.write_text("\n".join(report_lines), encoding='utf-8')
    print("Wrote report", report_path)
    return report_path


def run_headless(ser, pixel_format, args):
    output_dir = Path(args.screenshot_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    duration = args.duration_seconds if args.duration_seconds is not None else 10.0
    deadline = started + duration
    next_snapshot = started
    frames = 0
    snapshots = 0
    last_snapshot_frame = 0
    frame = None
    print(f"Headless observation: {duration:g}s; snapshots in {output_dir}")
    while time.monotonic() < deadline:
        if not ser.in_waiting:
            if args.replay:
                break
            time.sleep(0.01)
            continue
        try:
            frame = read_frame_packet(ser, pixel_format, min(deadline, time.monotonic() + 2.0))
        except FrameProtocolError as error:
            print("Rejected frame:", error)
            continue
        except TimeoutError as error:
            print("Viewer read timeout:", error)
            continue
        except EOFError:
            if args.replay:
                break
            raise
        frames += 1
        now = time.monotonic()
        if now >= next_snapshot:
            snapshots += 1
            snapshot = output_dir / f"frame_{snapshots:06d}.png"
            for path in (snapshot, output_dir / "latest.png"):
                if not cv2.imwrite(str(path), frame):
                    raise IOError(f"Could not write snapshot: {path}")
            print(f"Frame {frames}: saved {snapshot}")
            last_snapshot_frame = frames
            next_snapshot = now + args.snapshot_interval_seconds
    if frames and frames != last_snapshot_frame:
        snapshots += 1
        for path in (output_dir / f"frame_{snapshots:06d}.png", output_dir / "latest.png"):
            if not cv2.imwrite(str(path), frame):
                raise IOError(f"Could not write snapshot: {path}")
    elapsed = time.monotonic() - started
    print(f"Observation complete: frames={frames}, rejected={ser.rejected_frames}, "
          f"skipped_bytes={ser.skipped_bytes}, elapsed={elapsed:.2f}s")
    if not frames:
        raise IOError("Observation received no valid frames")


def run_viewer(ser, args):
    print("Requesting device capabilities...")
    caps = read_capabilities_block(ser) if args.replay else discover_capabilities(ser)
    prefix        = caps['prefix']
    controls      = caps['controls']
    dev_w         = caps['width']
    dev_h         = caps['height']
    pixel_format  = caps['pixel_format']
    print(f"Connected: {dev_w}x{dev_h}, prefix=0x{prefix:02x}, pixel_format={pixel_format}, controls={controls}")
    if caps.get('pages', 0):
        print(f"Pages: {caps.get('page_index', -1) + 1}/{caps.get('pages')}, title={caps.get('page_title', '')}")
    if caps.get('board') or caps.get('cpu_mhz'):
        print(f"HW: board={caps.get('board', 'unknown')} cpu_mhz={caps.get('cpu_mhz', '?')}")
    if caps.get('flags'):
        print(f"Flags: {caps.get('flags')}")
    if caps.get('build_flags'):
        print(f"Build flags: {caps.get('build_flags')}")
    if caps.get('serial_output'):
        print("Serial output:", caps['serial_output'])

    if args.headless:
        run_headless(ser, pixel_format, args)
        return

    # Live mode is started automatically by the device on '^' receipt.
    win_name = "Remote Viewer"
    cv2.namedWindow(win_name, cv2.WINDOW_NORMAL)
    cv2.setWindowTitle(win_name, f"Remote Viewer {args.port} {dev_w}\u00d7{dev_h}")
    double_size = False
    frame = None

    # human-readable key hints based on discovered controls
    action_to_key = {v: k for k, v in controls.items()}
    hints = (
        f"'s'=save  '{action_to_key.get('knob_left','d')}'=left  '{action_to_key.get('knob_right','f')}'=right  "
        f"'{action_to_key.get('button_back','a')}'=back  '{action_to_key.get('button_select','b')}'=select  "
        + (f"'{action_to_key.get('button_right','n')}'=right-btn  " if 'button_right' in action_to_key else "")
        + "'['=prev-page  ']'=next-page  '?'=summary  'e'=frame  'L'=live  'c'=clipboard  'z'=zoom  'q'=quit"
    )
    print(hints)

    benchmark_frames = 0
    benchmark_started = None
    auto_sweep = args.auto_sweep_pages
    output_dir = Path(args.screenshot_dir) if args.screenshot_dir else None
    page_dwell_ms = args.page_dwell_ms

    def handle_frame(frame):
        nonlocal benchmark_frames
        benchmark_frames += 1
        if double_size:
            display_frame = cv2.resize(frame, (frame.shape[1]*2, frame.shape[0]*2), interpolation=cv2.INTER_NEAREST)
        else:
            display_frame = frame
        cv2.resizeWindow(win_name, display_frame.shape[1], display_frame.shape[0])
        cv2.imshow(win_name, display_frame)

    if auto_sweep:
        total_pages = max(1, int(caps.get('pages', 1)))
        if output_dir is None:
            output_dir = Path(f"screenshots_{datetime.now().strftime('%Y%m%d_%H%M%S')}")
        print(f"Auto sweep enabled: {total_pages} pages, dwell={page_dwell_ms}ms")
        print(f"Sweep output dir: {output_dir}")
        benchmark_started = time.time()
        git_info = get_git_summary(Path(__file__).resolve().parent)
        page_entries = []
        run_info = {
            'port': args.port,
            'board': caps.get('board', 'unknown'),
            'cpu_mhz': caps.get('cpu_mhz', '?'),
            'pixel_format': pixel_format,
            'pages': total_pages,
            'git_branch': git_info['branch'],
            'git_commit': git_info['commit'],
            'git_dirty': git_info['dirty'],
            'build_flags': caps.get('build_flags', ''),
            'flags': caps.get('flags', ''),
            'fps_values': [],
        }
        last_page_title = caps.get('page_title', '')
        for page_step in range(total_pages):
            send_cmd(ser, prefix, 'X')
            cv2.waitKey(1)
            time.sleep(page_dwell_ms / 1000.0)
            summary_after_dwell = try_read_capabilities_block(ser, timeout=0.25)
            if summary_after_dwell is not None:
                caps = summary_after_dwell
                last_page_title = caps.get('page_title', last_page_title)
            else:
                send_cmd(ser, prefix, '?')
                summary_after_dwell = read_capabilities_block(ser, timeout=2.0)
                caps = summary_after_dwell
                last_page_title = caps.get('page_title', last_page_title)

            send_cmd(ser, prefix, 'e')
            frame = read_frame_packet(ser, pixel_format)
            handle_frame(frame)
            image_path = save_frame_png(frame, output_dir, page_step + 1, last_page_title)

            fps_value = parse_profile_fps(caps.get('profile', ''))
            if fps_value is not None:
                run_info['fps_values'].append(fps_value)

            page_entries.append({
                'page_index': page_step + 1,
                'page_title': last_page_title,
                'fps': fps_value if fps_value is not None else 'n/a',
                'profile': caps.get('profile', ''),
                'build_flags': caps.get('build_flags', ''),
                'image_name': image_path.name,
            })

            if page_step < total_pages - 1:
                send_cmd(ser, prefix, ']')
                try_read_capabilities_block(ser, timeout=0.05)

        elapsed = time.time() - benchmark_started if benchmark_started else 0.0
        if elapsed > 0:
            print(f"Sweep complete: {total_pages} pages in {elapsed:.2f}s ({total_pages / elapsed:.2f} pages/s)")
            run_info['sweep_elapsed_sec'] = elapsed
        report_path = output_dir / "sweep_report.md"
        write_sweep_report(report_path, run_info, page_entries)
        cv2.destroyAllWindows()
        return

    stop_at = time.monotonic() + args.duration_seconds if args.duration_seconds is not None else None
    try:
        while True:
            if stop_at is not None and time.monotonic() >= stop_at:
                break
            key = cv2.waitKey(1) & 0xFF
            if key == ord('s'):
                if frame is not None:
                    ts = int(time.time())
                    fname = f"screenshot_{ts}.png"
                    cv2.imwrite(fname, frame)
                    print("Saved", fname)
            elif key in [ord('d'), ord('f'), ord('a'), ord('b'), ord('e')]:
                print("Sending command for key", chr(key))
                send_cmd(ser, prefix, chr(key))
                if key != ord('e'):
                    send_cmd(ser, prefix, 'e')  # request frame after nav
            elif key == ord('n') and 'n' in controls:
                print("Sending button_right")
                send_cmd(ser, prefix, 'n')
                send_cmd(ser, prefix, 'e')
            elif key == ord('['):
                print("Sending previous page")
                send_cmd(ser, prefix, '[')
            elif key == ord(']'):
                print("Sending next page")
                send_cmd(ser, prefix, ']')
            elif key == ord('?'):
                print("Requesting summary block")
                send_cmd(ser, prefix, '?')
            elif key == ord('c'):
                if frame is not None:
                    print("Sending current frame to clipboard")
                    send_to_clipboard(Image.fromarray(cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)))
            elif key == ord('z'):
                double_size = not double_size
                print("Double-size display:", "ON" if double_size else "OFF")
            elif key == ord('L'):
                print("Toggling live mode")
                send_cmd(ser, prefix, 'L')
            elif key == ord('q'):
                break

            if not ser.in_waiting:
                continue

            try:
                read_deadline = time.monotonic() + 2.0
                if stop_at is not None:
                    read_deadline = min(read_deadline, stop_at)
                frame = read_frame_packet(ser, pixel_format, read_deadline)
                handle_frame(frame)
            except FrameProtocolError as error:
                print("Rejected frame:", error)
                continue
            except TimeoutError as error:
                print("Viewer read timeout:", error)
                continue

    except KeyboardInterrupt:
        pass
    finally:
        print(f"Viewing complete: frames={benchmark_frames}, rejected={ser.rejected_frames}, "
              f"skipped_bytes={ser.skipped_bytes}")
        cv2.destroyAllWindows()


def main(port, args):
    recording = None
    transport = None
    replay_source = None
    try:
        if args.replay:
            replay_source = Path(args.replay).open('rb')
            transport = ReplayTransport(replay_source)
        else:
            transport = serial.Serial(port, BAUD, timeout=0.1, write_timeout=2)
            time.sleep(0.1)
        if args.record_stream:
            recording = Path(args.record_stream).open('xb')
        ser = ViewerStream(transport, recording)
        run_viewer(ser, args)
    finally:
        if transport is not None and not args.replay:
            try:
                # The current firmware advertises prefix 1; stop live mode even
                # when discovery or window initialization fails.
                send_cmd(ViewerStream(transport), 1, 'X')
            except (serial.SerialException, OSError) as error:
                print("Could not stop live streaming:", error)
            finally:
                transport.close()
        if recording is not None:
            recording.close()
        if replay_source is not None:
            replay_source.close()


def positive_seconds(value):
    seconds = float(value)
    if not np.isfinite(seconds) or seconds <= 0:
        raise argparse.ArgumentTypeError("Must be a finite number greater than zero")
    return seconds


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Remote viewer for mymenu-based devices")
    parser.add_argument("port", nargs="?", default=PORT, help=f"Serial port (default: {PORT})")
    parser.add_argument("--auto-sweep-pages", action="store_true", help="step through every page and save a screenshot per page")
    parser.add_argument("--page-dwell-ms", type=int, default=2500, help="milliseconds to stay on each page during auto sweep")
    parser.add_argument("--screenshot-dir", default="screenshots", help="output directory for sweep or headless screenshots")
    parser.add_argument("--headless", action="store_true", help="observe without a window; save sampled frames and latest.png")
    parser.add_argument("--duration-seconds", type=positive_seconds, help="stop viewing after this many seconds (headless default: 10)")
    parser.add_argument("--snapshot-interval-seconds", type=positive_seconds, default=1.0, help="headless snapshot interval (default: 1)")
    parser.add_argument("--record-stream", help="save received bytes to a new binary file for offline replay")
    parser.add_argument("--replay", help="replay a recorded stream instead of opening a COM port (requires --headless)")
    args = parser.parse_args()
    if args.replay and not args.headless:
        parser.error("--replay requires --headless")
    if args.replay and args.record_stream:
        parser.error("--replay and --record-stream cannot be combined")
    if args.headless and args.auto_sweep_pages:
        parser.error("--headless and --auto-sweep-pages cannot be combined")
    if args.auto_sweep_pages and args.duration_seconds is not None:
        parser.error("--duration-seconds cannot be combined with --auto-sweep-pages")

    print(f"Starting viewer on port {args.port}")
    main(args.port, args)