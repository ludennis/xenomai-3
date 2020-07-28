#!/bin/bash

kill -9 `pgrep -x controller`
kill -9 `pgrep -x motor_monitor`
kill -9 `pgrep -x motor`
