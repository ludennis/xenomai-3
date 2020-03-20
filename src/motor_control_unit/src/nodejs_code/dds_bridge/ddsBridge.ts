import path = require("path");
import dds = require("vortexdds");

class DDSBridge {
  participant: typeof dds.Participant;
  publisher: typeof dds.Publisher;
  subscriber: typeof dds.Subscriber;
  writer: typeof dds.Writer;
  reader: typeof dds.Reader;

  // TODO: make partitionName an Array of strings

  constructor() {
  }

  async LoadIDLFile(idlPath: string) {
    console.log("looking file at path: " + idlPath);
    try {
      return await dds.importIDL(idlPath);
    }
    catch (err) {
      console.log('dds.importIDL failed', err);
    }
  }


  CreateDomainParticipant() {
    this.participant = new dds.Participant();
  }

  CreatePublisher(partitionName: string) {
    const publisherQos = dds.QoS.publisherDefault();
    publisherQos.partition = {names: partitionName};
    this.publisher = this.participant.createPublisher(publisherQos);
  }

  CreateSubscriber(partitionName: string) {
    const subscriberQos = dds.QoS.subscriberDefault();
    subscriberQos.partition = {names: partitionName};
    this.subscriber = this.participant.createSubscriber(subscriberQos);
  }

  CreateTopic(topicName: string, messageType: typeof dds.TypeSupport): typeof dds.Topic {
    return this.participant.createTopic(topicName, messageType);
  }

  CreateDataWriter(topic: typeof dds.Topic) {
    const writerQos = new dds.QoS({
      reliability: {kind: dds.ReliabilityKind.Reliable}});
    this.writer = this.publisher.createWriter(topic, writerQos);
  }

  CreateDataReader(topic: typeof dds.Topic) {
    const readerQos = new dds.QoS({
      reliability: {kind: dds.ReliabilityKind.Reliable}});
    this.reader = this.subscriber.createReader(topic, readerQos);
  }
}

main();

function main() {
  sendNodejsRequestToMotor().then(() => {
    console.log("successfully sent message to motor");
  }).catch((error: any) => {
    console.log("error sending message to motor");
  });
}

async function sendNodejsRequestToMotor() {

  /* DDS */
  let ddsBridge = new DDSBridge();

  /* IDL file */
  const idlFileDirectory = "/../../idl/"
  const idlFilename = "MotorControllerUnitModule.idl";
  const idlPath = __dirname + idlFileDirectory + idlFilename;

  let messageTypes = await ddsBridge.LoadIDLFile(idlPath);

  /* DomainParticipant */
  ddsBridge.CreateDomainParticipant();

  /* Publisher */
  ddsBridge.CreatePublisher('NodejsRequestPartition');

  /* Subscriber */
  ddsBridge.CreateSubscriber('MotorResponsePartition');

  /* DataWriter */
  ddsBridge.CreateDataWriter(
    ddsBridge.CreateTopic(
      'control_topic',
      messageTypes.get('MotorControllerUnitModule::ControlMessage')
    )
  );

  /* DataReader */
  ddsBridge.CreateDataReader(
    ddsBridge.CreateTopic(
      'motor_topic',
      messageTypes.get('MotorControllerUnitModule::MotorMessage')
    )
  );

  /* WaitSet */

  process.exit(0);
}
