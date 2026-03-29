# viewer.py
import serial
import struct
from PIL import Image
import numpy as np
import cv2
import sys
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

def main(port=PORT):
    ser = serial.Serial(port, BAUD, timeout=2)
    time.sleep(0.1)
    # request live mode
    ser.write(b'L')

    cv2.namedWindow("RP2350 Live", cv2.WINDOW_NORMAL)
    double_size = False
    try:
        while True:
            #print("loop")
            key = cv2.waitKey(1) & 0xFF
            if key == ord('s'):
                # save screenshot
                ts = int(time.time())
                fname = f"screenshot_{ts}.png"
                cv2.imwrite(fname, frame)
                print("Saved", fname)
            elif key in [ord('d'), ord('f'), ord('a'), ord('b'), ord('e')]:
                print("Sending command for key", chr(key))
                ser.write(bytes([key]))
                ser.write(bytes([ord('e')]))  # send frame command after button press
            elif key == ord('c'):
                # send frame to clipboard
                print("Sending current frame to clipboard")
                send_to_clipboard(Image.fromarray(cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)))
            elif key == ord('z'):
                double_size = not double_size
                print("Double-size display:", "ON" if double_size else "OFF")
            elif key == ord('L'):
                print("Toggling live mode")
                ser.write(b'L')
            elif key == ord('q'):
                break

            if not ser.in_waiting:
                continue

            marker = ser.read(len("==START-FRAME=="))
            if not marker:
                continue
            if marker != b'==START-FRAME==':
                # ignore unexpected bytes
                # todo: if we receive any other information outside of the expected frame format, we should print it out to console for debugging purposes
                print("Unexpected data received, skipping:", marker)
                continue

            w = struct.unpack('<H', read_exact(ser, 2))[0]
            h = struct.unpack('<H', read_exact(ser, 2))[0]
            size = struct.unpack('<I', read_exact(ser, 4))[0]

            raw = read_exact(ser, size)

            # If payload length matches uncompressed 16-bit image, treat as raw
            # Otherwise assume RLE stream of [count:2][value:2] pairs and expand.
            expected_raw_bytes = w * h * 2
            if size == expected_raw_bytes:
                data16 = np.frombuffer(raw, dtype=np.uint16).byteswap()
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
                if filled != total:
                    print(f"RLE decode error: expected {total} pixels, got {filled}")
                data16 = pixels.byteswap()

            # read end marker
            end = read_exact(ser, len("==END-FRAME=="))
            if end != b'==END-FRAME==':
                print("Bad packet end marker, skipping")
                continue

            # Convert RGB565 raw -> RGB888 using Pillow
            # Pillow expects little-endian 16-bit BGR565 as "BGR;16"
            # pass bytes to Pillow (from the swapped 16-bit view)
            img = Image.frombytes("RGB", (w, h), data16.tobytes(), "raw", "BGR;16")
            arr = np.array(img)  # RGB

            frame = cv2.cvtColor(arr, cv2.COLOR_RGB2BGR)
            # optionally double the displayed size (nearest-neighbor)
            if double_size:
                display_frame = cv2.resize(frame, (frame.shape[1]*2, frame.shape[0]*2), interpolation=cv2.INTER_NEAREST)
            else:
                display_frame = frame
            # ensure the named window matches the image size (some backends don't auto-resize)
            try:
                cv2.resizeWindow("RP2350 Live", display_frame.shape[1], display_frame.shape[0])
            except Exception:
                pass
            cv2.imshow("RP2350 Live", display_frame)


    except KeyboardInterrupt:
        pass
    finally:
        # stop live mode
        try:
            ser.write(b'X')
        except:
            pass
        ser.close()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    port = sys.argv[1] if len(sys.argv) > 1 else PORT

    print("Starting viewer on port", port)
    print("Controls are 's' to save screenshot, 'd'=left/'f'=right/'a'=back/'b'=select/'e'=update frame/'z'=toggle double-size display, 'q' to quit")

    main(port)