import busio
import time
from adafruit_bus_device.i2c_device import I2CDevice

class EepromDevice:
    """ For AT24CS04-SSHM-T EEPROM chip """

    i2c_addr = 0x50 # Chip pins A0, A1 tied to ground result in this I2C address

    def __init__(self, scl_pin, sda_pin):
        self.i2c_bus = busio.I2C(scl=scl_pin, sda=sda_pin)

        attempts = 0
        max_attempts = 100

        # Wait until I2C bus is ready
        while (not self.i2c_bus.try_lock()) and attempts < max_attempts:
            time.sleep(0.01)
            attempts += 1
        
        if attempts >= max_attempts:
            print('error_init: Failed to init EEPROM device - I2C bus not available')
            return

        self.i2c_bus.unlock()

        # Create I2C device
        self.device = I2CDevice(self.i2c_bus, EepromDevice.i2c_addr)
    
    def write_byte(self, mem_addr, value):
        """Write a single byte to a memory address."""
        block = (mem_addr >> 8) & 0x01
        i2c_addr = EepromDevice.i2c_addr | block
        addr_lo = mem_addr & 0xFF
        data = bytes([addr_lo, value])
        self.i2c_bus.try_lock()
        self.i2c_bus.writeto(i2c_addr, data)
        self.i2c_bus.unlock()
        time.sleep(0.01)  # Wait for write cycle

    def read_byte(self, mem_addr):
        """Read one byte from AT24CS04 (4Kbit EEPROM)"""
        block = (mem_addr >> 8) & 0x01
        i2c_addr = EepromDevice.i2c_addr | block
        addr_lo = mem_addr & 0xFF
        result = bytearray(1)
        self.i2c_bus.try_lock()
        self.i2c_bus.writeto_then_readfrom(i2c_addr, bytes([addr_lo]), result)
        self.i2c_bus.unlock()
        return result[0]

    def write_word(self, mem_addr, value):
        for i in range(4):
            self.write_byte(mem_addr + i, (value >> (8*(3-i))) & 0xFF)
            
    def read_word(self, mem_addr):
        ret = 0
        for i in range(4):
            ret |= self.read_byte(mem_addr + i) << (3-i)*8
        return ret
        
