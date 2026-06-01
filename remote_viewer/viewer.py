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

def read_exact(ser, n):
    buf = b''
    while len(buf) < n:
        chunk = ser.read(n - len(buf))
        if not chunk:
            raise IOError("Serial read timeout or disconnect")
        buf += chunk
    return buf

START_MARKER = b'==START-FRAME=='

def read_until_start_marker(ser):
    """Slide byte-by-byte through the serial stream until the start marker is
    found.  Any bytes that are *not* part of the marker are printed as debug
    garbage so the caller can see what arrived before the frame started."""
    window = b''
    garbage = b''
    while True:
        byte = ser.read(1)
        if not byte:
            raise IOError("Serial read timeout or disconnect")
        window += byte
        # Keep only the last len(START_MARKER) bytes in the sliding window
        if len(window) > len(START_MARKER):
            garbage += window[:1]
            window = window[-len(START_MARKER):]
        if window == START_MARKER:
            if garbage:
                print("Non-frame data skipped before marker:", garbage)
            return  # marker found, stream is now positioned right after it

def read_capabilities_block(ser, timeout=2.0):
    deadline = time.time() + timeout
    in_hello = False
    lines = []
    buf = b''
    while time.time() < deadline:
        chunk = ser.read(ser.in_waiting or 1)
        if not chunk:
            time.sleep(0.01)
            continue
        buf += chunk
        while b'\n' in buf:
            line, buf = buf.split(b'\n', 1)
            text = line.decode('ascii', errors='replace').strip()
            if text == '==VIEWER-HELLO==':
                in_hello = True
                lines = []
            elif in_hello and text == '==END-VIEWER-HELLO==':
                caps = {}
                for entry in lines:
                    if ':' in entry:
                        k, v = entry.split(':', 1)
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
                return caps
            elif in_hello:
                lines.append(text)
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

def read_frame_packet(ser, pixel_format):
    read_until_start_marker(ser)

    w = struct.unpack('<H', read_exact(ser, 2))[0]
    h = struct.unpack('<H', read_exact(ser, 2))[0]
    encoding_b = read_exact(ser, 1)
    if not encoding_b:
        raise IOError("Failed to read encoding byte")
    encoding = encoding_b[0]
    size = struct.unpack('<I', read_exact(ser, 4))[0]

    raw = read_exact(ser, size)

    expected_raw_bytes = w * h * 2
    if encoding == 0 or size == expected_raw_bytes:
        data16 = np.frombuffer(raw, dtype=np.uint16)
    elif encoding == 1:
        total = w * h
        pixels = np.empty(total, dtype=np.uint16)
        offset = 0
        filled = 0
        while filled < total and offset + 4 <= len(raw):
            cnt = struct.unpack('<H', raw[offset:offset+2])[0]
            val = struct.unpack('<H', raw[offset+2:offset+4])[0]
            offset += 4
            if filled + cnt > total:
                cnt = total - filled
            if cnt > 0:
                pixels[filled:filled+cnt] = val
                filled += cnt
        if filled != total:
            print(f"RLE decode error: expected {total} pixels, got {filled}")
            if size == expected_raw_bytes:
                data16 = np.frombuffer(raw, dtype=np.uint16)
            else:
                data16 = pixels
        else:
            data16 = pixels
    else:
        print(f"Unknown encoding {encoding}, trying raw")
        if size == expected_raw_bytes:
            data16 = np.frombuffer(raw, dtype=np.uint16)
        else:
            total = w * h
            pixels = np.empty(total, dtype=np.uint16)
            offset = 0
            filled = 0
            while filled < total and offset + 4 <= len(raw):
                cnt = struct.unpack('<H', raw[offset:offset+2])[0]
                val = struct.unpack('<H', raw[offset+2:offset+4])[0]
                offset += 4
                if filled + cnt > total:
                    cnt = total - filled
                if cnt > 0:
                    pixels[filled:filled+cnt] = val
                    filled += cnt
            data16 = pixels

    end = read_exact(ser, len("==END-FRAME=="))
    if end != b'==END-FRAME==':
        raise IOError("Bad packet end marker")

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


def main(port=PORT):
    ser = serial.Serial(port, BAUD, timeout=2)
    time.sleep(0.1)

    print("Requesting device capabilities...")
    caps = discover_capabilities(ser)
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

    # Live mode is started automatically by the device on '^' receipt.

    win_name = "Remote Viewer"
    cv2.namedWindow(win_name, cv2.WINDOW_NORMAL)
    cv2.setWindowTitle(win_name, f"Remote Viewer {port} {dev_w}\u00d7{dev_h}")
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

    auto_sweep = False
    output_dir = None
    page_dwell_ms = 2500
    benchmark_frames = 0
    benchmark_started = None
    if hasattr(main, "args"):
        auto_sweep = main.args.auto_sweep_pages
        output_dir = Path(main.args.screenshot_dir) if main.args.screenshot_dir else None
        page_dwell_ms = main.args.page_dwell_ms

    def handle_frame(frame):
        nonlocal benchmark_frames
        benchmark_frames += 1
        if double_size:
            display_frame = cv2.resize(frame, (frame.shape[1]*2, frame.shape[0]*2), interpolation=cv2.INTER_NEAREST)
        else:
            display_frame = frame
        try:
            cv2.resizeWindow(win_name, display_frame.shape[1], display_frame.shape[0])
        except Exception:
            pass
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
            'port': port,
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
        ser.close()
        cv2.destroyAllWindows()
        return

    try:
        while True:
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

            frame = read_frame_packet(ser, pixel_format)

            handle_frame(frame)

    except KeyboardInterrupt:
        pass
    finally:
        try:
            send_cmd(ser, prefix, 'X')
        except Exception:
            pass
        ser.close()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Remote viewer for mymenu-based devices")
    parser.add_argument("port", nargs="?", default=PORT, help=f"Serial port (default: {PORT})")
    parser.add_argument("--auto-sweep-pages", action="store_true", help="step through every page and save a screenshot per page")
    parser.add_argument("--page-dwell-ms", type=int, default=2500, help="milliseconds to stay on each page during auto sweep")
    parser.add_argument("--screenshot-dir", default="screenshots", help="output directory for auto-sweep screenshots")
    args = parser.parse_args()

    print(f"Starting viewer on port {args.port}")
    main.args = args
    main(args.port)