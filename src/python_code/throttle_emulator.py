#!/usr/bin/python3

import time
import readchar
import sys

from dds import *
import ddsutil

if __name__ == '__main__':
    throttle = 0

    print('Initializing DDS and idl files ...')
    idlFile = 'carla_client_server_user.idl'
    driverSignalTopicName = 'DriverSignalTopic'
    vehicleSignalTopicName = 'VehicleSignalTopic'
    tDriverSignal = 'basic::module_vehicleSignal::vehicleSignalStruct'
    tVehicleSignal = 'basic::module_driverSignal::driverSignalStruct'

    dp = DomainParticipant()
    driverSignalIdlClass = ddsutil.get_dds_classes_from_idl(idlFile, tDriverSignal);
    vehicleSignalIdlClass = ddsutil.get_dds_classes_from_idl(idlFile, tVehicleSignal);

    qos = Qos([DurabilityQosPolicy(DDSDurabilityKind.TRANSIENT),
        ReliabilityQosPolicy(DDSReliabilityKind.RELIABLE)])
    driverSignalTopic = driverSignalIdlClass.register_topic(dp, driverSignalTopicName, qos)
    vehicleSignalTopic = vehicleSignalIdlClass.register_topic(dp, vehicleSignalTopicName, qos)

    pub = dp.create_publisher()

    driverSignalWriter = pub.create_datawriter(driverSignalTopic)
    vehicleSignalWriter = pub.create_datawriter(vehicleSignalTopic)


    print('Press w to increase throttle, s to decrease throttle, q to quit')

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
            # send dds message driverSignalStruct (throttle)
            driverSignalMessage = driverSignalIdlClass.topic_data_class(
                id=messageId, throttle=throttle)
            driverSignalWriter.write(driverSignalMessage)

            # sends dds message vehicleSignalStruct (vehicle_speed)
            vehicleSignalMessage = vehicleSignalIdlClass.topic_data_class(
                id=messageId, vehicle_speed=vehicleSpeed)
            vehicleSignalWriter.write(vehicleSignalMessage)
        elif keyboardInput == 'q':
            sys.exit()
        else:
            if throttle > 0:
                throttle -= 1
            if vehicleSpeed > 0:
                vehicleSpeed -= 2
