from machine import Pin, UART
import time


# Pin Configuration
uart = UART(0, baudrate=19200, tx=Pin(0), rx=Pin(1))
rst_pin = Pin(4, Pin.OUT)
wake_pin = Pin(5, Pin.OUT)

TRUE         =  1
FALSE        =  0

# Basic response message definition (adapted from Waveshare demo)
ACK_SUCCESS           = 0x00
ACK_FAIL              = 0x01
ACK_FULL              = 0x04
ACK_NO_USER           = 0x05
ACK_USR_OCCUPIED      = 0x06
ACK_FINGER_OCCUPIED   = 0x07
ACK_TIMEOUT           = 0x08
ACK_GO_OUT            = 0x0F     # The center of the fingerprint is out of alignment with sensor

# User information definition (adapted from Waveshare demo)
ACK_ALL_USER          = 0x00
ACK_GUEST_USER        = 0x01
ACK_NORMAL_USER       = 0x02
ACK_MASTER_USER       = 0x03

USER_MAX_CNT          = 1000        # Maximum fingerprint number

# Command definition (adapted from Waveshare demo)
CMD_HEAD              = 0xF5
CMD_TAIL              = 0xF5
CMD_ADD_1             = 0x01
CMD_ADD_2             = 0x02
CMD_ADD_3             = 0x03
CMD_MATCH             = 0x0C
CMD_DEL               = 0x04
CMD_DEL_ALL           = 0x05
CMD_USER_CNT          = 0x09
CMD_COM_LEV           = 0x28
CMD_LP_MODE           = 0x2C
CMD_TIMEOUT           = 0x2E

CMD_FINGER_DETECTED   = 0x14

# reset fingerprint sensor
def hard_reset():
    print("Resetting fingerprint sensor...")
    rst_pin.value(0)
    time.sleep(0.1)
    rst_pin.value(1)
    time.sleep(0.8)
    print("Reset complete.")

# wake fingerprint sensor
def wake_sensor():
    print("Waking sensor...")
    wake_pin.value(1)
    time.sleep(0.05)

# perform checksum on buffer, needed for passing commands to sensor and validating responses
def CheckSUM(command_buf):
    checksum = 0
    for i in range (1,len(command_buf)):
        checksum ^= command_buf[i]
    command_buf.append(checksum)
    command_buf.append(CMD_TAIL)
    return command_buf

# function to send a packet to a sensor via UART, and read response
def send_packet(packet):
    # first, flush UART read buffer
    while uart.any():
        uart.read()
    
    # the command is denoted in the second byte of the packet
    command_sent = packet[1]
    
    # send packet via UART
    uart.write(bytes(packet))
    print("Sent:", packet)
    time.sleep_ms(50)
    data = b""
    start = False
    buf = bytearray()
    t0 = time.ticks_ms()
    while time.ticks_diff(time.ticks_ms(), t0) < 10000:
        if uart.any():
            current_buffer = uart.read(1)
            # print("Test", b)
            if not current_buffer:
                continue
            
            # synchronized with the CMD_HEAD (0xF5) of recieved packet
            head = current_buffer[0]
            if not start:
                if head == CMD_HEAD:
                    start = True
                    buf = bytearray([head])
                continue
            else:
                buf.append(head)

            # if we got at least 8 bytes, check end and checksum
            if len(buf) >= 8 and buf[-1] == CMD_TAIL:
                # minimal 8 byte response — validate
                if buf[0] == CMD_HEAD:
                    # perform checksum and ensure that it matches the 7th field of buffer
                    if (CheckSUM(buf)[6]) == buf[6]:
                        return buf
                # if invalid, reset and look for next start
                start = False
                buf = bytearray()
                
            if len(buf) == 8:
                # Start / end check
                if buf[0] != CMD_HEAD or buf[7] != CMD_END:
                    buf = bytearray()
                    start_seen = False
                    continue

                # verify checksum 
                if (CheckSUM(buf))[6] != buf[6]:
                    buf = bytearray()
                    start_seen = False
                    continue

                if buf[1] != command_sent:
                    buf = bytearray()
                    start_seen = False
                    continue

                return buf
            
    return data

def read_response(timeout=1000):
    start = time.ticks_ms()
    data = b""
    while time.ticks_diff(time.ticks_ms(), start) < timeout:
        if uart.any():
            data += uart.read()
    return data

# function for adding fingerprint
def add_fingerprint(ID=0, permission=1):
    # change later, provide id by counting number of existing users but for now hardcode id for testing purposes
    id = 1
    # build command buffer using format specified by data sheet
    command_buf =[CMD_HEAD, CMD_ADD_1, 0, id+1, permission, 0]
    command = CheckSUM(command_buf)
    response = send_packet(command)
    print ("response:", response)
    if not response:
        print ("No response")
        return ACK_FAIL
    if response[4] == ACK_SUCCESS:
        command_buf =[CMD_HEAD, CMD_ADD_2, 0, id+1, permission, 0]
        command = CheckSUM(command_buf)
        response = send_packet(command)
        if response[4] == ACK_SUCCESS:
            command_buf =[CMD_HEAD, CMD_ADD_3, 0, id+1, permission, 0]
            command = CheckSUM(command_buf)
            response = send_packet(command)
            if response[4] == ACK_SUCCESS:
                print("User %d is added to database successfully" %(id+1))
                return ACK_SUCCESS
            elif response[4] == ACK_TIMEOUT:
                print("Failed： Timeout！")
                return ACK_TIMEOUT
            else:
                print("Failed !")
                return ACK_FAIL

        elif response[4] == ACK_TIMEOUT:
            print("Failed： Timeout！")
            return ACK_TIMEOUT
        else:
            print("Failed !")
        return ACK_FAIL
    elif response[4] == ACK_TIMEOUT:
        print("Failed： Timeout！")
        return ACK_TIMEOUT
    elif response[4] == ACK_FULL:
        print("The database is full!")
        return ACK_FULL
    elif response[4] == ACK_USR_OCCUPIED:
        print ("The User already exists, please change the id and test again!")
        return ACK_USR_OCCUPIED
    elif response[4] == ACK_FINGER_OCCUPIED:
        print ("The fingerprint already exists, please change a finger and test again!")
        return ACK_FINGER_OCCUPIED
    else:
            print("Failed !")
            return ACK_FAIL

# function to verify user
def verify_user():
    command_buf =[CMD_HEAD, CMD_MATCH, 0, 0, 0, 0]
    command = CheckSUM(command_buf)
    response = send_packet(command)
    time.sleep(2)

    if response[4] == 1 or response[4] == 2 or response[4] == 3:
        ID = response[2] + response[3]
        permission = response[4]
        print("The user %d is matched, permission is %d"%(ID, permission))
        return ACK_SUCCESS
    elif response[4] == ACK_TIMEOUT:
        print("Failed: Time out !")
        return ACK_TIMEOUT
    elif response[4] == ACK_NO_USER:
        print("Failed: There is no matched fingerprint.")
        return ACK_NO_USER
    else:
        print("Failed！")
        return ACK_FAIL

# test procedure
print("Starting fingerprint sensor test...")

# initialize pins
rst_pin.value(1)    # keep sensor out of reset
wake_pin.value(1)   # wake it up

hard_reset()
wake_sensor()

while True:
    print("\nSending add fingerprint command...")
    verify_user
    ()

    time.sleep(5)

