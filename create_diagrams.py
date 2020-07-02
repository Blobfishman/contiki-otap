import sys
import binascii
import re
import matplotlib
from datetime import datetime, time, date
import matplotlib.pyplot as plt

TIME_FORMAT = "%M:%S.%f"
max_time = 0

def generateDiagrams(input_file, output_file, only_packets, only_traffic): 

    transmission_file = open(input_file, "r")

    packets = {}
    load = {}

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

        # for some reason, without the addition the 0th second is -3543
        time = int(datetime.combine(date.fromtimestamp(0), datetime.strptime(time, TIME_FORMAT).time()).timestamp()) + 3543
        max_time = time

        if time in packets:
            packets[time] += 1
        else:
            packets[time] = 1

        if time in load:
            load[time] += len(message)
        else:
            load[time] = len(message)

    transmission_file.close()
    
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
    print('usage: ' + sys.argv[0] + ' cooja_file output_file [--only-packets//--only-traffic]')
    exit(0)

only_packets = False
only_traffic = False
if len(sys.argv) > 3:
    if "-only-packets" in sys.argv[3]:
        only_packets = True
    if "-only-traffic" in sys.argv[3]:
        only_traffic = True

generateDiagrams(sys.argv[1], sys.argv[2], only_packets, only_traffic)
