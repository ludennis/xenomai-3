from dds import *
import ddsutil

if __name__ == '__main__':
    print('Initializing DDS and idl files ...')
    idlFile = 'carla_client_server_user.idl'

    vehicleSignalTopicName = 'VehicleSignalTopic'
    tVehicleSignal = 'basic::module_vehicleSignal::vehicleSignalStruct'

    dp = DomainParticipant()
    vehicleSignalIdlClass = ddsutil.get_dds_classes_from_idl(idlFile, tVehicleSignal);

    qos = Qos([DurabilityQosPolicy(DDSDurabilityKind.TRANSIENT),
        ReliabilityQosPolicy(DDSReliabilityKind.RELIABLE)])
    vehicleSignalTopic = vehicleSignalIdlClass.register_topic(dp, vehicleSignalTopicName, qos)

    sub = dp.create_subscriber()

    vehicleSignalReader = sub.create_datareader(vehicleSignalTopic)


    print('waiting for messages')
    while True:
        samples = vehicleSignalReader.take(1)
        for sd, si in samples:
            sd.print_vars()
