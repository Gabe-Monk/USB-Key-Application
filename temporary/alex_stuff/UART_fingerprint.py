import board
import busio
import digitalio
import time

# -----------------------------
# Pin Configuration
# -----------------------------
uart = busio.UART(board.GP0, board.GP1, baudrate=19200)

rst_pin = digitalio.DigitalInOut(board.GP22)
rst_pin.direction = digitalio.Direction.OUTPUT

wake_pin = digitalio.DigitalInOut(board.GP21)
wake_pin.direction = digitalio.Direction.OUTPUT


TRUE  = 1
FALSE = 0

# -----------------------------
# Response Definitions
# -----------------------------
ACK_SUCCESS         = 0x00
ACK_FAIL            = 0x01
ACK_FULL            = 0x04
ACK_NO_USER         = 0x05
ACK_USR_OCCUPIED    = 0x06
ACK_FINGER_OCCUPIED = 0x07
ACK_TIMEOUT         = 0x08
ACK_GO_OUT          = 0x0F

# -----------------------------
# Command Definitions
# -----------------------------
CMD_HEAD  = 0xF5
CMD_TAIL  = 0xF5
CMD_ADD_1 = 0x01
CMD_ADD_2 = 0x02
CMD_ADD_3 = 0x03
CMD_MATCH = 0x0C
CMD_DEL   = 0x04
CMD_DEL_ALL = 0x05
CMD_USER_CNT = 0x09

def hard_reset():
    print("Resetting fingerprint sensor...")
    rst_pin.value = False
    time.sleep(0.1)
    rst_pin.value = True
    time.sleep(0.8)
    print("Reset complete.")


def wake_sensor():
    print("Waking sensor...")
    wake_pin.value = True
    time.sleep(0.05)

# Perform a CheckSUM as verified per datasheet on packet
def CheckSUM(command_buf):
    checksum = 0
    for i in range(1, len(command_buf)):
        checksum ^= command_buf[i]
    command_buf.append(checksum)
    command_buf.append(CMD_TAIL)
    return command_buf

# Send a packet to sensor via UART bus
def send_packet(packet):

    # Flush UART buffer
    uart.reset_input_buffer()

    command_sent = packet[1]

    uart.write(bytes(packet))
    print("Sent:", packet)

    time.sleep(0.05)

    buf = bytearray()
    start = False
    start_time = time.monotonic()

    while time.monotonic() - start_time < 10:

        if uart.in_waiting:
            byte = uart.read(1)
            print ("Byte:", byte)
            if not byte:
                continue

            value = byte[0]

            if not start:
                if value == CMD_HEAD:
                    start = True
                    buf = bytearray([value])
                continue
            else:
                buf.append(value)

            if len(buf) == 8:

                if buf[0] != CMD_HEAD or buf[7] != CMD_TAIL:
                    start = False
                    buf = bytearray()
                    continue

                # checksum validation
                temp = buf[:6]
                CheckSUM(temp)
                if temp[6] != buf[6]:
                    start = False
                    buf = bytearray()
                    continue

                if buf[1] != command_sent:
                    start = False
                    buf = bytearray()
                    continue

                return buf

    return None

# Function for adding fingerprint
def add_fingerprint(ID=0, permission=1):
    # change later, provide id by counting number of existing users but for now hardcode id for testing purposes
    id = 0
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

# Function to verify user
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
    
# Function to get count of total number of users currently registered
def get_user_count():
    command_buf = [CMD_HEAD, CMD_USER_CNT, 0, 0, 0, 0]
    command = CheckSUM(command_buf)
    #print(command)
    response = send_packet(command)
    if response[4] == ACK_SUCCESS:
        finger_account = response[2] + response[3]
        return finger_account
    else:
        print("Failed to query the account!")
        return ACK_FAIL
    
def delete_all_users():
    command_buf = [CMD_HEAD, CMD_DEL_ALL, 0, 0, 0, 0]
    command = CheckSUM(command_buf)
    response = send_packet(command)
    if response[4] == ACK_TIMEOUT:
        return ACK_TIMEOUT
    if response[4] == ACK_SUCCESS:
        return ACK_SUCCESS
    else:
        print ("Failed to delete all users!")
        return ACK_FAIL
    
# Translates request number to request execution and response
def decode_request(request):
    if request == CMD_ADD_1:
        print ("Adding new user... place finger on fingperprint sensor")
        print ("Add fingerprint  (Put your finger on sensor until successfully/failed information returned) ")
        rc = add_fingerprint()
        if rc == ACK_SUCCESS:
            print ("Fingerprint added successfully !")
        elif rc == ACK_FAIL:
            print ("Failed: Please try to place the center of the fingerprint flat to sensor, or this fingerprint already exists !")
        elif rc == ACK_FULL:
            print ("Failed: The fingerprint library is full !") 
    elif request == CMD_MATCH:
        print ("Verifying fingerprint... place finger on fingerprint sensor")
        rc = verify_user()
        if rc == ACK_SUCCESS:
            print ("Matching successful !")
            return 0
        elif rc == ACK_NO_USER:
            print ("Failed: This fingerprint was not found in the library !")
            return 1
        elif rc == ACK_TIMEOUT:
            print ("Failed: Time out !")
            return 1
        elif rc == ACK_GO_OUT:
            print ("Failed: Please try to place the center of the fingerprint flat to sensor !")
            return 1
    elif request == CMD_USER_CNT:
        count = get_user_count()
        print ("Number of users recorded is: %d" % count)
    elif request == CMD_DEL_ALL:
        rc = delete_all_users()
        if rc == ACK_SUCCESS:
            print ("All users deleted successfully!")
        elif rc == ACK_TIMEOUT:
            print ("Failed: Time out!")
        elif rc == ACK_FAIL:
            print ("Failed to delete all useres!")


# Main test procedure
print("Starting fingerprint sensor test...")

rst_pin.value = True
#wake_pin.value = True

hard_reset()
#wake_sensor()

while True:
    decode_request(CMD_MATCH)
    time.sleep(5)


