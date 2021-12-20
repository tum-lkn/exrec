#!/bin/bash

echo -1 > /proc/sys/kernel/sched_rt_period_us
echo -1 > /proc/sys/kernel/sched_rt_runtime_us
echo 10 > /proc/sys/vm/stat_interval
echo 0 > /proc/sys/kernel/watchdog_thresh
