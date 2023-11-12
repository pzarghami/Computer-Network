
import numpy as np
import matplotlib.pyplot as plt

def throughput(filename):
	trace = open(filename ,  'r')
	recvdSize = 0
	startTime = 1e6
	stopTime = 0
	for line in trace:
		words = line.split(' ')
		packet_size = int(words[36])
		time = float(words[2])
		if line.startswith('s') and 'AGT' in line and packet_size > 512:
			if  time < startTime:
				startTime = time 
		if line.startswith('r') and 'AGT' in line and packet_size > 512:
			if time > stopTime: 
				stopTime = time
			packet_size -= packet_size % 512
			recvdSize += packet_size
	throughput = recvdSize / (stopTime - startTime)*(8/1000)
	trace.close()
	return throughput	

def packet_transfer_rate(filename):
	trace = open(filename ,  'r')
	max = 0
	for line in trace:
		words = line.split(' ')
		index = line.find("-Ps")
		if index != -1:
			seq_num_str = line[index+4:]
			seq_words = seq_num_str.split(' ')
			seq_num = seq_words[0]	
			if line.startswith('f') and 'tcp' in line and int(seq_num) > max:
				max = int(seq_num)
				time = float(words[2])
	trace.close()
	return max/time

def end_to_end_delay(filename):
	trace = open(filename ,  'r')
	_max = 0

	for line in trace:
		index = line.find("-Ps")
		if index != -1:
			seq_num_str = line[index+4:]
			seq_words = seq_num_str.split(' ')
			seq_num = seq_words[0]	
			if int(seq_num) > _max:
				_max = int(seq_num)
	
	start_time = [-1]*_max
	end_time = [-1]*_max

	trace1 = open(filename ,  'r')
	for line in trace1:
		word = line.split(' ')
		index = line.find("-Ps")
		if index != -1:
			seq_num_str = line[index+4:]
			seq_words = seq_num_str.split(' ')
			seq_num = int(seq_words[0])
			if line.startswith('s') and "AGT" in line and start_time[seq_num-1] == -1:
				start_time[seq_num-1] = float(word[2])
			if line.startswith('r') and 'tcp' in line:
				end_time[seq_num-1] = float(word[2])
		
	packet_duration = 0
	for packet_id in range(_max):
		start = start_time[packet_id]
		end = end_time[packet_id]
		packet_duration += end - start
	trace.close()

	return packet_duration/_max 

def main():

	# throughput_array = [0.0]*5
	# throughput_array[0] = throughput("out0.tr", 100) 
	# throughput_array[1] = throughput("out1.tr", 500) 
	# throughput_array[2] = throughput("out2.tr", 1000) 
	# throughput_array[3] = throughput("out3.tr", 2000)
	# throughput_array[4] = throughput("out4.tr", 5000)  	
	# print(throughput_array)

	# xpoints = np.array([100, 500, 1000, 2000, 5000])
	# plt.plot(xpoints, throughput_array, 'o') 
	# plt.show() 

	print("throughput is: ", throughput("out.tr"))
	print("packet transfer ratio is: ", packet_transfer_rate("out.tr"))
	print("end to end delay is: ", end_to_end_delay("out.tr"))

if __name__ == '__main__':
	main()
