import path = require("path");
import dds = require("vortexdds");

class DDSBridge {
  participant: typeof dds.Participant;
  publisher: typeof dds.Publisher;
  subscriber: typeof dds.Subscriber;
  writer: typeof dds.Writer;
  reader: typeof dds.Reader;
  waitset: typeof dds.Waitset;
  guard: typeof dds.GuardCondition;

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

  async FindMatchedPublication() {
    let publicationMatchedCondition = null;
    publicationMatchedCondition = this.writer.createStatusCondition();
    publicationMatchedCondition.enable(dds.StatusMask.publication_matched);

    this.waitset = new dds.Waitset(publicationMatchedCondition);
    this.guard = new dds.GuardCondition();
    this.waitset.attach(this.guard);

    console.log('Finding matching publication topic for writer ...');
    await this.waitset.wait(dds.SEC_TO_NANO(10));
    // TODO: to add ctrl + c guard condition to waitset
    console.log('Found matching publication');
  }

  SendMessage(message: any) {
    this.writer.writeReliable(message);
  }

  async WaitForReply() {
    let newDataCondition = null;

    try {
      newDataCondition = this.reader.createReadCondition(
        dds.StateMask.sample.not_read
      );

      this.waitset = new dds.Waitset(this.reader.createReadCondition(
        dds.StateMask.sample.not_read));

      await this.waitset.wait(10);
      let takeArray = this.reader.take(1);
      if(takeArray.length > 0 && takeArray[0].info.valid_data) {
        let samples = takeArray[0].sample;
        console.log("received: " + JSON.stringify(samples));
        return samples;
      }
    } finally {
      if(this.waitset !== null) {
        this.waitset.delete();
      }
    }
  }

  DeleteDomainParticipant() {
    if(this.participant !== null) {
      this.participant.delete().catch((error: any) => {
        console.log('Error cleaning up Domain Participant: ' + error.message);
      });
    }
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

  /* Check Matching Publication with Waitset */
  await ddsBridge.FindMatchedPublication();

  /* Send Message */
  let message = {content: "Hello"};
  ddsBridge.SendMessage(message);

  /* Wait for replaying message */
  let samples = await ddsBridge.WaitForReply();

  /* Cleanup Resources */
  ddsBridge.DeleteDomainParticipant();
}
