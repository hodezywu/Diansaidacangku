"""Detect a blue square and command an MSPM0 car over UART.

Wiring:
    K230 GPIO3 (UART1_TX) -> MSPM0G3507 PB3 (UART3_RX)
    K230 GPIO4 (UART1_RX) <- MSPM0G3507 PB2 (UART3_TX, optional)
    K230 GND               -- MSPM0 GND

Protocol:
    DRIVE,1\r\n means forward; DRIVE,0\r\n means stop.
"""

from machine import FPIOA, UART
from media.display import Display
from media.media import MediaManager
from media.sensor import Sensor
import time

FRAME_WIDTH = 320
FRAME_HEIGHT = 240
UART_BAUDRATE = 115200
BLUE_THRESHOLDS = [(15, 100, -45, 30, -80, -5)]
MIN_BLUE_PIXELS = 120
MIN_BLUE_AREA = 180
MIN_SIDE_PIXELS = 12
MIN_ASPECT_RATIO = 0.55
MAX_ASPECT_RATIO = 1.80
MIN_RECTANGULARITY = 0.45
REQUIRED_DETECT_FRAMES = 3
REQUIRED_LOST_FRAMES = 3


def configure_uart():
    fpioa = FPIOA()
    fpioa.set_function(3, FPIOA.UART1_TXD)
    fpioa.set_function(4, FPIOA.UART1_RXD)
    return UART(
        UART.UART1,
        baudrate=UART_BAUDRATE,
        bits=UART.EIGHTBITS,
        parity=UART.PARITY_NONE,
        stop=UART.STOPBITS_ONE,
    )


def is_blue_square(blob):
    width = blob[2]
    height = blob[3]
    if width < MIN_SIDE_PIXELS or height < MIN_SIDE_PIXELS:
        return False
    aspect_ratio = width / height
    rectangularity = blob[4] / (width * height)
    return (
        MIN_ASPECT_RATIO <= aspect_ratio <= MAX_ASPECT_RATIO
        and rectangularity >= MIN_RECTANGULARITY
    )


def find_blue_square(img):
    blobs = img.find_blobs(
        BLUE_THRESHOLDS,
        pixels_threshold=MIN_BLUE_PIXELS,
        area_threshold=MIN_BLUE_AREA,
        merge=True,
        margin=8,
    )
    best = None
    for blob in blobs:
        img.draw_rectangle(
            (blob[0], blob[1], blob[2], blob[3]),
            color=(255, 255, 0),
            thickness=1,
        )
        if is_blue_square(blob):
            if best is None or blob[4] > best[4]:
                best = blob
    if best is not None:
        img.draw_rectangle(
            (best[0], best[1], best[2], best[3]),
            color=(0, 255, 0),
            thickness=3,
        )
        img.draw_cross(best[5], best[6], color=(255, 255, 255), size=8)
    return best


def send_drive_command(uart, should_drive):
    uart.write(b"DRIVE,1\r\n" if should_drive else b"DRIVE,0\r\n")


def main():
    uart = configure_uart()
    sensor = Sensor()
    media_initialized = False
    display_initialized = False
    sensor_running = False
    detected_frames = 0
    lost_frames = 0
    driving = False
    previous_driving = False
    frame_count = 0

    try:
        sensor.reset()
        sensor.set_framesize(width=FRAME_WIDTH, height=FRAME_HEIGHT)
        sensor.set_pixformat(Sensor.RGB565)
        Display.init(
            Display.VIRT,
            width=FRAME_WIDTH,
            height=FRAME_HEIGHT,
            fps=30,
            to_ide=True,
        )
        display_initialized = True
        MediaManager.init()
        media_initialized = True
        sensor.run()
        sensor_running = True
        print("[VISION] blue-square detector started")
        print("[UART] GPIO3 TX / GPIO4 RX, 115200-8N1")

        while True:
            img = sensor.snapshot()
            found = find_blue_square(img) is not None
            if found:
                detected_frames = min(detected_frames + 1, REQUIRED_DETECT_FRAMES)
                lost_frames = 0
                if detected_frames >= REQUIRED_DETECT_FRAMES:
                    driving = True
            else:
                lost_frames = min(lost_frames + 1, REQUIRED_LOST_FRAMES)
                detected_frames = 0
                if lost_frames >= REQUIRED_LOST_FRAMES:
                    driving = False

            status_color = (0, 255, 0) if driving else (255, 0, 0)
            img.draw_string_advanced(
                4, 4, 18,
                "FORWARD" if driving else "STOP",
                color=status_color,
            )
            if found:
                for marker_index in range(detected_frames):
                    img.draw_rectangle(
                        (4 + marker_index * 10, 25, 7, 7),
                        color=(0, 255, 255),
                        fill=True,
                    )
            Display.show_image(img)
            send_drive_command(uart, driving)

            if driving != previous_driving:
                print("[STATE]", "FORWARD" if driving else "STOP", "blue_square=", found)
                previous_driving = driving
            frame_count += 1
            if frame_count % 60 == 0:
                print(
                    "[STATUS] frame=%d found=%s command=%s"
                    % (frame_count, str(found), "DRIVE,1" if driving else "DRIVE,0")
                )
    except KeyboardInterrupt:
        print("[VISION] stopped by user")
    except Exception as exc:
        print("[ERROR]", exc)
        raise
    finally:
        try:
            for _ in range(3):
                send_drive_command(uart, False)
                time.sleep_ms(10)
        except Exception:
            pass
        if sensor_running:
            sensor.stop()
        if display_initialized:
            Display.deinit()
        if media_initialized:
            MediaManager.deinit()
        uart.deinit()
        print("[SAFETY] stop command sent; resources released")


main()
