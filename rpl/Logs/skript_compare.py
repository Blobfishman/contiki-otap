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

dictionary_normal_times = {}
dictionary_myrpl_times = {}
dictionary_normal_energy = {}
dictionary_myrpl_energy = {}

array_normal_paketlost = []
array_myrpl_paketlost = []

array_normal_paketsend = []
array_myrpl_paketsend = []

def plot():

    wight = 0.35
    ind = np.arange(node_number-1)

    columns = ['Node %d' % x for x in range(node_number)]
    rows =  ('Paket lost Normal', 'Paket lost MyRPL', 'Paket send Normal', 'Paket send MyRPL')
    
    cell_text = []
    cell_text.append(['%d' % x for x in array_normal_paketlost])
    cell_text.append(['%d' % x for x in array_myrpl_paketlost])
    cell_text.append(['%d' % x for x in array_normal_paketsend])
    cell_text.append(['%d' % x for x in array_myrpl_paketsend])
    print(cell_text)
    print(len(rows))

    cpu_on_normal = []
    cpu_lpm_normal = []
    cpu_deep_lpm_normal = []
    radio_listen_normal = []
    radio_transmit_normal = []
    radio_off_normal = []
    
    cpu_on_myrpl = []
    cpu_lpm_myrpl = []
    cpu_deep_lpm_myrpl = []
    radio_listen_myrpl = []
    radio_transmit_myrpl = []
    radio_off_myrpl = []


    for k,v in dictionary_normal_energy.items():
        if int(k) == (server - 1):
            continue
        
        tmp = v[-1] 
        cpu_on_normal.append(tmp[0])
        cpu_lpm_normal.append(tmp[1])
        cpu_deep_lpm_normal.append(tmp[2])
        radio_listen_normal.append(tmp[3])
        radio_transmit_normal.append(tmp[4])
        radio_off_normal.append(tmp[5])

    for k,v in dictionary_myrpl_energy.items():
            if int(k) == (server - 1):
                continue
            
            tmp = v[-1] 
            cpu_on_myrpl.append(tmp[0])
            cpu_lpm_myrpl.append(tmp[1])
            cpu_deep_lpm_myrpl.append(tmp[2])
            radio_listen_myrpl.append(tmp[3])
            radio_transmit_myrpl.append(tmp[4])
            radio_off_myrpl.append(tmp[5])


    fig = plt.figure(constrained_layout=True)
    # table = plt.table(cellText= array_paketlost, rowLabels= rows, colLabels= columns)
    gs = gridspec.GridSpec(11,3, figure= fig)
    
    ax1_1 = fig.add_subplot(gs[0,0])
    ax1_1.bar(ind,cpu_on_normal, wight, color='blue')
    ax1_1.bar(ind+wight,cpu_on_myrpl, wight, color='red')
    ax1_1.set_title('CPU ON')

    ax1_2 = fig.add_subplot(gs[0,1])
    ax1_2.bar(ind,cpu_lpm_normal, wight, color='blue')
    ax1_2.bar(ind+wight,cpu_lpm_myrpl, wight, color='red')
    ax1_2.set_title('CPU LPM')
    
    ax1_3 = fig.add_subplot(gs[0,2])
    ax1_3.bar(ind,cpu_deep_lpm_normal, wight, color='blue')
    ax1_3.bar(ind+wight,cpu_deep_lpm_myrpl, wight, color='red')
    ax1_3.set_title('CPU DEEP LPM')
    
    ax2_1 = fig.add_subplot(gs[1,0])
    ax2_1.bar(ind,radio_listen_normal, wight, color='blue')
    ax2_1.bar(ind+wight,radio_listen_myrpl, wight, color='red')
    ax2_1.set_title('RADIO LISTEN')
    
    ax2_2 = fig.add_subplot(gs[1,1])
    ax2_2.bar(ind,radio_transmit_normal, wight, color='blue')
    ax2_2.bar(ind+wight,radio_transmit_myrpl, wight, color='red')
    ax2_2.set_title('RADIO TRANSMIT')
    
    ax2_3 = fig.add_subplot(gs[1,2])
    ax2_3.bar(ind,radio_off_normal, wight, color='blue')
    ax2_3.bar(ind+wight,radio_off_myrpl, wight, color='red')
    ax2_3.set_title('RADIO OFF')
    
    ax_box = fig.add_subplot(gs[2:5,:])
    ax_box.boxplot(dictionary_normal_times.values())
    ax_box.set_title('RTTs normal')

    ax_box = fig.add_subplot(gs[6:9,:])
    ax_box.boxplot(dictionary_myrpl_times.values())
    ax_box.set_title('RTTs myrpl')
    
    table_lost = fig.add_subplot(gs[10,:])
    table_lost.axis('tight')
    table_lost.axis('off')
    table_lost.table(cellText= cell_text, rowLabels= rows, colLabels= columns, loc='center')

    plt.show()


