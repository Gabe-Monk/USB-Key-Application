import board
import sys
import digitalio
from eeprom import EepromDevice

SERIAL_NUM = 0x0001
FIRMWARE = "1.0.0"

led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT

eeprom = EepromDevice(scl_pin=board.GP1, sda_pin=board.GP0)

# # Example: Write and read a byte
# print("Writing 0xDEADBEEF to address 0x10...")
# eeprom.write_word(0x10, 0xDEADBEEF)

# print("Reading back value...")
# val = eeprom.read_word(0x10)
# print("Read:", hex(val))

# print('Reading all non-default memory values (default is 0xFF)')
# i = 0
# while i <= 0xFF:
#     val = eeprom.read_byte(i)
#     if (val != 0xFF):
#         print(hex(i), ":", hex(val))
#     i += 1

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
    else:
        print("unrecognized value received ('" + line + "')")

    led.value = not led.value