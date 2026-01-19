import time
import board
import busio
from adafruit_bus_device.i2c_device import I2CDevice

# Initialize I2C bus
i2c = busio.I2C(scl=board.GP1, sda=board.GP0)

print('Waiting for I2C bus...')
# Wait until I2C bus is ready
while not i2c.try_lock():
    pass
print('I2C bus ready!')

devices = i2c.scan()
print("Found I2C devices:", [hex(d) for d in devices])
i2c.unlock()

EEPROM_I2C_ADDRESS = 0x50  # A0, A1 = 0

# Create I2C device
eeprom = I2CDevice(i2c, EEPROM_I2C_ADDRESS)

def write_byte(mem_addr, value):
    """Write a single byte to a memory address."""
    block = (mem_addr >> 8) & 0x01
    i2c_addr = 0x50 | block
    addr_lo = mem_addr & 0xFF
    data = bytes([addr_lo, value])
    i2c.try_lock()
    i2c.writeto(i2c_addr, data)
    i2c.unlock()
    time.sleep(0.01)  # Wait for write cycle

def read_byte(mem_addr):
    """Read one byte from AT24CS04 (4Kbit EEPROM)"""
    block = (mem_addr >> 8) & 0x01
    i2c_addr = 0x50 | block
    addr_lo = mem_addr & 0xFF
    result = bytearray(1)
    i2c.try_lock()
    i2c.writeto_then_readfrom(i2c_addr, bytes([addr_lo]), result)
    i2c.unlock()
    return result[0]

def write_word(mem_addr, value):
    for i in range(4):
        write_byte(mem_addr + i, (value >> (8*(3-i))) & 0xFF)
        
def read_word(mem_addr):
    ret = 0
    for i in range(4):
        ret |= read_byte(mem_addr + i) << (3-i)*8
    return ret

# Example: Write and read a byte
print("Writing 0xDEADBEEF to address 0x10...")
write_word(0x10, 0xDEADBEEF)

print("Reading back value...")
val = read_word(0x10)
print("Read:", hex(val))

print('Reading all non-default memory values (default is 0xFF)')
i = 0
while i <= 0xFF:
    val = read_byte(i)
    if (val != 0xFF):
        print(hex(i), ":", hex(val))
    i += 1

