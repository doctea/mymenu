# viewer.py
import serial
import struct
from PIL import Image
import numpy as np
import cv2
import time
import win32clipboard

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

import argparse

def discover_capabilities(ser):
    """Send '^', wait up to 2 s for ==VIEWER-HELLO== block, return parsed dict."""
    ser.write(b'^')
    ser.flush()
    deadline = time.time() + 2.0
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
                return {
                    'width':        int(caps.get('width',  320)),
                    'height':       int(caps.get('height', 240)),
                    'prefix':       int(caps.get('prefix',   1)),
                    'pixel_format': caps.get('pixel_format', 'RGB565'),
                    'controls':     controls,
                }
            elif in_hello:
                lines.append(text)
    raise TimeoutError("No ==VIEWER-HELLO== received within 2 s; is the device running with ENABLE_REMOTE_VIEWER?")


def send_cmd(ser, prefix, cmd_char):
    ser.write(bytes([prefix, ord(cmd_char)]))


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
        + "'e'=frame  'L'=live  'c'=clipboard  'z'=zoom  'q'=quit"
    )
    print(hints)

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

            read_until_start_marker(ser)

            w = struct.unpack('<H', read_exact(ser, 2))[0]
            h = struct.unpack('<H', read_exact(ser, 2))[0]
            encoding_b = read_exact(ser, 1)
            if not encoding_b:
                print("Failed to read encoding byte")
                continue
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
                print("Bad packet end marker, skipping")
                continue

            # Decode RGB565 or BGR565 uint16 array -> BGR uint8 for OpenCV
            if pixel_format == 'BGR565':
                # TFT_eSPI sends pixels big-endian (byte-swapped for SPI DMA),
                # but the logical colour format is still RGB565 (R in high bits).
                # Byteswap restores the correct uint16 value, then extract as RGB565.
                d = data16.byteswap()
                r16 = ((d >> 11) & 0x1F).astype(np.uint32)
                g16 = ((d >> 5)  & 0x3F).astype(np.uint32)
                b16 = ( d        & 0x1F).astype(np.uint32)
            else:  # RGB565 native LE (e.g. ILI9341_t3n)
                r16 = ((data16 >> 11) & 0x1F).astype(np.uint32)
                g16 = ((data16 >> 5)  & 0x3F).astype(np.uint32)
                b16 = ( data16        & 0x1F).astype(np.uint32)
            r8 = (r16 * 255 // 31).astype(np.uint8).reshape(h, w)
            g8 = (g16 * 255 // 63).astype(np.uint8).reshape(h, w)
            b8 = (b16 * 255 // 31).astype(np.uint8).reshape(h, w)
            frame = np.dstack([b8, g8, r8])  # BGR for OpenCV

            if double_size:
                display_frame = cv2.resize(frame, (frame.shape[1]*2, frame.shape[0]*2), interpolation=cv2.INTER_NEAREST)
            else:
                display_frame = frame
            try:
                cv2.resizeWindow(win_name, display_frame.shape[1], display_frame.shape[0])
            except Exception:
                pass
            cv2.imshow(win_name, display_frame)

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
    args = parser.parse_args()

    print(f"Starting viewer on port {args.port}")
    main(args.port)