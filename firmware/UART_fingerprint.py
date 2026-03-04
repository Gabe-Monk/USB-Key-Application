import board
import busio
import digitalio
import time

class FingerprintSensor:

    TRUE  = 1
    FALSE = 0

    # Response Codes
    ACK_SUCCESS         = 0x00
    ACK_FAIL            = 0x01
    ACK_FULL            = 0x04
    ACK_NO_USER         = 0x05
    ACK_USR_OCCUPIED    = 0x06
    ACK_FINGER_OCCUPIED = 0x07
    ACK_TIMEOUT         = 0x08
    ACK_GO_OUT          = 0x0F

    # Command Codes
    CMD_HEAD  = 0xF5
    CMD_TAIL  = 0xF5
    CMD_ADD_1 = 0x01
    CMD_ADD_2 = 0x02
    CMD_ADD_3 = 0x03
    CMD_MATCH = 0x0C
    CMD_DEL   = 0x04
    CMD_DEL_ALL = 0x05
    CMD_USER_CNT = 0x09
    
    def __init__ (self, uart_tx, uart_rx, rst_pin, wake_pin):
        # Pin Configuration
        self.uart = busio.UART(uart_tx, uart_rx, baudrate=19200)

        self.rst_pin = digitalio.DigitalInOut(rst_pin)
        self.rst_pin.direction = digitalio.Direction.OUTPUT

        self.wake_pin = digitalio.DigitalInOut(wake_pin)
        self.wake_pin.direction = digitalio.Direction.OUTPUT
        
        self.rst_pin.value = True
        self.hard_reset()
        

    def hard_reset(self):
        # print("Resetting fingerprint sensor...")
        self.rst_pin.value = False
        time.sleep(0.1)
        self.rst_pin.value = True
        time.sleep(0.8)
        # print("Reset complete.")

    # Perform a CheckSUM as verified per datasheet on packet
    def CheckSUM(self, command_buf):
        checksum = 0
        for i in range(1, len(command_buf)):
            checksum ^= command_buf[i]
        command_buf.append(checksum)
        command_buf.append(self.CMD_TAIL)
        return command_buf

    # Send a packet to sensor via UART bus
    def send_packet(self, packet):

        # Flush UART buffer
        self.uart.reset_input_buffer()

        command_sent = packet[1]
        self.uart.write(bytes(packet))
        # print("Sent:", packet)

        time.sleep(0.05)
        buf = bytearray()
        start = False
        start_time = time.monotonic()

        while time.monotonic() - start_time < 5:

            if self.uart.in_waiting:
                byte = self.uart.read(1)
                # print ("Byte:", byte)
                if not byte:
                    continue

                value = byte[0]

                if not start:
                    if value == self.CMD_HEAD:
                        start = True
                        buf = bytearray([value])
                    continue
                else:
                    buf.append(value)

                if len(buf) == 8:

                    if buf[0] != self.CMD_HEAD or buf[7] != self.CMD_TAIL:
                        start = False
                        buf = bytearray()
                        continue

                    # checksum validation
                    temp = buf[:6]
                    self.CheckSUM(temp)
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
    
    # Function to get count of total number of users currently registered
    def get_user_count(self):
        command_buf = [self.CMD_HEAD, self.CMD_USER_CNT, 0, 0, 0, 0]
        command = self.CheckSUM(command_buf)
        response = self.send_packet(command)
        if response[4] == self.ACK_SUCCESS:
            finger_account = response[2] + response[3]
            return finger_account
        else:
            print("error_fingerprint_sensor: Failed to query the account!")
            return self.ACK_FAIL
        
    def delete_all_users(self):
        command_buf = [self.CMD_HEAD, self.CMD_DEL_ALL, 0, 0, 0, 0]
        command = self.CheckSUM(command_buf)
        response = self.send_packet(command)
        if response[4] == self.ACK_TIMEOUT:
            return self.ACK_TIMEOUT
        if response[4] == self.ACK_SUCCESS:
            return self.ACK_SUCCESS
        else:
            return self.ACK_FAIL

    # Function for adding fingerprint
    def add_fingerprint(self, ID=0, permission=1):
        # TODO: change later, provide id by counting number of existing users but for now hardcode id for testing purposes
        self.delete_all_users()
        id = 0
        # build command buffer using format specified by data sheet
        command_buf =[self.CMD_HEAD, self.CMD_ADD_1, 0, id+1, permission, 0]
        command = self.CheckSUM(command_buf)
        response = self.send_packet(command)
        # print ("response:", response)
        if not response:
            # print ("error_fingerprint_sensor: No response")
            return self.ACK_FAIL
        if response[4] == self.ACK_SUCCESS:
            command_buf =[self.CMD_HEAD, self.CMD_ADD_2, 0, id+1, permission, 0]
            command = self.CheckSUM(command_buf)
            response = self.send_packet(command)
            if response[4] == self.ACK_SUCCESS:
                command_buf = [self.CMD_HEAD, self.CMD_ADD_3, 0, id+1, permission, 0]
                command = self.CheckSUM(command_buf)
                response = self.send_packet(command)
                if response[4] == self.ACK_SUCCESS:
                    # print("User %d is added to database successfully" %(id+1))
                    return self.ACK_SUCCESS
                elif response[4] == self.ACK_TIMEOUT:
                    return self.ACK_TIMEOUT
                else:
                    return self.ACK_FAIL

            elif response[4] == self.ACK_TIMEOUT:
                return self.ACK_TIMEOUT
            
            return self.ACK_FAIL
        elif response[4] == self.ACK_TIMEOUT:
            return self.ACK_TIMEOUT
        elif response[4] == self.ACK_FULL:
            return self.ACK_FULL
        elif response[4] == self.ACK_USR_OCCUPIED:
            return self.ACK_USR_OCCUPIED
        elif response[4] == self.ACK_FINGER_OCCUPIED:
            return self.ACK_FINGER_OCCUPIED
        else:
            return self.ACK_FAIL

    # Function to verify user
    def verify_user(self):
        command_buf = [self.CMD_HEAD, self.CMD_MATCH, 0, 0, 0, 0]
        command = self.CheckSUM(command_buf)
        response = self.send_packet(command)
        time.sleep(0.5)

        if not response:
            return self.ACK_TIMEOUT

        if response[4] == 1 or response[4] == 2 or response[4] == 3:
            ID = response[2] + response[3]
            permission = response[4]
            # print("The user %d is matched, permission is %d"%(ID, permission))
            return self.ACK_SUCCESS
        elif response[4] == self.ACK_TIMEOUT:
            return self.ACK_TIMEOUT
        elif response[4] == self.ACK_NO_USER:
            return self.ACK_NO_USER
        else:
            return self.ACK_FAIL
        
    # Translates request number to request execution and response
    def decode_request(self, request):
        if request == self.CMD_ADD_1:
            # print ("Adding new user... place finger on fingperprint sensor")
            # print ("Add fingerprint  (Put your finger on sensor until successfully/failed information returned) ")
            rc = self.add_fingerprint()
            if rc == self.ACK_SUCCESS:
                print ("added")
                return 0
            elif rc == self.ACK_FAIL:
                print ("error_fingerprint_sensor: Failed: Please try to place the center of the fingerprint flat to sensor, or this fingerprint already exists !")
                return 1
            elif rc == self.ACK_FULL:
                print ("error_fingerprint_sensor: Failed: The fingerprint library is full !")
                return 1 
            elif rc == self.ACK_TIMEOUT:
                print("error_fingerprint_sensor: Failed： Timeout！")
                return 1
            elif rc == self.ACK_FINGER_OCCUPIED:
                print ("error_fingerprint_sensor: The fingerprint already exists, please change a finger and test again!")
                return 1
            elif rc == self.ACK_USR_OCCUPIED:
                print ("error_fingerprint_sensor: The User already exists, please change the id and test again!")
                return 1
        elif request == self.CMD_MATCH:
            # print ("Verifying fingerprint... place finger on fingerprint sensor")
            rc = self.verify_user()
            if rc == self.ACK_SUCCESS:
                print ("matched")
                return 0
            elif rc == self.ACK_NO_USER:
                print ("error_fingerprint_sensor: Failed: This fingerprint was not found in the library !")
                return 1
            elif rc == self.ACK_TIMEOUT:
                print ("error_fingerprint_sensor: Failed: Time out !")
                return 1
            elif rc == self.ACK_GO_OUT:
                print ("error_fingerprint_sensor: Failed: Please try to place the center of the fingerprint flat to sensor !")
                return 1
            elif rc == self.ACK_FAIL:
                print("error_fingerprint_sensor: Failed！")
                return 1
        elif request == self.CMD_USER_CNT:
            count = self.get_user_count()
            print ("Number of users recorded is: %d" % count)
        elif request == self.CMD_DEL_ALL:
            rc = self.delete_all_users()
            if rc == self.ACK_SUCCESS:
                print ("All users deleted successfully!")
                return 0
            elif rc == self.ACK_TIMEOUT:
                print ("error_fingerprint_sensor: Failed: Time out!")
                return 1
            elif rc == self.ACK_FAIL:
                print ("error_fingerprint_sensor: Failed to delete all users!")
                return 1


