#!/usr/bin/env python3
import re
from datetime import datetime
import matplotlib.pyplot as plt
import matplotlib as mpl



filename = './loglistener.txt'

#Öffnen der Datei und auslesen aller Zeilen
lines = []
count = 0
received = 0

fp =  open(filename, 'r')
	
		
for line in fp:

	if 'Sending broadcast' in line:
		break
lines.append(line)

for line in fp:
	if 'Update required' in line:
		count += 1
	if 'Update successfully received' in line:
		received += 1
		
	lines.append(line)
	if (count == received) and (received != int("0")):
		print(count)
		print(received)
		fp.close()		
		break




filtered_lines = []
for line in lines :
	filtered_line = re.findall("\d+:\d+\.\d+|ID\:\d+|Sending request .*|Received Data .+ .*" , line)
	if filtered_line !=0 and len(filtered_line) > 2:
		filtered_lines.append(filtered_line)


	



RTT_1 = []
RTT_2 = []
RTT_3 = []
RTT_4 = []
RTT_5 = []
RTT_6 = []
RTT_7 = []
RTT_8 = []
RTT_9 = []
i = 0



#ausrechnen der RTT
def calc_rtt(RTT,i) :
	j = 0
	for line in filtered_lines:
		
		#Für jede ID suchen der passenden antwort zum request
		if line[1] == 'ID:' + str(i):
				
			if  ('Sending request') in line[2]:
					
				s = re.search(r'\d+', line[2]).group()
				int(s)
				
				
				for line_2 in filtered_lines :
					if ("Received Data") in line_2[2] and (line_2[2][len(line_2[2]) - 1]) == str(i):
						d = re.search(r'\d+', line_2[2]).group()
								
						
						if( s == d):
							print(line)
							print(line_2)					
						#Umwandeln der Zeiten zu datetime Objekten um sie dann zu subtrahieren 
							time_req = datetime.strptime(line[0],'%M:%S.%f')
							time_resp = datetime.strptime(line_2[0],'%M:%S.%f')
						#Ausgabe soll in ms erfolgen
							time = (time_resp - time_req).microseconds /1000
							RTT.append(time)
						
												
#Aufruf für jeden Client

calc_rtt(RTT_2,2)
calc_rtt(RTT_3,3)
calc_rtt(RTT_4,4)
calc_rtt(RTT_5,5)
calc_rtt(RTT_6,6)
calc_rtt(RTT_7,7)
calc_rtt(RTT_8,8)
calc_rtt(RTT_9,9)



#Erstellung des Boxbplot mit ausgewerteten Daten
data = [RTT_2, RTT_3, RTT_4,RTT_5,RTT_6,RTT_7,RTT_8,RTT_9]
fig = plt.figure(1, figsize=(9,6))
ax = fig.add_subplot(111)
bp = ax.boxplot(data)
fig.savefig('Auswertung_update.png',bbox_inches = 'tight')				
				
		
	

