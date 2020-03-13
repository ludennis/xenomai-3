'use strict';

const dds = require('vortexdds');
const path = require('path');

main();

function main(){
  sendControlMessage('Hi,from nodejs').then(() => {
    console.log('finished');
    process.exit(0);
  }).catch((error) => {
    console.log(error.stack);
    process.exit(1);
  });
}

async function sendControlMessage(message){
  console.log('Sending following message to motor: ' + message);

  // create dds stuff
  let participant = null;
  try {
    participant = new dds.Participant();

    // find idl file and import
    const idlName = 'MotorControllerUnitModule.idl';
    const idlPath = __dirname + '/../idl/' + idlName;
    console.log('idlPath: ' + idlPath);

    const messageTypes = await dds.importIDL(idlPath);
    const controlMessageType = messageTypes.get('MotorControllerUnitModule::ControlMessage');
    const motorMessageType = messageTypes.get('MotorControllerUnitModule::MotorMessage');

    const controlTopic = participant.createTopic(
      'control_topic',
      controlMessageType
    );

    const motorTopic = participant.createTopic(
      'motor_topic',
      motorMessageType
    );


    const publisherQos = dds.QoS.publisherDefault();
    publisherQos.partition = {names: 'nodejsPartition'};
    const publisher = participant.createPublisher(publisherQos);

    const writerQos = new dds.QoS({
      reliability: {kind: dds.ReliabilityKind.Reliable}});
    const writer = publisher.createWriter(controlTopic, writerQos);

    const subscriberQos = dds.QoS.subscriberDefault();
    subscriberQos.partition = {names: 'motorPartition'};
    const subscriber = participant.createSubscriber(subscriberQos);

    const reader = subscriber.createReader(motorTopic);

    let publisherMatchedCondition = null;
    let publisherMatchedWaitset = null;
    try {
      publisherMatchedCondition = writer.createStatusCondition();
      publisherMatchedCondition.enable(dds.StatusMask.publication_matched);
      publisherMatchedWaitset = new dds.Waitset(publisherMatchedCondition);

      console.log('Waiting for motor message subscriber/topic to be found ...');
      await publisherMatchedWaitset.wait(dds.SEC_TO_NANO(10));
      console.log('Found motor subscriber/topic');
    } finally {
      if (publisherMatchedWaitset !== null){
        publisherMatchedWaitset.delete();
      }
    }

    // after finding the subscriber and topic, send message
    let msg = {content: message};
    console.log('sending: ' + JSON.stringify(msg));
    writer.writeReliable(msg);

    // wait for data, and terminates if received
    let newDataCondition = null;
    let newDataWaitset = null;

    try {
      // create waitset for new data
      newDataCondition = reader.createReadCondition(
        dds.StateMask.sample.not_read
      );
      newDataWaitset = new dds.Waitset(newDataCondition);

      await newDataWaitset.wait(10);
      let takeArray = reader.take(1);
      if (takeArray.length > 0 && takeArray[0].info.valid_data) {
        let sample = takeArray[0].sample;
        console.log('received: ' + JSON.stringify(sample));
      }
    } finally {
      if (newDataWaitset !== null) {
        newDataWaitset.delete();
      }
    }
  } finally {
    console.log('Terminating. Cleaning up Domain Participant.');
    if(participant !== null){
      participant.delete().catch((error) => {
        console.log('Error cleaning up Domain Participant: ' +
          error.message);
      });
    }
  }

  // send a message to controller
  // listen to reply from controller
  process.exit(0);
}
