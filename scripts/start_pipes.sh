#!/bin/bash

ospl start

DIR=$(dirname "$0")

$DIR/from_rt_pipe & \
$DIR/to_rt_pipe
