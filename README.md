# ExRec
Experimental Framework for Reconfigurable Networks Based on Off-the-Shelf Hardware

Johannes Zerwas, Chen Avin, Stefan Schmid, and Andreas Blenk. 2021. ExReC: Experimental Framework for Reconfigurable Networks Based on Off-the-Shelf Hardware. In Symposium on Architectures for Networking and Communications Systems (ANCS ’21), December 13–16, 2021, Layfette, IN, USA. ACM, New York, NY, USA, 7 pages. https://doi.org/10.1145/3493425.3502748


This repository contains the framework presented in the above paper. More details about the architecture can be found there.



## Folder structure:
- `automation`: Python framework to orchestrate experiments. Also contains the experiment scripts.
- `do-controller`: Sources for DO OpenFlow controller, which installs the forwarding rules on the EPS
- `clkgen`: Sources for clock generator that cycles through the matchings
- `da-controller`: Sources for DA controller, which sets the circuit on the OCS and updates forwarding in the ToR emulation
- `end-host`: Source files for the ToR emulation and scheduling. VM configurations and management scripts for rack emulations and traffic generation
- `evaluation`: Preprocessing and plotting scripts
- `utils`: Some scripts to ease initial installation of requirements.


## Used Equipment
### Servers

- 4x Supermicro SuperServer 7049GP-TRT (CPU: 2x Xeon Silver 4114; Motherboard: Super X11DPG-QT, RAM: 8 x 16GB DDR4-2666 ECC, SSD: Intel 960G)
- Besides the two onboard 1G NICs, each server has two Intel X710-DA4 NICs located at PCI addresses 0000:5e:00 (abbreviated: `5e`) and 0000:af:00 (abbrev.: `af`).
- Each server has one Nvidia Tesla T4 GPU for running the ML workload.
- The servers have 128GB RAM (if you have less, update the number of numa nodes in end-host/docker/run_docker.sh)
- Assumed hostnames of the servers are `server1`, `server2`, `server3`, `server4` (used for setting MAC addresses). Update `end-host/vagrant/create-vm.sh` and `../vagrant-ml-gpu/..` if neccessary
- Two workstations run the controllers and are named rotor-ctr (rotor) and ocs-ctr (ocs). 
  The Rotor clock generator needs a dedicated DPDK capable NIC. The OCS controller also needs a dedicated NIC to control the OCS.

### Switches:
- OCS: Polatis Series 6000n 32x32
- EPS: Dell S4048-ON
- Layer2: regular layer 2 packet switch

### Transceivers:
- 32x 10G-LR SFP+ transceivers
- 32x 10G-T SFP+ Transceivers (or 10G DAC)

### Wiring
We use three somehow separated networks:
- All servers and the controller machines are connected via a 1G management network (using the onboard NICs).
- The second onboard NIC of the Supermicro servers and the dedicated NICs of the controllers are connected via a second 1G control network using a Layer 2 packet switch. 
  The OCS's management port is also connected to this network.
- The actual data plane uses the Intel X710 in the servers. We refer to a specific port via the (shortened) PCI address, e.g., server3.5e.0 is the first port of the first NIC of the server named server3.
The following table shows the connections to the OCS and to the EPS as used by the provided experiment scripts. Each connection is bidirectional, i.e., uses one in- and one out-port at the OCS and a off-the-shelf 10G SFP+ transceiver. 
The connections from the servers to the EPS can also be realized using 10G-T transceivers/DACs. The starting port at the EPS was chosen arbitrarily and has no further meaning.

| Server.nic.port | Switch.port |  | Server.nic.port | Switch.port |
| --- | --- | --- | --- | --- |
| Server1.5e.0 | EPS.9 | | Server3.5e.0 | EPS.13 |
| Server1.5e.1 | EPS.17 | | Server3.5e.1 | EPS.21 |
| Server1.5e.2 | OCS.1 | | Server3.5e.2 | OCS.5 |
| Server1.5e.3 | OCS.9 | | Server3.5e.3 | OCS.13 |
| Server1.af.0 | EPS.10 | | Server3.af.0 | EPS.14 |
| Server1.af.1 | EPS.18 | | Server3.af.1 | EPS.22 |
| Server1.af.2 | OCS.2 | | Server3.af.2 | OCS.6 |
| Server1.af.3 | OCS.10 | | Server3.af.3 | OCS.14 |
| Server2.5e.0 | EPS.11 | | Server4.5e.0 | EPS.15 |
| Server2.5e.1 | EPS.19 | | Server4.5e.1 | EPS.23 |
| Server2.5e.2 | OCS.3 | | Server4.5e.2 | OCS.7 |
| Server2.5e.3 | OCS.11 | | Server4.5e.3 | OCS.15 |
| Server2.af.0 | EPS.12 | | Server4.af.0 | EPS.16 |
| Server2.af.1 | EPS.20 | | Server4.af.1 | EPS.24 |
| Server2.af.2 | OCS.4 | | Server4.af.2 | OCS.8 |
| Server2.af.3 | OCS.12 | | Server4.af.3 | OCS.16 |

Additionally, we have some direct connections between OCS and EPS to support 3 DO case:

| Switch.Port | OCS Port |
| --- | --- |
| EPS.25 | 25 |
| EPS.26 | 26 |
| EPS.27 | 27 |
| EPS.28 | 28 |
| EPS.29 | 29 |
| EPS.30 | 30 |
| EPS.31 | 31 |
| EPS.32 | 32 |

## Requirements
- Docker-ce installed (`utils/install_docker.sh`)
- kvm/qemu, libvirt and vagrant installed (`utils/install-vagrant.sh`)
- For ML workloads, the GPUs need to be passed through to the VMs. The folder `end-host/utils` contains some scripts as starting point to update grub and Kernel modules accordingly.
  `utils/grub' might have to be updated according to your server. Alternatively, the Internet contains many tutorials.

## Usage
### Preparation
(Update data at all places marked by  `# TODO(user)`)
- The DO clockgen also acts as the orchestrator for the measurements, hence, it needs passwordless SSH access to all other physical machines.
- Update SSH keys of the DO clockgen controllers in `end-host/vagrant/base-box/vagrant-docker-vm-install.sh` and `../vagrant-ml-gpu/..`
- Clone the repository on each involved machine and update the `automation/global_config.py` to match your setup
- Replace the fingerprint of the OCS in `da-controller/src/circuit-switch.py` to match your systems
- Adapt and install grup config in `end-host/utils` accordingly.
- Adapt hostnames in `vagrant` and/or `vagrant-ml-gpu` if needed.
- Create a base data directory (e.g., by mounting a NAS) and update the path in `automation/global_config.py`
- Set the path to the project on DO clockgen as environment variable:
  ```bash
  export PROJECT=/home/user/exrec
  ```

### Running Experiments
Scripts to run the experiments of the respective sections are given in sub-folders in `automation`.
On the experiment orchestrator (DO clockgen), go to the project root directory and run 
```bash
python3 automation/<subfolder>/<script>.py
```
Data will be moved to the folder indicated in `automation/global_config.py`.

### Preprocessing and Plotting
- Prepare Python or anaconda venv
- Update `evaluation/config.py` to match your data path.
- Run all prprocessing steps:
```bash
cd evaluation
python3.8 preprocessing_scenario1_validation.py
python3.8 preprocessing_scenario2_measuring_traffic.py
python3.8 preprocessing_scenario3_ml_workload.py
```
- After aggregation of the data, it can be loaded and plotted using the prepared scripts (`evaluation/plotting-*.py`)


## Credits
The architecture of this framework is inspired by https://github.com/AmoVanB/chameleon-end-host