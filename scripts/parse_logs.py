import sys
import os

if len(sys.argv) < 4:
    print("Need to include parameters. Usage Example: python3 parse_logs.py <log_type> <directory_name> <experiment_count>")

log_type = sys.argv[1]
if log_type != "network" and log_type != "cpu":
    print("Error: log type parameter needs to be \"network\" or \"cpu\"")

try: 
    log_path = sys.argv[2]
    os.path.isdir(log_path)
except Exception as e:
    print("Error: log directory path is incorrect")

try:
    exp_count = int(sys.argv[3])
    print("Parsing Usage for up to experiment count: %d" %(exp_count))
except Exception as e:
    print("Error: experiment count input should be an integer")


def get_rate_multiplier(rate):
	if "KB" in rate:
		return 1000
	elif "MB" in rate:
		return 1000 * 1000
	elif "GB" in rate:
		return 1000 * 1000 * 1000
	else:
		return 1

def split_unit_from_rate(rate):
	if "KB" in rate:
		return rate.split("KB")[0]
	elif "MB" in rate:
		return rate.split("MB")[0]
	elif "GB" in rate:
		return rate.split("GB")[0]
	else:
		return rate.split("B")[0]

if log_type == "cpu":
	for experiment_index in range(exp_count):
		with open(log_path + "/" + str(experiment_index)) as f:
			file_lines = f.read().split("\n")

			usage_values = []
			for line in file_lines:
				if "all" in line:
					usage_data = float(line.split("all")[1].strip().split(" ")[0])
					usage_data = round(100 - float(line.split(" ")[-1]), 2)
					usage_values.append(usage_data)

			#print(usage_values)

			mean = sum(usage_values) / len(usage_values)

			retained_usage_values = []
			for i in range(len(usage_values)):
				ommit = "OOO"

				if usage_values[i] < mean:
					ommit = "XXX"
				else:
					retained_usage_values.append(usage_values[i])

				#print(str(usage_values[i]) + ":   " + ommit)
			
			true_mean = round(sum(retained_usage_values) / len(retained_usage_values), 2)
			print(str(experiment_index) + " " + str(true_mean))


elif log_type == "network":
	for experiment_index in range(exp_count):
		with open(log_path + "/" + str(experiment_index)) as f:
			file_lines = f.read().split("\n")

			network_out_usage_values = []
			network_in_usage_values = []

			for line in file_lines:
				if "Upload Speed:" and "Download Speed:" in line:
					line_split = line.split("Download Speed:")
					upload_rate = line_split[0].split("Upload Speed:")[1]
					download_rate = line_split[1]

					upload_rate = float(split_unit_from_rate(upload_rate)) * get_rate_multiplier(upload_rate)
					download_rate = float(split_unit_from_rate(download_rate)) * get_rate_multiplier(download_rate)
					
					network_out_usage_values.append(upload_rate)
					network_in_usage_values.append(download_rate)

			network_out_mean = sum(network_out_usage_values) / len(network_out_usage_values)
			network_in_mean = sum(network_in_usage_values) / len(network_in_usage_values)

			# print("Network out mean: " + str(network_out_mean))
			# print("Network in mean: " + str(network_in_mean))

			retained_network_out_usage_values = []
			retained_network_in_usage_values = []

			for i in range(len(network_out_usage_values)):
				ommit = "OOO"

				if network_out_usage_values[i] < network_out_mean:
					ommit = "XXX"
				else:
					retained_network_out_usage_values.append(network_out_usage_values[i])

			for i in range(len(network_in_usage_values)):
				ommit = "OOO"

				if network_in_usage_values[i] < network_in_mean:
					ommit = "XXX"
				else:
					retained_network_in_usage_values.append(network_in_usage_values[i])


			# print(retained_network_out_usage_values)
			# print(retained_network_in_usage_values)
			
			true_network_out_mean = sum(retained_network_out_usage_values) / len(retained_network_out_usage_values)
			true_network_in_mean = sum(retained_network_in_usage_values) / len(retained_network_in_usage_values)

			# print(str(experiment_index) + " NO   " + str(true_network_out_mean))
			# print(str(experiment_index) + " NI   " + str(true_network_in_mean))

			print(str(experiment_index) + " " + str(true_network_out_mean) + " " + str(true_network_in_mean))


