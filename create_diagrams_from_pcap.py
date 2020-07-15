import sys
import binascii
import re
import matplotlib
from datetime import datetime, time, date
import matplotlib.pyplot as plt
from scapy.all import *

def generateDiagrams(input_file, output_file, only_packets, only_traffic): 

    start_time = 0

    packets = {}
    load = {}

    pcap_packets = rdpcap(input_file)

    for packet in pcap_packets:
        if start_time == 0:
            start_time = packet.time
        time = int(packet.time - start_time)
        max_time = time

        total_size = len(packet)
#       use_data_size = 


        if time in packets:
            packets[time] += 1
        else:
            packets[time] = 1

        if time in load:
            load[time] += total_size
        else:
            load[time] = total_size

    time_list = []
    packet_list = []
    load_list = []

    for i in range(0, max_time):
        time_list.append(i)
        if i in packets:
            packet_list.append(packets[i])
        else:
            packet_list.append(0)
        if i in load:
            load_list.append(load[i])
        else:
            load_list.append(0)

    fig, ax1 = plt.subplots()
    ax1.set_xlabel('time (s)')

    if only_traffic:
        ax1.set_ylabel('traffic (bytes/s)')
        ax1.plot(time_list, load_list)
        ax1.tick_params(axis='y')

    else:
        ax1.set_ylabel('# packets')
        ax1.plot(time_list, packet_list)
        ax1.tick_params(axis='y')

        if not only_packets:
            ax2 = ax1.twinx()
            color = 'tab:red'
            ax2.set_ylabel('traffic (bytes/s)', color=color)
            ax2.plot(time_list, load_list, color=color)
            ax2.tick_params(axis='y', labelcolor=color)

    fig.tight_layout()
    plt.savefig(output_file)
    plt.show()
    

if len(sys.argv) < 3:
    print('usage: ' + sys.argv[0] + ' pcap_file output_file [--only-packets//--only-traffic]')
    exit(0)

only_packets = False
only_traffic = False
if len(sys.argv) > 3:
    if "-only-packets" in sys.argv[3] or "-packets-only" in sys.argv[3]:
        only_packets = True
    if "-only-traffic" in sys.argv[3] or "-traffic-only" in sys.argv[3]:
        only_traffic = True

generateDiagrams(sys.argv[1], sys.argv[2], only_packets, only_traffic)
