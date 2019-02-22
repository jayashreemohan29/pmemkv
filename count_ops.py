#!/usr/bin/env python
import getopt
import sys

def main():
	
	#input arg : file name
	if (len(sys.argv) != 2):
		print("Please input log file")
		exit(1)
	log_file = sys.argv[1]

	count = 0
	count_fence = 0
	count_flush = 0
	count_store = 0
	count_others = 0	

	operations = open(log_file).read().split("|")
	for op in operations:
		count+=1
		op_value = op.split(";")[0]
		if (op_value == "FENCE"):
			count_fence += 1
		elif (op_value == "STORE"):
			count_store += 1
		elif (op_value == "FLUSH"):
			count_flush += 1
		else:
			count_others +=1
			#print(str(op))
	print("\n----- OP SUMMARY --------\n")
	print("\n Total Ops : " + str(count))
	print("\n Total store ops : " + str(count_store))
	print("\n Total flush ops : " + str(count_flush))
	print("\n Total fence ops : " + str(count_fence))
	print("\n Other ops : " + str(count_others))

if __name__ == "__main__":
	main()
