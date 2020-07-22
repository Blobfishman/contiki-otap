import sys
import binascii
import re
import matplotlib
from datetime import datetime, time, date
import matplotlib.pyplot as plt
from scapy.all import *


# Class for accumulating traffic per second
class Item:
    payload_size = 0
    icmp_size = 0
    udp_size = 0
    ip_size = 0
    lowpan_size = 0
    ether_size = 0
    
    def __init__(self, payload, icmp, udp, ip, lowpan, ether):
        self.payload_size = payload
        self.icmp_size = icmp
        self.udp_size = udp
        self.ip_size = ip
        self.lowpan_size = lowpan
        self.ether_size = ether

    def overhead(self):
        return self.icmp_size + self.udp_size + self.ip_size + self.lowpan_size + self.ether_size

    def total(self):
        return self.overhead() + self.payload_size


def generateDiagrams(input_file, output_file, mode): 

    start_time = 0

    evaluation = {}

    pcap_packets = rdpcap(input_file)

    for packet in pcap_packets:
        if start_time == 0:
            start_time = packet.time
        time = int(packet.time - start_time)
        max_time = time

        payload_size = 0
        icmp_size = 0
        udp_size = 0
        ip_size = 0
        lowpan_size = 0
        ether_size = 0

        if len(packet) < 10:
            # ACK message
            ether_size = len(packet)
        elif packet.load.startswith(b"\x9b"):
            # ICMP Hello
            icmp_size = len(packet[IPv6].load)
            ip_size = len(packet[IPv6]) - icmp_size
            lowpan_size = len(packet[LoWPAN_IPHC]) - len(packet[LoWPAN_IPHC].load)
            ether_size = len(packet[Dot15d4]) - lowpan_size
        elif UDP in packet:
            # Actual data message
            payload_size = len(packet[UDP].load)
            udp_size = len(packet[UDP]) - payload_size
            ip_size = len(packet[IPv6]) - len(packet[IPv6].load)
            lowpan_size = len(packet[LoWPAN_IPHC]) - len(packet[LoWPAN_IPHC].load)
            ether_size = len(packet[Dot15d4]) - lowpan_size
            
	


        if time in evaluation:
            evaluation[time].payload_size += payload_size
            evaluation[time].icmp_size += icmp_size
            evaluation[time].udp_size += udp_size
            evaluation[time].ip_size += ip_size
            evaluation[time].lowpan_size += lowpan_size
            evaluation[time].ether_size += ether_size
        else:
            evaluation[time] = Item(payload_size,
                                    icmp_size,
                                    udp_size,
                                    ip_size,
                                    lowpan_size,
                                    ether_size)

    time_list = []
    payload_list = []
    icmp_list = []
    udp_list = []
    ip_list = []
    lowpan_list = []
    ether_list = []
    overhead_list = []
    percentage_list = []

    for i in range(0, max_time):
        time_list.append(i)
        if i in evaluation:
            payload_list.append(evaluation[i].payload_size)
            icmp_list.append(evaluation[i].icmp_size)
            udp_list.append(evaluation[i].udp_size)
            ip_list.append(evaluation[i].ip_size)
            lowpan_list.append(evaluation[i].lowpan_size)
            ether_list.append(evaluation[i].ether_size)
            overhead_list.append(evaluation[i].overhead())
            percentage_list.append(float(evaluation[i].payload_size) / float(evaluation[i].total()))
        else:
            payload_list.append(0)
            icmp_list.append(0)
            udp_list.append(0)
            ip_list.append(0)
            lowpan_list.append(0)
            ether_list.append(0)
            overhead_list.append(0)
            percentage_list.append(0)

    fig, ax1 = plt.subplots()
    ax1.set_xlabel('time (s)')

    if mode == "sum_overhead":
        labels = ["Header", "Data"]
        ax1.stackplot(time_list, overhead_list, payload_list, labels=labels)
    elif mode == "percentage":
        ax1.stackplot(time_list, percentage_list)
    else:    
        labels = ["802.15.4", "6LoWPAN", "IPv6", "UDP", "ICMP", "Data"]
        ax1.stackplot(time_list, ether_list, lowpan_list, ip_list, udp_list, icmp_list, payload_list, labels=labels)

    ax1.tick_params(axis='y')
    if mode == "percentage":
        ax1.set_ylabel('use data proportion')    
    else:
        ax1.set_ylabel('traffic (byte/s)')
    ax1.legend(loc='best')
    fig.tight_layout()
    plt.show()

    plt.savefig(output_file)
    

if len(sys.argv) < 3:
    print('usage: ' + sys.argv[0] + ' pcap_file output_file [--sum-overhead//--use-data-percentage]')
    exit(0)

mode = 'detailed'
if len(sys.argv) > 3:
    if "-sum-overhead" in sys.argv[3] or "-overhead_sum" in sys.argv[3]:
        mode = "sum_overhead"
    elif "-percentage" in sys.argv[3]:
        mode = "percentage"


generateDiagrams(sys.argv[1], sys.argv[2], mode)
