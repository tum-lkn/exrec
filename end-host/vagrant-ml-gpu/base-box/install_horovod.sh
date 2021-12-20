#!/bin/bash

function install_cuda {

	# Add driver package repo
	sudo add-apt-repository ppa:graphics-drivers

	# Add NVIDIA package repos for cuda toolkit 10.0
	wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-repo-ubuntu1804_10.0.130-1_amd64.deb

	# Depackage and update cuda toolkit
	sudo apt-key adv --fetch-keys http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/7fa2af80.pub
	sudo dpkg -i cuda-repo-ubuntu1804_10.0.130-1_amd64.deb
	sudo apt-get update

	# Add NVIDIA package repos for cudnn and nccl2
	wget http://developer.download.nvidia.com/compute/machine-learning/repos/ubuntu1804/x86_64/nvidia-machine-learning-repo-ubuntu1804_1.0.0-1_amd64.deb
	sudo apt -y install ./nvidia-machine-learning-repo-ubuntu1804_1.0.0-1_amd64.deb
	sudo apt-get update

	# other cuda install
	wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-ubuntu1804.pin
	sudo mv cuda-ubuntu1804.pin /etc/apt/preferences.d/cuda-repository-pin-600
	sudo apt-key adv --fetch-keys http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/7fa2af80.pub
	sudo add-apt-repository "deb http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/ /"
	sudo apt-get update
	sudo apt-get -y install cuda

	# Install development and runtime libraries
	sudo apt-get -y install --no-install-recommends  cuda-10-0 libcudnn7=7.4.1.5-1+cuda10.0 libcudnn7-dev=7.4.1.5-1+cuda10.0  libnccl-dev=2.4.7-1+cuda10.0 libnccl2=2.4.7-1+cuda10.0
	
	# Add things to bashrc and source it
	echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/usr/local/cuda-10.0/lib64:/usr/local/cuda-10.0/extras/CUPTI/lib64' >> ~/.bashrc
	echo 'export PATH=/usr/local/cuda-10.0/bin${PATH:+:${PATH}}$' >> ~/.bashrc
	source ~/.bashrc
}

function install_open_mpi {
	# Install open mpi
	sudo apt-get -y install openmpi-bin openmpi-common openssh-client openssh-server libopenmpi-dev g++-4.8 gcc python3.6 python3-pip

}

function install_tf_gpu {
	pip3 install tensorflow-gpu==1.13.2

}

function install_tf_cpu {
	pip3 install tensorflow==1.13.2
}

function install_horovod {
	# -------------------------- C- Horovod
	# go to home and clone horovod source
	cd $HOME
	mkdir horovod_source
	cd horovod_source
	git clone --recursive https://github.com/uber/horovod.git # ---> Creates the folder horovod
	cd horovod

	# clean the folder as safe measure
	python3 setup.py clean
	
	echo "Starting to build horovod..."
	# build wheel
	HAVE_MPI=1 HAVE_NCCL=1 HOROVOD_NCCL_INCLUDE=/usr/include HOROVOD_NCCL_LIB=/usr/lib/x86_64-linux-gnu python3 setup.py bdist_wheel
	# record horovod's dist
	MY_HVD_WHL=$(ls dist/ | grep horovod)
	# install horovod built from source 
	HAVE_MPI=1 HAVE_NCCL=1 HOROVOD_NCCL_INCLUDE=/usr/include HOROVOD_NCCL_LIB=/usr/lib/x86_64-linux-gnu pip3 install --no-cache-dir dist/$MY_HVD_WHL
}


function install_tf_benchmark {
	# -------------------------- D- Running tensorflow benchmarks (v1.13)
	# clone and cd into benchmark repo
	cd $HOME
	mkdir tf_benchmark
	cd tf_benchmark
	git clone --single-branch --branch cnn_tf_v1.13_compatible https://github.com/tensorflow/benchmarks.git
	cd benchmarks

}


USE_GPU=0


# Install base stuff
install_open_mpi

install_cuda
install_tf_gpu
# install_tf_cpu

install_horovod
install_tf_benchmark

