#!/bin/bash

taskset -c 5,6,7 ./motor --session=motor_control_session & \
taskset -c 8 ./motor_monitor --session=motor_control_session & \
taskset -c 5,6 ./controller --session=motor_control_session
