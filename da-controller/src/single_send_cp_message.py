import sys

from scapy.all import *

CTR_PORT = 12490
CTR_IP = "10.0.2.1"

#ACTIONS = {
#    0: '\x00', # FLOW ADD
#    1: '\x01', # FLOW REMOVE
#    2: '\x02'  # CACHE CLEAR
#}
#
#TORS = {
#    0: '\x00', # TOR 0
#    1: '\x01'  # TOR 1
#}
#
#CACHE_IDs = {
#    0: '\x00',
#    1: '\x01'
#}

action = int(sys.argv[1])
tor = int(sys.argv[2])
cache_id = int(sys.argv[3])
dst_tor_id = int(sys.argv[4])
port = int(sys.argv[5])
interface = sys.argv[6]

bytes_to_send = b''
bytes_to_send += (action.to_bytes(1, byteorder='little'))
bytes_to_send += (tor.to_bytes(1, byteorder='little'))
bytes_to_send += (cache_id.to_bytes(1, byteorder='little'))
bytes_to_send += (dst_tor_id.to_bytes(1, byteorder='little'))
bytes_to_send += (port.to_bytes(2, byteorder='big'))

print(bytes_to_send)

if __name__ == '__main__':
    sendp(
	Ether(dst='ff:ff:ff:ff:ff:ff')/IP(dst=CTR_IP)/UDP(dport=CTR_PORT)/Raw(load=bytes_to_send), 
            iface=interface
     )
