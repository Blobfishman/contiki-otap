#!/usr/bin/env python3
import re
from datetime import datetime
import matplotlib.pyplot as plt
import matplotlib as mpl
import pickle


filename = './grid_65100.txt'

#Öffnen der Datei und auslesen aller Zeilen
lines = []
count = 0
received = 0
last_line = []

lines_ =[]

with open(filename, 'r') as f:
	last_line = f.readlines()[-1]


fp = open(filename, 'r')
		
for line in fp:
	
	if 'RESENDING BROADCAST' in line:
		lines.append(line)
		
		for line in fp:
			if 'Update required' in line:
				count += 1
			if 'Update successfully received' in line:
				received += 1
			
			lines.append(line)
			
			if (count == received) and (received != 0):
				count = 0
				received = 0			
				for line in lines:		
					lines_.append(line)
				lines = []
				break		
			
			if last_line == line:
				lines = []
				
		
			
	




filtered_lines = []
for line in lines_ :
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
RTT_10 = []
i = '0'



#ausrechnen der RTT
def calc_rtt(RTT,i) :
	f = '0'
	j = 0
	for line in filtered_lines:
		
		#Für jede ID suchen der passenden antwort zum request
		if line[1] == 'ID:' + str(i):
				
			if  ('Sending request') in line[2]:
					
				
				b = len(line[2]) - 1				
				s = line[2][b]
				#print(s)
				#print(line[2])
				if(s == 'a'):
					s = 10
				for line_2 in filtered_lines :
					if ("Received Data") in line_2[2]:
						f = line_2[2][len(line_2[2]) - 1]
						print(f)
						if(f) == 'd':
							f = '13'
						if(f) == 'c':
							f = '12'
						if(f) == 'b':
							f = '11'
						if(f) == 'a':
							f = '10'
					l = int(f)		
					if() == str(i):
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

calc_rtt(RTT_2,2)
calc_rtt(RTT_3,3)
calc_rtt(RTT_4,4)
calc_rtt(RTT_5,5)
calc_rtt(RTT_6,6)
calc_rtt(RTT_7,7)
calc_rtt(RTT_8,8)
calc_rtt(RTT_9,9)
calc_rtt(RTT_10,13)
#print(RTT_3)




#Erstellung des Boxbplot mit ausgewerteten Daten
data = [RTT_2, RTT_3, RTT_4,RTT_5,RTT_6,RTT_7,RTT_8,RTT_9]
fig = plt.figure(1, figsize=(9,6))
ax = fig.add_subplot(111)
bp = ax.boxplot(data)
ax.set_xlabel("Node_id")
ax.set_ylabel("ms")
ax.set_title("RTT")
fig.savefig('Auswertung_update.png',bbox_inches = 'tight')				
				
		
	