def parser(logfile_normal, logfile_myrpl):
    global server
    global node_number
    global dictionary_normal_all
    global dictionary_normal_times
    global dictionary_normal_energy
    global array_normal_paketlost  
    global array_normal_paketsend
    
    global dictionary_myrpl_all
    global dictionary_myrpl_times
    global dictionary_myrpl_energy
    global array_myrpl_paketlost  
    global array_myrpl_paketsend


    dictionary_normal_times = {new_list: [] for new_list in range(node_number)}
    dictionary_normal_energy = {new_list: [] for new_list in range(node_number)}
    dictionary_normal_all = {new_list: [] for new_list in range(node_number)}
    array_normal_paketlost = [0] * node_number
    array_normal_paketsend = [0] * node_number
    
    dictionary_myrpl_times = {new_list: [] for new_list in range(node_number)}
    dictionary_myrpl_energy = {new_list: [] for new_list in range(node_number)}
    dictionary_myrpl_all = {new_list: [] for new_list in range(node_number)}
    array_myrpl_paketlost = [0] * node_number
    array_myrpl_paketsend = [0] * node_number

    array_data_tmp = []
    array_cpu_tmp = []
 
    app_regex = r"\[INFO:\s+App\s+\]" 
    cpu_sending_regex = r'Sending CPU'
    data_sending_regex = r'Sending [^(CPU)]'
    cpu_received_regex = r"Received response 'CPU"
    data_received_regex = r"Received response '[^(CPU)]"
    
    logfile_normal = open(logfile_normal, 'r')
    
    # Traffic Infos der APPs aus Logfile filtern, andere Logs z.B. MAIN SKY ... 
    # werden ignoriert
    traffic_normal_all = logfile_normal.readlines()
    traffic_normal_data = [line for line in traffic_normal_all if re.search(app_regex, line) is not None]
    
    # nach Node id filtern 
    for idx in range(node_number):
        id_regex = 'ID:' + str(idx + 1)
        traffic = [line for line in traffic_normal_data if re.search(id_regex, line) is not None]
        dictionary_normal_all[idx].extend(traffic)

    # Time deltas der einzelnen Pakete errechnen
    for k,v in dictionary_normal_all.items():
        if int(k) == (server - 1):
            continue
        for i in v:
            if re.search(data_sending_regex, i) is not None:
                x = re.findall(r'\d+', i)
                array_data_tmp.append(int(x[2]))
                array_data_tmp.append(int(x[0]))
                    
            elif re.search(cpu_sending_regex, i) is not None:
                x = re.findall(r'\d+', i)
                array_cpu_tmp.append(int(x[3]))
                array_cpu_tmp.append(int(x[0]))

            elif re.search(data_received_regex, i) is not None:
                x = re.findall(r'\d+', i)
                delta_data_time = 0
                while True:
                    if int(x[2]) in array_data_tmp:
                        idx = array_data_tmp.index(int(x[2]))
                        array_data_tmp.remove(int(x[2]))
                        data_time_tmp =array_data_tmp.pop(idx)
                        delta_data_time = int(x[0]) - data_time_tmp
                    if delta_data_time < 50000:
                        break
                    else:
                        array_normal_paketlost[k] += 1
                dictionary_normal_times[k].append(delta_data_time)
                array_normal_paketsend[k] += 1

            elif re.search(cpu_received_regex, i) is not None:
                x = re.findall(r'\d+', i)
                delta_cpu_time = 0
                while True:
                    if int(x[3]) in array_cpu_tmp:
                        idx = array_cpu_tmp.index(int(x[3]))
                        array_cpu_tmp.remove(int(x[3]))
                        cpu_time_tmp = array_cpu_tmp.pop(idx)
                        delta_cpu_time = int(x[0]) - cpu_time_tmp
                    if delta_cpu_time < 50000:
                        break
                    else:
                        array_normal_paketlost[k] += 1
                dictionary_normal_energy[k].append(delta_cpu_time)
                array_normal_paketsend[k] += 1

                y = re.split("'", i)
                z = re.findall(r"\d+", y[1])
                z = [int(i) for i in z]
                w = tuple(z)
                dictionary_normal_energy[k].append(w)
        array_normal_paketlost[k] += len(array_data_tmp) / 2
        array_normal_paketlost[k] += len(array_cpu_tmp) / 2
        array_data_tmp = []
        array_cpu_tmp = []
    
    logfile_normal.close()
    
    logfile_myrpl = open(logfile_myrpl, 'r')

    # Traffic Infos der APPs aus Logfile filtern, andere Logs z.B. MAIN SKY ... 
    # werden ignoriert
    traffic_myrpl_all = logfile_myrpl.readlines()
    traffic_myrpl_data = [line for line in traffic_myrpl_all if re.search(app_regex, line) is not None]

    # nach Node id filtern 
    for idx in range(node_number):
        id_regex = 'ID:' + str(idx + 1)
        traffic = [line for line in traffic_myrpl_data if re.search(id_regex, line) is not None]
        dictionary_myrpl_all[idx].extend(traffic)

    # Time deltas der einzelnen Pakete errechnen
    for k,v in dictionary_myrpl_all.items():
        if int(k) == (server - 1):
            continue
        for i in v:
            if re.search(data_sending_regex, i) is not None:
                x = re.findall(r'\d+', i)
                array_data_tmp.append(int(x[2]))
                array_data_tmp.append(int(x[0]))
                    
            elif re.search(cpu_sending_regex, i) is not None:
                x = re.findall(r'\d+', i)
                array_cpu_tmp.append(int(x[3]))
                array_cpu_tmp.append(int(x[0]))

            elif re.search(data_received_regex, i) is not None:
                x = re.findall(r'\d+', i)
                delta_data_time = 0
                while True:
                    if int(x[2]) in array_data_tmp:
                        idx = array_data_tmp.index(int(x[2]))
                        array_data_tmp.remove(int(x[2]))
                        data_time_tmp =array_data_tmp.pop(idx)
                        delta_data_time = int(x[0]) - data_time_tmp
                    if delta_data_time < 50000:
                        break
                    else:
                        array_myrpl_paketlost[k] += 1
                dictionary_myrpl_times[k].append(delta_data_time)
                array_myrpl_paketsend[k] += 1

            elif re.search(cpu_received_regex, i) is not None:
                x = re.findall(r'\d+', i)
                delta_cpu_time = 0
                while True:
                    if int(x[3]) in array_cpu_tmp:
                        idx = array_cpu_tmp.index(int(x[3]))
                        array_cpu_tmp.remove(int(x[3]))
                        cpu_time_tmp = array_cpu_tmp.pop(idx)
                        delta_cpu_time = int(x[0]) - cpu_time_tmp
                    if delta_cpu_time < 50000:
                        break
                    else:
                        array_myrpl_paketlost[k] += 1
                dictionary_myrpl_energy[k].append(delta_cpu_time)
                array_myrpl_paketsend[k] += 1

                y = re.split("'", i)
                z = re.findall(r"\d+", y[1])
                z = [int(i) for i in z]
                w = tuple(z)
                dictionary_myrpl_energy[k].append(w)
        array_myrpl_paketlost[k] += len(array_data_tmp) / 2
        array_myrpl_paketlost[k] += len(array_cpu_tmp) / 2
        array_data_tmp = []
        array_cpu_tmp = []

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
