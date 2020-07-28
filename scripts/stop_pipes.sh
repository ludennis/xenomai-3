#!/bin/bash

if [[ $(pgrep -x to_rt_pipe | wc -c) -gt 0 ]];
then
    kill -9 `pgrep -x to_rt_pipe`
fi

if [[ $(pgrep -x from_rt_pipe | wc -c) -gt 0 ]]
then
    kill -9 `pgrep -x from_rt_pipe`
fi

pipe_pids=`lsof -w +D /dev | grep rtp | awk '{ print $2}'`

for pipe_pid in $pipe_pids
do
  kill -9 $pipe_pid
done

