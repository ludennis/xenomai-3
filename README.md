# Overview
A Xenomai linux RTOS (Real-Time Operating System) to be deployed on any linux machine for RT HiL (Real-Time Hardware in the Loop). The kernel would run a given generated C code of a motor model designed and ported from Simulink while being able to bridge external communication to PXI I/O cards, NI interface cards, and PEAK CAN. Also to handle non RT internal communication through OpenSplice DDS (Data Distribution Service).

# Installation
1. Make a build directory where the CMakeLists.txt is at
```shell
mkdir build
cd build
```
2. Specify the modules to be installed with CMake. The following command would also install NI, Pickering, Xenomai, and not install PCAN, DDS.
```shell
cmake .. -DNI=ON -DPICKERING=ON -DXENOMAI=ON -DPCAN=OFF -DDDS=OFF
```
3. Once commands run successfully, run mkae with the generated makefiles
```shell
make
```
4. Install binaries
```shell
sudo make install
```
5. Check installed files in /usr/local
```shell
ls /usr/local
```
6. Running the binaries
```shell
/usr/local/rt-hil-simulation/bin/start_motor.sh
/usr/local/rt-hil-simulation/bin/start_pipes.sh
/usr/local/rt-hil-simulation/scripts/throttle_emulator.py
```
7. Killing running proceses
```shell
ctrl + c
/usr/local/rt-hil-simulation/bin/stop_pipes.sh
/usr/local/rt-hil-simulation/bin/stop_motor.sh
```
