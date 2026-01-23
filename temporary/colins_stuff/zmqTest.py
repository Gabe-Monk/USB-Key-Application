import zmq

def main():
    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    socket.connect("tcp://localhost:5555")

    testStr = 'test string :)'

    print('REQ: sending \'' + testStr + '\'...')
    socket.send_string(testStr)
    print('REQ: sent')
    mystr = socket.recv_string()
    print('REQ: Received reply \'' + mystr + '\'')

    socket.close()
    context.destroy()

if __name__ == "__main__":
    main()
