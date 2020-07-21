#!/usr/bin/env python

import matplotlib
import matplotlib.pyplot as plt 
import matplotlib.gridspec as gridspec
import re
import sys
import numpy as np

server = 0
node_number = 0
dictionary_all = {}
dictionary_times = {}
dictionary_energy = {}
array_paketlost = []
array_paketsend = []

def plot():

    columns = ['Node %d' % x for x in range(node_number)]
    rows =  ('Paket lost', 'Paket send')
    
    cell_text = []
    cell_text.append(['%d' % x for x in array_paketlost])
    cell_text.append(['%d' % x for x in array_paketsend])
    print(cell_text)
    print(len(rows))

    cpu_on = []
    cpu_lpm = []
    cpu_deep_lpm = []
    radio_listen = []
    radio_transmit = []
    radio_off = []

    for k,v in dictionary_energy.items():
        if int(k) == (server - 1):
            continue
        
        tmp = v[-1] 
        cpu_on.append(tmp[0])
        cpu_lpm.append(tmp[1])
        cpu_deep_lpm.append(tmp[2])
        radio_listen.append(tmp[3])
        radio_transmit.append(tmp[4])
        radio_off.append(tmp[5])

    fig = plt.figure(constrained_layout=True)
    # table = plt.table(cellText= array_paketlost, rowLabels= rows, colLabels= columns)
    gs = gridspec.GridSpec(8,3, figure= fig)
    
    ax1_1 = fig.add_subplot(gs[0,0])
    ax1_1.bar(np.arange(len(cpu_on)),cpu_on, align='center')
    ax1_1.set_title('CPU ON')

    ax1_2 = fig.add_subplot(gs[0,1])
    ax1_2.bar(np.arange(len(cpu_lpm)),cpu_lpm, align='center')
    ax1_2.set_title('CPU LPM')
    
    ax1_3 = fig.add_subplot(gs[0,2])
    ax1_3.bar(np.arange(len(cpu_deep_lpm)),cpu_deep_lpm, align='center')
    ax1_3.set_title('CPU DEEP LPM')
    
    ax2_1 = fig.add_subplot(gs[1,0])
    ax2_1.bar(np.arange(len(radio_listen)),radio_listen, align='center')
    ax2_1.set_title('RADIO LISTEN')
    
    ax2_2 = fig.add_subplot(gs[1,1])
    ax2_2.bar(np.arange(len(radio_transmit)),radio_transmit, align='center')
    ax2_2.set_title('RADIO TRANSMIT')
    
    ax2_3 = fig.add_subplot(gs[1,2])
    ax2_3.bar(np.arange(len(radio_off)),radio_off, align='center')
    ax2_3.set_title('RADIO OFF')
    
    ax_box = fig.add_subplot(gs[2:5,:])
    ax_box.boxplot(dictionary_times.values())
    ax_box.set_title('RTTs')

    ax_lost = fig.add_subplot(gs[6,:])
    ax_lost.bar(np.arange(len(array_paketlost)), array_paketlost, align='center')
    ax_lost.set_title('paketlost')
    
    table_lost = fig.add_subplot(gs[7,:])
    table_lost.axis('tight')
    table_lost.axis('off')
    table_lost.table(cellText= cell_text, rowLabels= rows, colLabels= columns, loc='center')

    plt.show()


def parser(logfile):
    global server
    global node_number
    global dictionary_all
    global dictionary_times
    global dictionary_energy
    global array_paketlost  
    global array_paketsend

    dictionary_times = {new_list: [] for new_list in range(node_number)}
    dictionary_energy = {new_list: [] for new_list in range(node_number)}
    dictionary_all = {new_list: [] for new_list in range(node_number)}
    array_paketlost = [0] * node_number
    array_paketsend = [0] * node_number
    array_data_tmp = []
    array_cpu_tmp = []
 
    # cpu_time_tmp = 0
    # data_time_tmp = 0
    # data_tmp = 0
    # cpu_tmp = 0
    
    app_regex = r"\[INFO:\s+App\s+\]" 
    cpu_sending_regex = r'Sending CPU'
    data_sending_regex = r'Sending [^(CPU)]'
    cpu_received_regex = r"Received response 'CPU"
    data_received_regex = r"Received response '[^(CPU)]"
    
    logfile = open(logfile, 'r')
    
    # Traffic Infos der APPs aus Logfile filtern, andere Logs z.B. MAIN SKY ... 
    # werden ignoriert
    traffic_all = logfile.readlines()
    traffic_data = [line for line in traffic_all if re.search(app_regex, line) is not None]
    
    # nach Node id filtern 
    for idx in range(node_number):
        id_regex = 'ID:' + str(idx + 1)
        traffic = [line for line in traffic_data if re.search(id_regex, line) is not None]
        dictionary_all[idx].extend(traffic)

    # Time deltas der einzelnen Pakete errechnen
    for k,v in dictionary_all.items():
        # data_send = False
        # cpu_send = False
        # server ignorieren
        if int(k) == (server - 1):
            continue
        for i in v:
            if re.search(data_sending_regex, i) is not None:
                x = re.findall(r'\d+', i)
                array_data_tmp.append(int(x[2]))
                array_data_tmp.append(int(x[0]))
                # if data_send == True and (data_tmp != int(x[2])):
                #     array_paketlost[k] += 1
                #     data_send = True
                #     data_time_tmp = int(x[0])
                #     data_tmp = int(x[2])
                # elif data_send == False:
                #     data_send = True
                #     data_time_tmp = int(x[0])
                #     data_tmp = int(x[2])
                    
            elif re.search(cpu_sending_regex, i) is not None:
                x = re.findall(r'\d+', i)
                array_cpu_tmp.append(int(x[3]))
                array_cpu_tmp.append(int(x[0]))
                # if cpu_send == True and (cpu_tmp != int(x[2])):
                #     array_paketlost[k] += 1
                #     cpu_send = True
                #     cpu_time_tmp = int(x[0])
                #     # check mit LPM
                #     cpu_tmp = int(x[3])
                # elif cpu_send == False:
                #     cpu_send = True
                #     cpu_time_tmp = int(x[0])
                #     # check mit LPM
                #     cpu_tmp = int(x[3])

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
                        array_paketlost[k] += 1
                dictionary_times[k].append(delta_data_time)
                array_paketsend[k] += 1

                # delta_time = int(x[0]) - data_time_tmp
                # if delta_time <= 10000:
                #     dictionary_times[k].append(delta_time)

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
                        array_paketlost[k] += 1
                dictionary_energy[k].append(delta_cpu_time)
                array_paketsend[k] += 1
                        
                # cpu_send = False
                # delta_cpu = int(x[0]) - cpu_time_tmp
                # if delta_cpu <= 20000:
                #     dictionary_times[k].append(delta_cpu)

                y = re.split("'", i)
                z = re.findall(r"\d+", y[1])
                z = [int(i) for i in z]
                w = tuple(z)
                dictionary_energy[k].append(w)
        array_paketlost[k] += len(array_data_tmp) / 2
        array_paketlost[k] += len(array_cpu_tmp) / 2
        array_data_tmp = []
        array_cpu_tmp = []
    
    logfile.close()

def main(args):
    global server
    global node_number
    if len(sys.argv) != 4:
        print("Error: use ./skript.py <Logfile> <Server_ID> <Number_of_Nodes>")
    logfile = str(sys.argv[1])
    server = int(sys.argv[2])
    node_number = int(sys.argv[3])
    parser(logfile)
    plot()


if __name__ == '__main__':
    main(sys.argv) 
