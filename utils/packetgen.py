#!/usr/bin/python

import os
import sys
import socket
import time
import struct

HOST = str(sys.argv[1])
PORT = int(sys.argv[2])

def crc8(data):
        data = map(ord, data)
	crc = 0;
        for d in data:
		crc ^= (d << 8)
                crc &= 0xffff;
                for i in range(8,0,-1): #= 8; i; i--) {
                        if (crc & 0x8000):
				crc ^= (0x1070 << 3)
                                crc &= 0xffff;
			crc <<= 1
                        crc &= 0xffff;
	return (crc >> 8) & 0xff;

def create_packet(data, hops=1):
    ln = len(data)
    crc = crc8(data)
    crcxor = crc ^ 0xff;
    s = struct.pack("<BB"+str(ln)+"s"+"BB", hops, ln, data, crc, crcxor)
    print "packet to send:", " ".join(map(hex, map(ord, s)))
    print "len: ", ln 
    return s


def unpack_packet(sock):
    data = ''
    while len(data) != 2:
        data += sock.recv(1)
    print type(data), "'{}'".format(data)
    hops, ln = struct.unpack("<BB", data[:2])
#    data = sock.recv(ln+2)
    while len(data) != ln + 4:
        data += sock.recv(ln+4-len(data))
    d, crc, crcxor = struct.unpack("<"+str(ln)+"sBB", data[2:])
    print hops, d
    return d

def main():
    sock = socket.create_connection((HOST, PORT))
    sock.settimeout(1)
    sock.send('X')
    r = sock.recv(1)
    print r
    if r == '':
        exit(0)
    while True:
        packet = create_packet("zdar vole", 1) #"yolo "+time.asctime(), 3)
        print 'sending'
        sock.send(packet)
        print 'recving ack'
        try:
            r = sock.recv(1)
            print len(r), "ack: '{}'".format(r)
            if r == '':
                exit(0)
        except socket.timeout as e:
            print e
        print 'recving packet'
        #while 1:
        #    c = sock.recv(1)
        #    print c
        #    if c == '':
        #        exit()
        try:
            print unpack_packet(sock)
            print 'sending ack'
            sock.send('A')
            print 'sent'
        except socket.timeout as e:
            print e
        time.sleep(2.5)

if __name__ == '__main__':
    main()
