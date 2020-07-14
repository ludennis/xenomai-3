#!/usr/bin/python3

import time
import readchar
import sys

from dds import *
import ddsutil

dp = DomainParticipant()

def WaitForThrottleInputRoutine(threadName):
    print('Thread {}: starting', threadName)
    print('{} => Initializing DDS and idl files ...', threadName)
    idlFile = 'carla_client_server_user.idl'

    vehicleSignalTopicName = 'VehicleSignalTopic'
    tVehicleSignal = 'basic::module_vehicleSignal::vehicleSignalStruct'

    vehicleSignalIdlClass = ddsutil.get_dds_classes_from_idl(idlFile, tVehicleSignal);

    vehicleSignalTopic = vehicleSignalIdlClass.register_topic(dp, vehicleSignalTopicName)

    pub = dp.create_publisher()

    qos = Qos([ReliabilityQosPolicy(DDSReliabilityKind.RELIABLE)])
    vehicleSignalWriter = pub.create_datawriter(vehicleSignalTopic, qos)

    print('Press w to increase throttle, s to decrease throttle, q to quit')

    throttle = 0
    messageId = 1;
    vehicleSpeed = 0.0;
    while True:
        print('Throttle = {}'.format(throttle))
        keyboardInput = readchar.readkey()
        print("keyboardInput: {}".format(keyboardInput))
        if keyboardInput == 'w':
            print("increasing throttle")
            if throttle < 100:
                throttle += 1
            vehicleSpeed += (throttle * 0.6)
        elif keyboardInput == 'q':
            sys.exit()
        else:
            if throttle > 0:
                throttle -= 1
            if vehicleSpeed - 2 < 0:
                vehicleSpeed = 0
            elif vehicleSpeed > 0:
                vehicleSpeed -= 2

        # sends dds message vehicleSignalStruct (vehicle_speed)
        vehicleSignalMessage = vehicleSignalIdlClass.topic_data_class(
            id=messageId, vehicle_speed=vehicleSpeed)
        vehicleSignalMessage.id = messageId
        vehicleSignalMessage.throttle = throttle
        vehicleSignalMessage.vehicle_speed = vehicleSpeed
        vehicleSignalMessage.simulation_time = str(int(time.monotonic() * 1e9))
        vehicleSignalWriter.write(vehicleSignalMessage)

def WaitForVehicleSignalStructRoutine(threadName):
    print('Thread {}: starting ...', threadName)
    print('{} => Initializing DDS and idl files ...', threadName)
    idlFile = 'carla_client_server_user.idl'

    vehicleSignalTopicName = 'VehicleSignalTopic'
    tVehicleSignal = 'basic::module_vehicleSignal::vehicleSignalStruct'

    vehicleSignalIdlClass = ddsutil.get_dds_classes_from_idl(idlFile, tVehicleSignal);

    vehicleSignalTopic = vehicleSignalIdlClass.register_topic(dp, vehicleSignalTopicName)

    # dds sub
    sub = dp.create_subscriber()
    qos = Qos([ReliabilityQosPolicy(DDSReliabilityKind.RELIABLE)])
    reader = sub.create_datareader(vehicleSignalTopic, qos)

    # read by waiting with infinite timeout
#    waitset = WaitSet()
#    dataAvailableCondition = QueryCondition(reader, DDSMaskUtil.new_samples())
#    waitset.attach(dataAvailableCondition)

    print('{} => Waiting for Motor Vehicle Signal messages', threadName)

    while True:
        samples = reader.take(1)
        for sd, si in samples:
            sd.print_vars()


if __name__ == '__main__':
    waitForThrottleInputThread = threading.Thread(
        target=WaitForThrottleInputRoutine, args=("WaitForThrottleInputThread",))
    waitForThrottleInputThread.start()

    waitForVehicleSignalStructThread = threading.Thread(
        target=WaitForVehicleSignalStructRoutine, args=("WaitForVehicleSignalStructThread",))
    waitForVehicleSignalStructThread.start()

    # wait for termination
    waitForThrottleInputThread.join()
