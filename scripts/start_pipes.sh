#!/bin/bash

ospl start

DIR=$(dirname "$0")

$DIR/init_rtp.sh
$DIR/from_rt_pipe & \
$DIR/to_rt_pipe
