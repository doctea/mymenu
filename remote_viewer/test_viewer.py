import contextlib
import io
import struct
import tempfile
import time
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import DEFAULT, patch

import numpy as np

import viewer


def packet(w=2, h=1, encoding=0, payload=None, size=None):
    if payload is None:
        payload = struct.pack('<HH', 0xF800, 0x07E0)
    if size is None:
        size = len(payload)
    return (viewer.START_MARKER + struct.pack('<HHBI', w, h, encoding, size)
            + payload + viewer.END_MARKER)


HELLO = (viewer.HELLO_MARKER + b'\r\nwidth:2\r\nheight:1\r\nprefix:1\r\n'
         b'pixel_format:RGB565\r\ncontrols:d=knob_left,f=knob_right\r\n'
         + viewer.HELLO_END_MARKER + b'\r\n')


class FakeSerial:
    def __init__(self, data=b'', chunk_size=65536):
        self.data = bytearray(data)
        self.chunk_size = chunk_size
        self.writes = []
        self.closed = False

    @property
    def in_waiting(self):
        return len(self.data)

    def read(self, size):
        size = min(size, self.chunk_size)
        chunk = bytes(self.data[:size])
        del self.data[:size]
        if not chunk:
            time.sleep(0.001)
        return chunk

    def write(self, data):
        self.writes.append(data)
        return len(data)

    def flush(self):
        pass

    def close(self):
        self.closed = True


