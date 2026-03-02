import board
import sys
import digitalio
import binascii
from eeprom import EepromDevice
import os

FIRMWARE = os.getenv("FIRMWARE")
SERIAL_NUM = os.getenv("SERIAL_NUM")

led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT

eeprom = EepromDevice(scl_pin=board.GP5, sda_pin=board.GP4)

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

    else:
        print("error_unrecognized: unrecognized value received ('" + line + "')")

    led.value = not led.value