#!/usr/bin/python

import os
import sys
import socket
import time
import struct

QUIET = '-q' in sys.argv
HOST = str(sys.argv[1])
PORT = int(sys.argv[2])

def main():
    sock = socket.create_connection((HOST, PORT))
    while True:
        r = sock.recv(1)
        if not QUIET: print r
        if r == 'X':
            if not QUIET: print 'sending X'
            sock.send('X')
        else:
            sock.send(r)
            pass

if __name__ == '__main__':
    main()
