# QUICKeR Artifacts

Paper title: **QUICKeR: Quicker Updates Involving Continuous Key Rotation**

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Description
We provide a GitHub repository for source code of our experiments and Amazon Machine Image (AMI) to run our experiments. The source code is what we compiled and ran for our bottleneck experiments and end-to-end experiments. The AMI is publicly available in us-east-2 with AMI ID ami-0ef8a18124a84e595. Please refer to commit-id 92b1207 of this repository for the artifacts used in our paper.

This is a GitHub reposoritory containing source code for experiments in our paper **QUICKeR: Quicker Updates Involving Continuous Key Rotation**. The experiments included in the source code involve the end-to-end experiments (which requires a Hardware Security Module (HSM)) and bottleneck experiments (which does not). **Important note: Our source code is dependent on Amazon Web Service's (AWS's) CloudHSM 3, which is no longer available, so it will not compile on any machine without that dependency. Instead, we provide a publicly available AMI, which has CloudHSM3 and other dependencies already installed. You can find the AMI on AWS in us-east-2 with AMI ID ami-0ef8a18124a84e595.**

Our code is divided into two parts: C++ source code under [src](src) and Python3 experiment runner code under [scripts](scripts). The C++ code contains source code for our end-to-end experiments and bottleneck experiments, while the Python3 code invokes the compiled binaries generated from compiling the C++ to start the experiments and monitors and outputs experimental results.

Python3 code:
| Filename                                                                 | Description                                                                                    |
|:-------------------------------------------------------------------------|------------------------------------------------------------------------------------------------|
| [scripts/runExperimentScript.py](scripts/runExperimentScript.py)         | Runner script for the end-to-end experiments                                                   |
| [scripts/runSimpleScript.py](scripts/runSimpleScript.py)                 | Runner script for the bottleneck experiments                                                   |
| [scripts/cpu_usage_logs](scripts/cpu_usage_logs)                         | Contains output cpu utilization metrics from running experiments                               |
| [scripts/network_usage_logs](scripts/network_usage_logs)                 | Contains network utilization metrics from running experiments                                  |
| [scripts/parse_logs.py](scripts/parse_logs.py)                           | Script that can parse cpu and network utilization                                              |
| [scripts/experiment_script_jobs.txt](scripts/experiment_script_jobs.txt) | A json file containing a list of jobs for end-to-end experiments                               |
| [scripts/simple_script_jobs.txt](scripts/simple_script_jobs.txt)         | A json file containing a list of jobs for bottleneck experiments                               |
| [scripts/populate_list.py](scripts/populate_list.py)                     | A script generating [src/list.txt](src/list.txt) for the indices in the end-to-end experiments |

C++ source code:
| Filename                                                                 | Description                                                                                    |
|:-------------------------------------------------------------------------|------------------------------------------------------------------------------------------------|
| [src/quicker_server.cpp](src/quicker_server.cpp)                         | Database server for end-to-end QUICKeR system and experiments                                  |
| [src/quicker_client.cpp](src/quicker_client.cpp)                         | Client for end-to-end QUICKeR system and experiments                                           |
| [src/quicker_update_client.cpp](src/quicker_update_client.cpp)           | Update Client for end-to-end QUICKeR system and experiments                                    |
| [src/quicker_init.cpp](src/quicker_init.cpp)                             | Initialization code for the end-to-end experiments                                             |
| [src/server.cpp](src/server.cpp)                                         | Database server for bottleneck experiments                                                     |
| [src/client.cpp](src/client.cpp)                                         | Client for bottleneck experiments                                                              |
| [src/update_client.cpp](src/update_client.cpp)                           | Update Client for bottleneck experiments                                                       |
| [src/ue_interface.h](src/ue_interface.h)                                 | Contains updatable encryption base wrapper class that updatable encryption schemes extend      |
| [src/ue_interface.cpp](src/ue_interface.cpp)                             | Factory design pattern creates the appropriate updatable encryption scheme                     |
| [src/ue_schemes](src/ue_schemes)                                         | Folder containing source code for source code of the updatable encryption schemes              |
| [src/ue_wrappers](src/ue_wrappers)                                       | Folder containing wrappers files for the updatable encryption schemes that extends [src/ue_interface.h](src/ue_interface.h) |
| [src/utils.cpp](src/utils.cpp) and [src/utils.h](src/utils.h)            | Contains useful functions such as implementations of communication protocols                   |
| [src/actions](src/actions)                                               | Folder containing source code for implementations of SecurePut, SecureGet, CtxtUpdate, etc.    |
| [src/update_queue](src/update_queue)                                     | Folder containing source code for implementations of an update queue                           |
| [src/Makefile](src/Makefile)                                             | Makefile compiles the source code                                                              |



