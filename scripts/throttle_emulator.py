#!/usr/bin/python3

import time
import readchar
import sys

from dds import *
import ddsutil

dp = DomainParticipant()
isFinished = False


def WaitForThrottleInputRoutine(threadName):
    global isFinished;
    print('Thread {}: starting'.format(threadName))
    print('{} => Initializing DDS and idl files ...'.format(threadName))
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
            # setting isFinished = true will signal WaitForVehicleSignalStructTask
            # to return
            print('{} => Exiting'.format(threadName))
            isFinished = True
            return
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
        messageId += 1


def WaitForVehicleSignalStructRoutine(threadName):
    print('Thread {}: starting ...'.format(threadName))
    print('{} => Initializing DDS and idl files ...'.format(threadName))
    idlFile = 'carla_client_server_user.idl'

    vehicleSignalTopicName = 'VehicleSignalTopic'
    tVehicleSignal = 'basic::module_vehicleSignal::vehicleSignalStruct'

    vehicleSignalIdlClass = ddsutil.get_dds_classes_from_idl(idlFile, tVehicleSignal);

    vehicleSignalTopic = vehicleSignalIdlClass.register_topic(dp, vehicleSignalTopicName)

    sub = dp.create_subscriber()
    qos = Qos([ReliabilityQosPolicy(DDSReliabilityKind.RELIABLE)])
    reader = sub.create_datareader(vehicleSignalTopic, qos)

    print('{} => Waiting for Motor Vehicle Signal messages'.format(threadName))
    while True:
        if isFinished == True:
            print('{} => Exiting'.format(threadName))
            return

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

    waitForThrottleInputThread.join()
    print("waitForThrottleInputThread stopped")
    waitForVehicleSignalStructThread.join()
    print("waitForVehicleSignalStructThread stopped")
