#!/usr/bin/env python3
import re
from datetime import datetime
import matplotlib.pyplot as plt
import matplotlib as mpl
import pickle



filename = './grid75100.txt'


#Öffnen der Datei und auslesen aller Zeilen
lines = []
with open(filename, 'r')as f:
	lines  = f.readlines()
		
lines_ = []
for line in lines:
	lines_.append(line)
	
	if ('RESENDING BROADCAST') in line:
		break



filtered_lines = []
for line in lines_ :
	filtered_line = re.findall("\d+:\d+\.\d+|ID\:\d+|Sending request .*|Received Data .+ .*" , line)
	if filtered_line !=0 and len(filtered_line) > 2:
		filtered_lines.append(filtered_line)


	

for line in filtered_lines:
	print(line)

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
				
				
				
				for line_2 in filtered_lines :
					if ("Received Data") in line_2[2]:
						
						f = line_2[2][len(line_2[2]) - 1]
						
						if 'd' in f:
							f = '13'
							
						if  'c' in f:
							f = '12'
						if 'b' in f:
							f = '11'
						if 'a' in f:
							f = '10'	
						if(f) == str(i):
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

l2 =[]
l3 =[]
l4 =[]
l5 =[]
l6 =[]
l7 =[]
l8 =[]
l9 =[]




try:
	with open ("RTT2" ,'rb') as fp:
		l2 = pickle.load(fp)
	with open ("RTT3" ,'rb') as fp:
		l3 = pickle.load(fp)
	with open ("RTT4" ,'rb') as fp:
		l4 = pickle.load(fp)
	with open ("RTT5" ,'rb') as fp:
		l5 = pickle.load(fp)
	with open ("RTT6" ,'rb') as fp:
		l6 = pickle.load(fp)
	with open ("RTT7" ,'rb') as fp:
		l7 = pickle.load(fp)
	with open ("RTT8" ,'rb') as fp:
		l8 = pickle.load(fp)
	with open ("RTT9" ,'rb') as fp:
		l9 = pickle.load(fp)
except:
	print("yoo")
RTT_2 = l2
RTT_3 = l3
RTT_4 = l4
RTT_5 = l5
RTT_6 = l6
RTT_7 = l7
RTT_8 = l8
RTT_9 = l9
RTT_10 = []
RTT_11 = []
RTT_12 = []
RTT_13 = []

calc_rtt(RTT_2,2)
calc_rtt(RTT_3,3)
calc_rtt(RTT_4,4)

calc_rtt(RTT_6,6)
calc_rtt(RTT_7,7)
calc_rtt(RTT_8,8)
calc_rtt(RTT_9,9)
calc_rtt(RTT_10,10)
calc_rtt(RTT_11,11)
calc_rtt(RTT_12,12)
calc_rtt(RTT_13,13)


print(RTT_3)


#Erstellung des Boxbplot mit ausgewerteten Daten
data = [RTT_2, RTT_3 ,RTT_4,RTT_5,RTT_6,RTT_7,RTT_8,RTT_9,RTT_10,RTT_11,RTT_12,RTT_13]
fig = plt.figure(1, figsize=(9,6))
ax = fig.add_subplot(111)
bp = ax.boxplot(data)
ax.set_xlabel("Node_id")
ax.set_ylabel("ms")
ax.set_title("RTT")
fig.canvas.draw()
labels = [item.get_text() for item in ax.get_xticklabels()]
for i in range(-1,12) :
	labels[i] = i + 2
ax.set_xticklabels(labels)
fig.savefig('Auswertung.png',bbox_inches = 'tight')	




