import time
import board
import digitalio

# Red LED (external)
redLed = digitalio.DigitalInOut(board.GP16)
redLed.direction = digitalio.Direction.OUTPUT

def indicateErrorOnLed():
    # Flash the LED 5 times
    for _ in range(5):
        redLed.value = True
        time.sleep(0.1)
        redLed.value = False
        time.sleep(0.07)

def reportError(msg):
    print(msg)
    indicateErrorOnLed()