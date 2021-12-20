#!/bin/bash

# Up the main interface
ip link set dev eth1 up

# Sleep to make sure interface is correctly up
#sleep 3

sed -i "/header_str = .*/i \ \ \ \ \ \ \ \ log_fn(time.time())" /root/tf_benchmark/benchmarks/scripts/tf_cnn_benchmarks/benchmark_cnn.py
sed -i "/log_str = .*/i \ \ \ \ log_fn(time.time())" /root/tf_benchmark/benchmarks/scripts/tf_cnn_benchmarks/benchmark_cnn.py

sed -i "/img_secs.append.*/i \ \ \ \ \ \ \ \ log('batch_duration='+str(time))" /root/horovod_source/horovod/examples/tensorflow_synthetic_benchmark.py

# Set realtime settings
echo 0 > /proc/sys/kernel/watchdog
echo 0 > /proc/sys/kernel/nmi_watchdog
echo -1 > /proc/sys/kernel/sched_rt_period_us
echo -1 > /proc/sys/kernel/sched_rt_runtime_us
