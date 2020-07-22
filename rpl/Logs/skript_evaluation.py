#!/usr/bin/env python

import matplotlib
import matplotlib.pyplot as plt 
import matplotlib.gridspec as gridspec
import re
import sys
import numpy as np

server = 0
node_number = 0

dictionary_normal_all = {}
dictionary_myrpl_all = {}

dictionary_normal_energy = {}
dictionary_myrpl_energy = {}

def plot():

    wight = 0.35
    ind = np.arange(2,node_number+1)

    radio_transmit_normal = []
    radio_transmit_myrpl = []

    for k,v in dictionary_normal_energy.items():
        tmp = v[-1] 
        radio_transmit_normal.append(tmp[4])

    for k,v in dictionary_myrpl_energy.items():
        tmp = v[-1] 
        radio_transmit_myrpl.append(tmp[4])

    fig = plt.figure(constrained_layout=True)
    # table = plt.table(cellText= array_paketlost, rowLabels= rows, colLabels= columns)
    gs = gridspec.GridSpec(1,1, figure= fig)
    
    ax1_1 = fig.add_subplot(gs[0,0])
    rect1 = ax1_1.bar(ind- wight/2,radio_transmit_normal, wight, color='blue',label='RPL Lite')
    rect2 = ax1_1.bar(ind+wight/2,radio_transmit_myrpl, wight, color='red', label='RPL Erweiterung')
    ax1_1.set_title('Transmitrate')
    ax1_1.set_ylabel('Tramitzeit in s')
    ax1_1.set_xlabel('Sensor Nummern')
    ax1_1.set_xticks(range(2,node_number+1))
    ax1_1.legend()

    autolabel(ax1_1, rect1)
    autolabel(ax1_1, rect2)

    fig.tight_layout()

    plt.show()

def autolabel(ax, rects):
    """Attach a text label above each bar in *rects*, displaying its height."""
    for rect in rects:
        height = rect.get_height()
        ax.annotate('{}'.format(height),
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')

def parser(logfile_normal, logfile_myrpl):
    global dictionary_normal_all
    global dictionary_normal_energy
    
    global dictionary_myrpl_all
    global dictionary_myrpl_energy


    dictionary_normal_energy = {new_list: [] for new_list in range(2,node_number+1)}
    dictionary_normal_all = {new_list: [] for new_list in range(2,node_number+1)}
    
    dictionary_myrpl_energy = {new_list: [] for new_list in range(2,node_number+1)}
    dictionary_myrpl_all = {new_list: [] for new_list in range(2,node_number+1)}

    app_regex = r"\[INFO:\s+App\s+\]" 
    cpu_sending_regex = r'Sending CPU'
    
    logfile_normal = open(logfile_normal, 'r')
    
    # Traffic Infos der APPs aus Logfile filtern, andere Logs z.B. MAIN SKY ... 
    # werden ignoriert
    traffic_normal_all = logfile_normal.readlines()
    traffic_normal_data = [line for line in traffic_normal_all if re.search(app_regex, line) is not None]
    
    # nach Node id filtern 
    for idx in range(2,node_number+1):
        id_regex = 'ID:' + str(idx)
        traffic = [line for line in traffic_normal_data if re.search(id_regex, line) is not None]
        dictionary_normal_all[idx].extend(traffic)

    # Time deltas der einzelnen Pakete errechnen
    for k,v in dictionary_normal_all.items():
        for i in v:
            if re.search(cpu_sending_regex, i) is not None:
                w = re.split(r"]", i)
                y = re.split(r"to", w[1])
                z = re.findall(r"\d+", y[0])
                z = [int(j) for j in z]
                p = tuple(z)
                dictionary_normal_energy[k].append(p)
    logfile_normal.close()
    
    logfile_myrpl = open(logfile_myrpl, 'r')

    # Traffic Infos der APPs aus Logfile filtern, andere Logs z.B. MAIN SKY ... 
    # werden ignoriert
    traffic_myrpl_all = logfile_myrpl.readlines()
    traffic_myrpl_data = [line for line in traffic_myrpl_all if re.search(app_regex, line) is not None]

    # nach Node id filtern 
    for idx in range(2,node_number+1):
        id_regex = 'ID:' + str(idx)
        traffic = [line for line in traffic_myrpl_data if re.search(id_regex, line) is not None]
        dictionary_myrpl_all[idx].extend(traffic)

    # Time deltas der einzelnen Pakete errechnen
    for k,v in dictionary_myrpl_all.items():
        for i in v:
            if re.search(cpu_sending_regex, i) is not None:
                w = re.split(r"]", i)
                y = re.split(r"to", w[1])
                z = re.findall(r"\d+", y[0])
                z = [int(j) for j in z]
                p = tuple(z)
                dictionary_myrpl_energy[k].append(p)
    logfile_myrpl.close()


def main(args):
    global server
    global node_number
    if len(sys.argv) != 5:
        print("Error: use ./skript.py <Logfile_normal>  <Logfile_myrpl> <Server_ID> <Number_of_Nodes>")
    logfile_normal = str(sys.argv[1])
    logfile_myrpl = str(sys.argv[2])
    server = int(sys.argv[3])
    node_number = int(sys.argv[4])
    parser(logfile_normal, logfile_myrpl)
    plot()


if __name__ == '__main__':
    main(sys.argv) 
