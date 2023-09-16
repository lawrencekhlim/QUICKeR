import boto3
import botocore
import paramiko
import sys
import threading
import time
import datetime
import os
import psutil
import json

if len(sys.argv) < 4:
  print("Need arguments, example: python3 runExperimentScript.py <server_ip_addr> <client_ip_addr> <update_ip_addr> <pem_file_path>")
  exit(0)


# Read a file to get the starting port. Just used to change up the port number so I don't have to keep changing it manually if the port is still in use
f = open("starting_port.txt", "r+")
starting_port = int(f.read()) + 400
if starting_port >= 20000:
    starting_port = 8000
f.seek(0)
f.write(str(starting_port))
f.truncate()
f.close()

#starting_port = 8000
print("STARTING PORT: %d" %starting_port)
run_seed = 101
run_psutil = 0
num_repeated_runs = 1

server_ip_addr = sys.argv[1]
client_ip_addr = sys.argv[2]
update_ip_addr = sys.argv[3]
pem_file = sys.argv[4]

update_thread_is_joined = False
client_thread_is_joined = False

job_dict = []
try: 
    with open("simple_script_jobs.txt", "r+") as json_file:
        job_dict = json.loads(json_file.read())
except Exception as e:
    print(str(e))

thread_return = ["", "", "", "", "", ""]



def get_size(bytes):
    """
    Returns size of bytes in a nice format
    """
    for unit in ['', 'K', 'M', 'G', 'T', 'P']:
        if bytes < 1024:
            return f"{bytes:.2f}{unit}B"
        bytes /= 1024


def join_update_thread(update_thread):
    global update_thread_is_joined

    update_thread.join()
    update_thread_is_joined = True


def join_client_thread(client_thread):
    global client_thread_is_joined

    client_thread.join()
    client_thread_is_joined = True


def call_to_instance(cmd, ip_address, return_index):
    global thread_return

    print("connecting to %s" %(ip_address))

    key = paramiko.RSAKey.from_private_key_file(pem_file)
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    print("cmd: %s\n" %cmd)

    # Connect/ssh to an instance
    # Here 'ubuntu' is user name and 'instance_ip' is public IP of EC2
    #cmd = "echo happy"
    success = 0
    while success != 1:
        try:
            client.connect(hostname=ip_address, username="ec2-user", pkey=key)
            success = 1
        except Exception as e:
            print("Error: " + str(e))

    # Execute a command(cmd) after connecting/ssh to an instance
    stdin, stdout, stderr = client.exec_command(cmd)

    output = stdout.read().decode('ascii')
    thread_return[return_index] = output
    
    # close the client connection once the job is done
    client.close()

    del client, stdin, stdout, stderr

    return output



def call_psutil(return_index):
    global run_psutil
    run_psutil = 1

    # get the network I/O stats from psutil
    io = psutil.net_io_counters()
    # extract the total bytes sent and received
    bytes_sent, bytes_recv = io.bytes_sent, io.bytes_recv
    UPDATE_DELAY = 5

    total_output = []
    while run_psutil == 1:
        #print(run_psutil)
        # sleep for `UPDATE_DELAY` seconds
        time.sleep(UPDATE_DELAY)
        # get the stats again
        io_2 = psutil.net_io_counters()
        # new - old stats gets us the speed
        us, ds = io_2.bytes_sent - bytes_sent, io_2.bytes_recv - bytes_recv
        # print the total download/upload along with current speeds

        output = "Upload: %s, Download: %s, Upload Speed: %s/s, Download Speed: %s/s\n" %(get_size(io_2.bytes_sent), get_size(io_2.bytes_recv), get_size(us / UPDATE_DELAY), get_size(ds / UPDATE_DELAY))
        #print(output)
        total_output.append(output)

        # # update the bytes_sent and bytes_recv for next iteration
        bytes_sent, bytes_recv = io_2.bytes_sent, io_2.bytes_recv
    thread_return[return_index] = "".join(total_output)

#call_to_instance(initialize_pkcs11_cmd, "172.31.45.70")

# call_to_instance("cd UE_Bench_Private/src/; make QUICKERUPDATECLIENT", update_ip_addr, 0)
# print(thread_return[0])
# call_to_instance("cd UE_Bench_Private/src/; make QUICKERINIT", client_ip_addr, 0)
# call_to_instance("cd UE_Bench_Private/src/; make QUICKERCLIENT", client_ip_addr, 0)
# call_to_instance("cd UE_Bench_Private/src/; make QUICKERSERVER", server_ip_addr, 0)

f = open("res.txt", "a")
f.write("----------------------------------------------------------------------------\n")
f.close()


f = open("failed_jobs.txt", "a")
f.write("----------------------------------------------------------------------------\n")
f.close()