### Security/Privacy Issues and Ethical Concerns
No security or Privacy Issues or Ethical Concerns to report.


## Basic Requirements
The Bottleneck experiments can be run on a set of three machines (for a client, an update client, and a database), while the end-to-end experiments additionally require Hardware Security Modules (HSMs). One experiment runs for 200 seconds, but if setup time is included, then it may take as long as 300 seconds.


### Hardware Requirements


All of our experiments require using Amazon Web Services (AWS) machines to reproduce. Bottleneck experiments were run on r5n.8xlarge machines for the client and update client machines and on a r5n.2xlarge, t3.2xlarge, and r5n.8xlarge for the database. For the end-to-end experiments, all three machines used r5n.8xlarge machines and three HSMs properly configured. 


### Software Requirements


Since we require old dependencies for our code to work, we provide an Amazon Machine Instance (AMI) that has the required software and packages installed. Specifically, our end-to-end experiments use AWS’s Cloud HSM SDK 3 which can no longer be downloaded, and our code is incompatible with CloudHSM SDK 5 (the latest version).

#### Dependencies
Additionally, we provide a list of dependencies here (the AMI has already been preinstalled with all the dependencies).

C/C++ Dependencies
* CloudHSM SDK 3 (no longer accessible)
* make
* gcc-c++
* openssl-devel

Python Dependencies
* boto3
* botocore
* paramiko
* psutil


### Estimated Time and Storage Consumption
An execution of 1 end-to-end experiment or bottleneck experiment (using runExperimentScript.py) is expected to take about 250 or so seconds. Note that one experiment is one dictionary object in the experiment_script_jobs.txt and simple_script_jobs.txt. 


In terms of storage, the machines acting as the update machine and the client machine should not take up any additional storage outside of the source code. The machine acting as the database server will take (bytes of each plaintext) * (number of plaintext) amount of additional space depending on the specs the user assigns in the xxxx_jobs.txt files. Note that the maximum storage usage is equal to the experiment with the largest storage usage, not the sum of all experiment's usage. 


## Environment




### Accessibility
On AWS, we provide an AMI located in us-east-2 with AMI ID ami-0ef8a18124a84e595. 


Our public GitHub repository is provided in this repository: https://github.com/lawrencekhlim/QUICKeR




### Set up the environment


Create three machines using the AMI ami-0ef8a18124a84e595 located in us-east-2. This should have the necessary dependencies installed already.

Designate one of them to be the database server, another to be the client machine, and the last one to be the update machine. Place a copy of the .pem file used to access (or ssh into) the machines in the database server (for instance, via a scp command). This is necessary for the program runner (on the database) to connect to the client and update machines and tell them to start the experiment.


