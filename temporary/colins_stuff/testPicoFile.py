import time
import board
import digitalio
import sys
# import select

led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT

while True:
    line = sys.stdin.readline().rstrip('\r\n')

    if line == 'pc_hello':
        print('usb_key_hello')
    elif line == 'pc_req_sn':
        print('PLACEHOLDER SERIAL NUMBER')
    else:
        print('unrecognized value received')



# while True:
#     rlist, _, _ = select.select([sys.stdin], [], [], 0)  # 0 = non-blocking
#     if rlist:
#         line = sys.stdin.readline().rstrip('\r\n')
#         print('Pico received: ' + line)
    
#     led.value = not led.value
#     print('Heartbeat - ' + str(led.value))
#     time.sleep(0.5)
