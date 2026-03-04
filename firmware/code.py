import board
import sys
import digitalio
import binascii
from eeprom import EepromDevice
import os
from UART_fingerprint import FingerprintSensor

FIRMWARE = os.getenv("FIRMWARE")
SERIAL_NUM = os.getenv("SERIAL_NUM")

led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT

eeprom = EepromDevice(scl_pin=board.GP5, sda_pin=board.GP4)
fingerprint = FingerprintSensor(uart_tx=board.GP0, uart_rx=board.GP1, rst_pin=board.GP22, wake_pin=board.GP21)

# Stores whether fingerprint has been authenticated this session
authenticated = False

while True:
    line = sys.stdin.readline().rstrip('\r\n')

    if len(line) <= 0:
        continue

    if line == 'pc_hello':
        print('usb_key_hello')
    elif line == 'pc_req_sn':
        print(SERIAL_NUM)
    elif line == 'pc_req_fw':
        print(FIRMWARE)
    elif line == 'pc_req_key':
        if not authenticated:
            print('error_auth_req')
            continue

        # Read length (first 2 bytes)
        try:
            len_hi = eeprom.read_byte(0)
            len_lo = eeprom.read_byte(1)
            length = (len_hi << 8) | len_lo
            
            if 0 < length <= 510:
                # Read key bytes
                raw_bytes = bytearray(length)
                for i in range(length):
                    raw_bytes[i] = eeprom.read_byte(i + 2)
                
                # Send key to PC
                print(raw_bytes.decode('utf-8'))
            else:
                print("error_invalid_length")
        except:
            print("error_eeprom_read")
    elif line == 'pc_enroll_fingerprint':
        # TODO: Only allow this if either no fingerprints already registered or
        #       if one is already registered, require finger scan to allow this
        #       operation
        fingerprint.decode_request(0x01)
    elif line == 'pc_authenticate_fingerprint':
        authenticated = (fingerprint.decode_request(0x0C) == 0)

    else:
        print("error_unrecognized: unrecognized value received ('" + line + "')")

    led.value = not led.value