failed_jobs = []
port_index = 0
for j in range (num_repeated_runs):
    run_seed += 1000
    #for i in range(len(job_dict)):
    i = 0
    while (i < len(job_dict)):

        job_success_status = "PASS"
    
        job = job_dict[i]
        true_index = (j * len(job_dict) + i)
        PORT = starting_port + (port_index%10 * 200)

        #SETUP 
        ue_scheme = job["scheme"]
        ptxt_size = str(job["ptxt_size"])
        num_of_messages = str(job["num_of_msg"])
        read_proportion = str(job["read_proportion"])
        num_server_threads = str(job["num_client_threads"] + job["num_update_threads"] + 20)
        num_init_threads = str(job["num_init_threads"])
        num_client_threads = str(job["num_client_threads"])
        num_update_threads = str(job["num_update_threads"])
        is_multithreaded = job["is_multi_threaded"]

        server_port = str(PORT)
        client_port = str(PORT)
        update_port = str(PORT + job["num_client_threads"] + 10)
        pin = "--pin bob:Applejack2022"

        #initialize_pkcs11_cmd = "sudo service cloudhsm-client stop; sudo /opt/cloudhsm/bin/configure -a " + hsm_ip_addr #+ "; sudo service cloudhsm-client start"
    

        server_cmd = "cd UE_Bench_Private/src/; ./server " + ue_scheme + " " + num_server_threads + " " + server_port + " " + is_multithreaded
        init_cmd = "cd UE_Bench_Private/src/; ./quicker_init " + ue_scheme + " " + ptxt_size + " " + num_of_messages + " " + num_init_threads + " " + server_ip_addr + " " + server_port + " list.txt " + pin
        
        # Starting Statistics Monitor (CPU Usage)
        mpstat_thread = threading.Thread(target=call_to_instance, args=("mpstat 5 200", server_ip_addr, 4,))
        mpstat_thread.start()

        # Starting Statistics Monitor (Network Usage)
        getNetworkUsage_thread = threading.Thread(target=call_psutil, args=(5,))
        getNetworkUsage_thread.start()
   

        print("Starting Server")
        server_thread = threading.Thread(target=call_to_instance, args=(server_cmd,server_ip_addr,3,))
        server_thread.start()
        time.sleep(10)


        print("Starting Client")
        client_cmd = "cd UE_Bench_Private/src/; ./client " + ue_scheme + " " + ptxt_size + " " + num_client_threads + " " + server_ip_addr + " " + client_port + " " + read_proportion + " " + str(run_seed)
        client_thread = threading.Thread(target=call_to_instance, args=(client_cmd, client_ip_addr,1,))
        client_thread.start()


        print("Starting Update Client")
        update_cmd = "cd UE_Bench_Private/src/; ./update_client " + ue_scheme + " " + ptxt_size + " " + num_update_threads + " " + server_ip_addr + " " + update_port
        update_thread = threading.Thread(target=call_to_instance, args=(update_cmd, update_ip_addr,2,))
        update_thread.start()


        # Joining with client_thread
        print("Joining with client thread\n")
        client_thread_is_joined = False
        joining_client_thread = threading.Thread(target=join_client_thread, args=(client_thread,))
        joining_client_thread.start()

        time.sleep(250);
        print("WAITING ON CLIENT THREAD")

        read_ops = 0
        write_ops = 0
        if (client_thread_is_joined): 
            print("Client thread has joined gracefully")
            client_output = thread_return[1] 
            print(client_output)
            parse_read_ops_string = "read_ops: "
            parse_write_ops_string = "write_ops: "
            print("Parsing Read and Write ops from client\n")
            for line in client_output.split("\n"):
                if parse_read_ops_string in line:
                    read_ops = int(line.split(parse_read_ops_string)[1])
                if parse_write_ops_string in line:
                    write_ops = int(line.split(parse_write_ops_string)[1])
            print("Read Ops: %d" %read_ops)
            print("Write Ops: %d" %write_ops)
        else:
            print("Client TIMED OUT. Kill Client Thread")
            output = call_to_instance("ps aux | grep ./client | grep -v grep | grep -v UE_Bench_Private", client_ip_addr, 0)
            split_open_process = output.split()
            if (len(split_open_process) > 1):
                client_process_id = str(int(split_open_process[1]))
                print("Delete Client Process ID: %s\n" %client_process_id)
                call_to_instance("kill -9 " + client_process_id, client_ip_addr, 0)
            failed_jobs.append(job)

            f = open("failed_jobs.txt", "a")
            f.write(str(true_index) + ": " + str(job) + "\n")
            f.close()
            job_success_status = "FAIL"

        print ("Joining joining client thread\n")
        joining_client_thread.join()
        print ("Completed joining joining client thread\n")
        

        print("Joining with update thread\n")
        update_thread_is_joined = False
        joining_thread = threading.Thread(target=join_update_thread, args=(update_thread,))
        joining_thread.start()

        time.sleep(30);
        print("Done Sleeping")

        update_ops = 0
        num_round = 0
        
        if (update_thread_is_joined): 
            print("Update success")
            update_output = thread_return[2] 
            print(update_output)
            parse_update_ops_string = "ops: "
            parse_num_round_string = "num_rounds: "
            print("Parsing Read and Write ops from client\n")
            for line in update_output.split("\n"):
                if parse_update_ops_string in line:
                    update_ops = int(line.split(parse_update_ops_string)[1])
                if parse_num_round_string in line:
                    num_round = int(line.split(parse_num_round_string)[1])
            print("Update Ops: %d" %update_ops)
            print("Num of Rounds: %d" %num_round)

            if update_ops == 0 and int(num_update_threads) != 0:
                job_success_status = "FAIL"
        else:
            print("Update TIMED OUT. Kill Update Thread")
            output = call_to_instance("ps aux | grep ./update_client | grep -v grep | grep -v UE_Bench_Private", update_ip_addr, 0)
            split_open_process = output.split()
            if (len(split_open_process) > 1):
                update_process_id = str(int(split_open_process[1]))
                print("Delete Update Process ID: %s\n" %update_process_id)
                call_to_instance("kill -9 " + update_process_id, update_ip_addr, 0)
            failed_jobs.append(job)

            f = open("failed_jobs.txt", "a")
            f.write(str(true_index) + ": " + str(job) + "\n")
            f.close()

            job_success_status = "FAIL"

        print("Joining joining thread")
        joining_thread.join()
        print("Competed Joining Joining Thread")



        # Just in case Kill client machine
        output = call_to_instance("ps aux | grep ./client | grep -v grep | grep -v UE_Bench_Private", client_ip_addr, 0)
        split_open_process = output.split()
        if (len(split_open_process) > 1):
            client_process_id = str(int(split_open_process[1]))
            print("Just in case: Delete Client Process ID: %s\n" %client_process_id)
            call_to_instance("Just in case: kill -9 " + client_process_id, client_ip_addr, 0)


        # Just in case kill update machine
        output = call_to_instance("ps aux | grep ./update_client | grep -v grep | grep -v UE_Bench_Private", update_ip_addr, 0)
        split_open_process = output.split()
        if (len(split_open_process) > 1):
            update_process_id = str(int(split_open_process[1]))
            print("Just in case: Delete Update Process ID: %s\n" %update_process_id)
            call_to_instance("Just in case: kill -9 " + update_process_id, update_ip_addr, 0)


        # Delete Server # TODO: Make a loop to continuously kill till no more server
        output = call_to_instance("ps aux | grep ./server | grep -v grep | grep -v UE_Bench_Private", server_ip_addr, 0)
        split_open_process = output.split()
        if (len(split_open_process) > 1):
            server_process_id = str(int(split_open_process[1]))
            print("Delete Server Process ID: %s\n" %server_process_id)
            call_to_instance("kill -9 " + server_process_id, server_ip_addr, 0)

        print("Before Thread Joins")
        server_thread.join()
        print("Thread Join Complete. End of experiment")


        #naive   1000000 no  client update 100 wriutes gets updates 

        # Stopping Monitor for CPU Usage
        output = call_to_instance("ps aux | grep mpstat", server_ip_addr, 0)
        split_open_process = output.split()
        if (len(split_open_process) > 1):
            mpstat_process_id = str(int(split_open_process[1]))
            print("Delete mpstat Process ID: %s\n" %mpstat_process_id)
            call_to_instance("kill -9 " + mpstat_process_id, server_ip_addr, 0)

        print("Waiting 20 seconds to sync up the deletion just in case")
        time.sleep(20)

        # Stopping Monitor for network usage
        run_psutil = 0

        # Joining Monitor threads
        mpstat_thread.join()
        getNetworkUsage_thread.join()

        # Save data analysis results to files
        mpstat_res = thread_return[4]
        getNetworkUsage_res = thread_return[5]

        f = open("cpu_usage_logs/"+str(true_index), "w")
        f.write(mpstat_res)
        f.close()

        f = open("network_usage_logs/"+str(true_index), "w")
        f.write(getNetworkUsage_res)
        f.close()


        if job_success_status == "PASS":

            is_multi_word = "no"
            if is_multithreaded == "1":
                is_multi_word = "yes"
            elif is_multithreaded == "2":
                is_multi_word = "2"

            f = open("res.txt", "a")
            f.write("%d) %s %s %s %s %s %s %s %s %s\n" %(true_index, ue_scheme, ptxt_size, is_multi_word, num_client_threads, num_update_threads, "200", write_ops, read_ops, update_ops))
            f.close()

            i += 1

        port_index += 1

f = open("res.txt", "a")
f.write("----------------------------------------------------------------------------\n")
f.close()


print("================================= FAILED JOBS ==========================================\n")
for fj in failed_jobs:
    print(fj)
print("================================= FAILED JOBS ==========================================\n")

print("DONE WITH TASK")

