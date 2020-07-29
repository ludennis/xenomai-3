#!/bin/bash

DIR=$(dirname "$0")

taskset -c 5,6,7 $DIR/motor --session=motor_control_session & \
taskset -c 8 $DIR/motor_monitor --session=motor_control_session & \
taskset -c 5,6 $DIR/controller --session=motor_control_session
