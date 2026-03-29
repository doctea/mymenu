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

            raw = np.frombuffer(raw, dtype=np.uint16).byteswap().astype(np.uint16)

            # read end marker
            end = read_exact(ser, len("==END-FRAME=="))
            if end != b'==END-FRAME==':
                print("Bad packet end marker, skipping")
                continue

            # Convert RGB565 raw -> RGB888 using Pillow
            # Pillow expects little-endian 16-bit BGR565 as "BGR;16"
            img = Image.frombytes("RGB", (w, h), raw, "raw", "BGR;16")
            arr = np.array(img)  # RGB

            frame = cv2.cvtColor(arr, cv2.COLOR_RGB2BGR)
            cv2.imshow("RP2350 Live", frame)


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
    print("Controls are 's' to save screenshot, 'd'=left/'f'=right/'a'=back/'b'=select/'e'=send frame to send commands, 'q' to quit")

    main(port)