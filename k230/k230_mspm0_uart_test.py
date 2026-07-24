"""CanMV-K230-LP4 V3.0 <-> MSPM0G3507 UART communication test.

Wiring:
    K230 GPIO3 (UART1_TX) -> MSPM0 PB3 (UART3_RX)
    K230 GPIO4 (UART1_RX) <- MSPM0 PB2 (UART3_TX)
    K230 GND              -- MSPM0 GND

Serial settings: 115200 baud, 8 data bits, no parity, 1 stop bit.
"""

from machine import FPIOA, UART
import time


UART_BAUDRATE = 115200
RESPONSE_TIMEOUT_MS = 800
SEND_INTERVAL_MS = 1000
MAX_RX_BUFFER_SIZE = 256


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


def run_test():
    uart = configure_uart()
    sequence = 0
    rx_buffer = b""
    success_count = 0
    timeout_count = 0
    error_count = 0

    print("[UART] UART1 initialized")
    print("[UART] TX=GPIO3 RX=GPIO4 115200-8N1")

    try:
        while True:
            request = ("PING,%d\r\n" % sequence).encode()
            expected = ("PONG,%d" % sequence).encode()

            uart.write(request)
            print("[TX]", request)

            matched = False
            deadline = time.ticks_add(
                time.ticks_ms(), RESPONSE_TIMEOUT_MS
            )

            while time.ticks_diff(deadline, time.ticks_ms()) > 0:
                data = uart.read()
                if data:
                    rx_buffer += data

                    if len(rx_buffer) > MAX_RX_BUFFER_SIZE:
                        print("[ERROR] RX buffer overflow; clearing buffer")
                        rx_buffer = b""
                        error_count += 1

                    while b"\r\n" in rx_buffer:
                        line, rx_buffer = rx_buffer.split(b"\r\n", 1)
                        print("[RX]", line)

                        if line == expected:
                            success_count += 1
                            matched = True
                            print(
                                "[OK] sequence=%d success=%d timeout=%d error=%d"
                                % (
                                    sequence,
                                    success_count,
                                    timeout_count,
                                    error_count,
                                )
                            )
                            break

                        error_count += 1
                        print("[ERROR] expected", expected, "but received", line)

                if matched:
                    break

                time.sleep_ms(10)

            if not matched:
                timeout_count += 1
                print(
                    "[TIMEOUT] sequence=%d success=%d timeout=%d error=%d"
                    % (
                        sequence,
                        success_count,
                        timeout_count,
                        error_count,
                    )
                )

            sequence = (sequence + 1) & 0xFFFFFFFF

            elapsed_ms = RESPONSE_TIMEOUT_MS if not matched else 0
            remaining_ms = SEND_INTERVAL_MS - elapsed_ms
            if remaining_ms > 0:
                time.sleep_ms(remaining_ms)

    except KeyboardInterrupt:
        print("[UART] test stopped by user")
    finally:
        uart.deinit()
        print("[UART] UART1 released")


run_test()

