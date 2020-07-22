import sys
import binascii
import re
from datetime import datetime, time
from scapy.all import *

TIME_FORMAT = "%M:%S.%f"

def generatePCAP(input_file, output_file): 

    transmission_file = open(input_file, "r")
    pcap_file = PcapWriter(output_file)

    while True:
        line = transmission_file.readline()
        if not line:
            break
        if not "[Packet Log]" in line:
            continue
        message = re.search("Message: '([^']+)'", line).group(1)
        receiver_address = re.search("received on ([^ ]+) ", line).group(1)
        receiver_port = int(re.search("Port ([0-9]+) from", line).group(1))
        sender_address = re.search("from ([^ ]+) ", line).group(1)
        sender_port = int(re.search("Port ([0-9]+)\n", line).group(1))
        time = re.search("([0-9]+:[0-9]{2}\.[0-9]{3})", line).group(1)
        time = datetime.combine(datetime.now().date(), datetime.strptime(time, TIME_FORMAT).time())

        pkt = IPv6() / UDP() / Raw()
        pkt.src = sender_address
        pkt.sport = sender_port
        pkt.dst = receiver_address
        pkt.dport = receiver_port
        pkt.time = time.timestamp()
        pkt.load = message

        pcap_file.write(pkt)

    transmission_file.close()
    pcap_file.close()

	

if len(sys.argv) < 3:
#        print 'usage: ' + sys.argv[0] + ' cooja_file output_file'
    print('usage: ' + sys.argv[0] + ' cooja_file output_file')
    exit(0)

generatePCAP(sys.argv[1], sys.argv[2])