For the end-to-end experiments, set up HSMs [(by following AWS’s instructions)](https://docs.aws.amazon.com/cloudhsm/latest/userguide/initialize-cluster.html). Ensure the machines can connect to the HSMs with the correct security groups enabled, by having the certificate named and placed in the right location `/opt/cloudhsm/etc/customerCA.crt`, and by running 
```bash
sudo service cloudhsm-client stop
sudo /opt/cloudhsm/bin/configure -a <HSM IP Address>
sudo service cloudhsm-client start
``` 

(Also for the end-to-end experiments), create a CryptoUser in the HSMs and a password [(AWS instructions here)](https://docs.aws.amazon.com/cloudhsm/latest/userguide/create-user-cloudhsm-cli.html). The username and password are needed to run the end-to-end experiments.


Clone the repository and compile code in all three machines
```bash
git clone https://github.com/lawrencekhlim/QUICKeR.git
cd QUICKeR/src
make all
cd ..
```


### Testing the Environment


To test whether the HSM is working correctly
```bash
/opt/cloudhsm/bin/key_mgmt_util
```
should access the HSM and prompt for a list of available commands. If that works, then try logging in with the created crypto user as well.


Ensure that the machines are able to communicate with each other (via pings, for instance).


## Artifact Evaluation
### Main Results and Claims
#### Main Result 1: Different bottlenecks result in different performance 
By changing the machine for the database between bottlenecks for CPU, network, and locking, it results in different performance for different key rotation and ciphertext update schemes (Figure 11, Section 6.2).


#### Main Result 2: QUICKeR’s Multi-threaded updatable encryption scheme performed the best at higher update throughput
Section 6.3 discusses the end-to-end experiments, with Figure 12 accompanying it. 


### Experiments


#### Experiment 1: Bottleneck Experiments


In this experiment, we varied the database and number of update threads while keeping everything else the same. 


To run the experiments, run the following commands on the database 
```bash
cd scripts
python3 runSimpleScript.py <server_ip_addr> <client_ip_addr> <update_ip_addr> <pem_file_path>
```
	- ip address is the ip address of the machine you'd like to run that machine type on
	- pem_file_path is the path to the private key to ssh into all three aws instances (server, client, update client)


Once you run the above steps, you can find the results in res.txt. The columns in res.txt are separated by space and are organized as such: 
"index" "ue_scheme" "plaintext size" "multithreaded" "number of client machine threads" "number of update machine threads" "experiment time" "number of write operations" "number of read operations" "number of update operations" 


Cpu and network utilization can be found in the folders "cpu_usage_logs" and "network_usage_logs" respectively. Each file will be labeled with an index number and will correspond one to one with the index found in res.txt. The files will show the state of cpu/network utilization every 5 seconds. To get an aggregate average per file of each utilization, run parse_logs.py. 
```python3 parse_logs.py <log_type> <directory_name> <experiment_count>```
	
* log_type is either "network" or "cpu"
* directory_name is either "network_usage_logs" or "cpu_usage_logs"
* experiment_count is the number of experiments you want to parse logs for in that folder. ie. if you give 10 here, it will parse files 0,1,2,3,4,5,6,7,8,9 and show the average utilization for those files




#### Experiment 2: End-to-end Experiments
To run the experiments, run the following commands on the database
```bash
cd scripts
python3 runExperimentScript.py <server_ip_addr> <client_ip_addr> <update_ip_addr> <pem_file_path> <crypto_user_name> <crypto_user_password>
```

* ip address is the ip address of the machine you'd like to run that machine type on
* pem_file_path is the path to the private key to ssh into all three aws instances (server, client, update client)
* crypto_user_name is the username of the crypto user created in the HSM
* crypto_user_password is the password of that crypto user


Once you run the above steps, you can find the results in res.txt. The columns in res.txt are separated by space and are organized as such: 
"index" "ue_scheme" "plaintext size" "multithreaded" "number of client machine threads" "number of update machine threads" "experiment time" "number of write operations" "number of read operations" "number of update operations" 


## Limitations


We do not include Microbenchmarks (Table 1) because these experiments require (taking) code from other authors and their research papers (although we keep UAE Bounded from these [author's source code](https://github.com/moshih/UpdateableEncryption_Code)). 


## Notes on Reusability

The code can be extended to support more updatable encryption algorithms. This code should not be used in production code as has not been checked for bugs.