class ViewerTests(unittest.TestCase):
    def setUp(self):
        self.output = io.StringIO()
        self.redirect = contextlib.redirect_stdout(self.output)
        self.redirect.__enter__()
        self.addCleanup(self.redirect.__exit__, None, None, None)

    def test_fragmented_hello_keeps_following_frames_and_consumes_trailers(self):
        for chunk_size in (1, 7, 65536):
            with self.subTest(chunk_size=chunk_size):
                stream = viewer.ViewerStream(FakeSerial(HELLO + packet() + packet(), chunk_size))
                caps = viewer.read_capabilities_block(stream)
                self.assertEqual(caps['controls']['d'], 'knob_left')
                for _ in range(2):
                    frame = viewer.read_frame_packet(stream, caps['pixel_format'])
                    np.testing.assert_array_equal(frame, [[[0, 0, 255], [0, 255, 0]]])
                self.assertEqual(stream.in_waiting, 0)
                self.assertEqual(stream.rejected_frames, 0)
                # A fragmented hello CRLF may be logged separately, but not a frame trailer.
                self.assertNotIn('END-FRAME', self.output.getvalue())

    def test_console_and_hello_between_frames_do_not_corrupt_pixels(self):
        stream = viewer.ViewerStream(FakeSerial(packet() + b'debug: hello\r\n' + HELLO + packet()))
        first = viewer.read_frame_packet(stream, 'RGB565')
        second = viewer.read_frame_packet(stream, 'RGB565')
        np.testing.assert_array_equal(first, second)
        self.assertIn('Device summary:', self.output.getvalue())
        self.assertIn('debug: hello', self.output.getvalue())
        self.assertEqual(stream.rejected_frames, 0)

    def test_rle_equal_to_raw_size_is_still_decoded_as_rle(self):
        stream = viewer.ViewerStream(FakeSerial(packet(encoding=1, payload=struct.pack('<HH', 2, 0xF800))))
        frame = viewer.read_frame_packet(stream, 'RGB565')
        np.testing.assert_array_equal(frame, [[[0, 0, 255], [0, 0, 255]]])

    def test_invalid_headers_are_rejected_before_payload_read_and_recover(self):
        cases = (
            dict(w=0), dict(h=1025), dict(encoding=7),
            dict(size=0xFFFFFFFF), dict(size=3),
            dict(encoding=1, size=0), dict(encoding=1, size=3),
            dict(encoding=1, size=12),
        )
        for case in cases:
            with self.subTest(case=case):
                stream = viewer.ViewerStream(FakeSerial(packet(**case) + packet()))
                with self.assertRaises(viewer.FrameProtocolError):
                    viewer.read_frame_packet(stream, 'RGB565')
                frame = viewer.read_frame_packet(stream, 'RGB565')
                np.testing.assert_array_equal(frame, [[[0, 0, 255], [0, 255, 0]]])
                self.assertEqual(stream.rejected_frames, 1)

    def test_invalid_rle_never_returns_partial_pixels_and_next_frame_survives(self):
        for runs in (((0, 0xF800),), ((3, 0xF800),), ((1, 0xF800),),
                     ((2, 0xF800), (1, 0x07E0))):
            with self.subTest(runs=runs):
                raw = b''.join(struct.pack('<HH', *run) for run in runs)
                stream = viewer.ViewerStream(FakeSerial(packet(encoding=1, payload=raw) + packet()))
                with self.assertRaises(viewer.FrameProtocolError):
                    viewer.read_frame_packet(stream, 'RGB565')
                frame = viewer.read_frame_packet(stream, 'RGB565')
                self.assertEqual(frame.shape, (1, 2, 3))
                self.assertEqual(stream.rejected_frames, 1)

    def test_bad_trailer_and_truncated_payload_resynchronize(self):
        bad_packets = (
            packet()[:-len(viewer.END_MARKER)] + b'corrupt trailer!',
            packet()[:len(viewer.START_MARKER) + viewer.FRAME_HEADER.size + 1],
        )
        for bad in bad_packets:
            with self.subTest(bad=bad):
                stream = viewer.ViewerStream(FakeSerial(bad + packet()))
                with self.assertRaises(viewer.FrameProtocolError):
                    viewer.read_frame_packet(stream, 'RGB565')
                frame = viewer.read_frame_packet(stream, 'RGB565')
                self.assertEqual(frame.shape, (1, 2, 3))
                self.assertEqual(stream.rejected_frames, 1)

    def test_marker_bytes_inside_valid_payload_are_not_treated_as_packets(self):
        raw = viewer.START_MARKER + viewer.HELLO_MARKER + b'\0'
        if len(raw) % 2:
            raw += b'\0'
        stream = viewer.ViewerStream(FakeSerial(packet(w=len(raw) // 2, payload=raw), 3))
        kind, decoded = stream.read_packet()
        self.assertEqual(kind, 'frame')
        self.assertEqual(decoded[3], raw)
        self.assertEqual(stream.skipped_bytes, 0)

    def test_partial_packet_timeout_rejects_and_allows_later_frame(self):
        transport = FakeSerial(packet()[:len(viewer.START_MARKER) + 2])
        stream = viewer.ViewerStream(transport)
        with self.assertRaisesRegex(viewer.FrameProtocolError, 'Incomplete frame'):
            viewer.read_frame_packet(stream, 'RGB565', time.monotonic() + 0.01)
        transport.data.extend(packet())
        self.assertEqual(viewer.read_frame_packet(stream, 'RGB565').shape, (1, 2, 3))
        self.assertEqual(stream.rejected_frames, 1)

    def test_truncated_replay_tail_is_reported_as_a_rejected_frame(self):
        data = packet() + packet()[:-len(viewer.END_MARKER)]
        stream = viewer.ViewerStream(viewer.ReplayTransport(io.BytesIO(data)))
        viewer.read_frame_packet(stream, 'RGB565')
        with self.assertRaisesRegex(viewer.FrameProtocolError, 'end of recorded stream'):
            viewer.read_frame_packet(stream, 'RGB565')
        self.assertEqual(stream.rejected_frames, 1)

    def test_recording_replays_and_saves_last_valid_frame_without_a_window(self):
        data = HELLO + packet() + packet(payload=struct.pack('<HH', 0x001F, 0x001F))
        recording = io.BytesIO()
        stream = viewer.ViewerStream(FakeSerial(data), recording)
        caps = viewer.read_capabilities_block(stream)
        viewer.read_frame_packet(stream, caps['pixel_format'])
        viewer.read_frame_packet(stream, caps['pixel_format'])
        self.assertEqual(recording.getvalue(), data)
        replay = viewer.ViewerStream(viewer.ReplayTransport(io.BytesIO(recording.getvalue())))
        caps = viewer.read_capabilities_block(replay)
        with tempfile.TemporaryDirectory() as directory:
            args = SimpleNamespace(screenshot_dir=directory, duration_seconds=1,
                                   snapshot_interval_seconds=1, replay=True)
            with patch.object(viewer.cv2, 'namedWindow', side_effect=AssertionError('GUI opened')):
                viewer.run_headless(replay, caps['pixel_format'], args)
            latest = viewer.cv2.imread(str(Path(directory) / 'latest.png'))
            np.testing.assert_array_equal(latest, [[[255, 0, 0], [255, 0, 0]]])
            self.assertEqual(len(list(Path(directory).glob('frame_*.png'))), 2)
        self.assertIn('frames=2, rejected=0', self.output.getvalue())

    def test_failed_discovery_closes_port_and_stops_live_streaming(self):
        transport = FakeSerial()
        args = SimpleNamespace(replay=None, record_stream=None)
        with patch.object(viewer.serial, 'Serial', return_value=transport), \
                patch.object(viewer, 'run_viewer', side_effect=RuntimeError('discovery failed')):
            with self.assertRaisesRegex(RuntimeError, 'discovery failed'):
                viewer.main('COM_TEST', args)
        self.assertTrue(transport.closed)
        self.assertEqual(transport.writes, [b'\x01X'])

    def test_interactive_viewer_still_opens_renders_and_quits(self):
        stream = viewer.ViewerStream(FakeSerial(HELLO + packet()))
        args = SimpleNamespace(port='COM_TEST', replay=None, headless=False,
                               auto_sweep_pages=False, screenshot_dir='screenshots',
                               page_dwell_ms=2500, duration_seconds=None)
        with patch.object(viewer.cv2, 'waitKey', side_effect=[-1, ord('q')]), \
                patch.multiple(viewer.cv2, namedWindow=DEFAULT, setWindowTitle=DEFAULT,
                               resizeWindow=DEFAULT, imshow=DEFAULT,
                               destroyAllWindows=DEFAULT) as windows:
            viewer.run_viewer(stream, args)
        windows['namedWindow'].assert_called_once()
        windows['destroyAllWindows'].assert_called_once()
        displayed = windows['imshow'].call_args.args[1]
        np.testing.assert_array_equal(displayed, [[[0, 0, 255], [0, 255, 0]]])
        self.assertIn('Viewing complete: frames=1, rejected=0', self.output.getvalue())

    def test_page_sweep_still_saves_a_frame_and_report(self):
        stream = viewer.ViewerStream(FakeSerial(HELLO + packet()))
        with tempfile.TemporaryDirectory() as directory:
            args = SimpleNamespace(port='COM_TEST', replay=None, headless=False,
                                   auto_sweep_pages=True, screenshot_dir=directory,
                                   page_dwell_ms=0, duration_seconds=None)
            with patch.object(viewer, 'try_read_capabilities_block',
                              return_value=viewer.parse_capabilities(HELLO[len(viewer.HELLO_MARKER):])), \
                    patch.object(viewer, 'get_git_summary',
                                 return_value=dict(branch='test', commit='test', dirty='clean')), \
                    patch.multiple(viewer.cv2, namedWindow=DEFAULT, setWindowTitle=DEFAULT,
                                   waitKey=DEFAULT, resizeWindow=DEFAULT, imshow=DEFAULT,
                                   destroyAllWindows=DEFAULT):
                viewer.run_viewer(stream, args)
            saved = viewer.cv2.imread(str(Path(directory) / 'page_001.png'))
            np.testing.assert_array_equal(saved, [[[0, 0, 255], [0, 255, 0]]])
            self.assertIn('- port: COM_TEST', (Path(directory) / 'sweep_report.md').read_text())


if __name__ == '__main__':
    unittest.main()
