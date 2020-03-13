'use strict';

const dds = require('vortexdds');
const path = require('path');

main();

function main(){
  sendMotorMessage('hi').then(() => {
    console.log('finished');
    process.exit(0);
  }).catch((error) => {
    console.log('Error: ' + error.message);
    process.exit(1);
  });
}

async function sendMotorMessage(message){
  console.log('Sending following message to motor: ' + message);

  // create dds stuff
  let participant = null;
  try {
    participant = new dds.Participant();

    // find idl file and import
    const idlName = 'MotorControllerUnitModule.idl';
    const idlPath = __dirname + '/../idl/' + idlName;
    console.log('idlPath: ' + idlPath);

    const typeSupports = await dds.importIDL(idlPath);
    const typeSupport = typeSupports.get('MotorControllerUnitModule::MotorMessage');

    const topicName = 'motor_topic';
    const topic = participant.createTopic(
      topicName,
      typeSupport
    );

    const publisherQos = dds.QoS.publisherDefault();
    publisherQos.partition = {name: 'controller'};
    const publisher = participant.createPublisher(publisherQos);

    const writerQos = new dds.QoS({
      reliability: {kind: dds.ReliabilityKind.BestEffor}});
    const writer = publisher.createWriter(topic, writerQos);

    const subscriberQos = dds.QoS.subscriberDefault();
    subscriberQos.partition = {name: 'motor'};
    const subscriber = participant.createSubscriber(subscriberQos);

    const reader = subscriber.createReader(topic);

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
