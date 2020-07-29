#!/bin/bash

taskset -c 5,6,7 ../bin/motor --session=motor_control_session & \
taskset -c 8 ../bin/motor_monitor --session=motor_control_session & \
taskset -c 5,6 ../bin/controller --session=motor_control_session
