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

    vehicleSignalTopicName = 'VehicleSignalTopic'
    tVehicleSignal = 'basic::module_vehicleSignal::vehicleSignalStruct'

    dp = DomainParticipant()
    vehicleSignalIdlClass = ddsutil.get_dds_classes_from_idl(idlFile, tVehicleSignal);

    vehicleSignalTopic = vehicleSignalIdlClass.register_topic(dp, vehicleSignalTopicName)

    pub = dp.create_publisher()

    qos = Qos([ReliabilityQosPolicy(DDSReliabilityKind.RELIABLE)])
    vehicleSignalWriter = pub.create_datawriter(vehicleSignalTopic, qos)


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
        vehicleSignalWriter.write(vehicleSignalMessage)
