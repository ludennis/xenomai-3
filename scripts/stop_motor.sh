#!/bin/bash

if [ ! -z `pgrep -x controller` ]; then
    kill -9 `pgrep -x controller`
fi

if [ ! -z `pgrep -x motor_monitor` ]; then
    kill -9 `pgrep -x motor_monitor`
fi

if [ ! -z `pgrep -x motor` ]; then
    kill -9 `pgrep -x motor`
fi
