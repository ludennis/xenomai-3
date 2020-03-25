import path = require("path");
import dds = require("vortexdds");

interface SampleObject {
  content: string
}

interface RequestMessageObject {
  request: string
}

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

  SendMessage(message: RequestMessageObject) {
    this.writer.writeReliable(message);
    console.log("Message sent: " + JSON.stringify(message));
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
        let sample = takeArray[0].sample;
        console.log("received: " + JSON.stringify(sample));
        return sample;
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
  sendNodejsRequestToController().then(() => {
    console.log("successfully sent message to controller");
  }).catch((error: any) => {
    console.log("error sending message to controller");
  });
}

async function sendNodejsRequestToController() {

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
  ddsBridge.CreatePublisher('NodejsPartition');

  /* Subscriber */
  ddsBridge.CreateSubscriber('NodejsPartition');

  /* DataWriter */
  ddsBridge.CreateDataWriter(
    ddsBridge.CreateTopic(
      'nodejs_request_topic',
      messageTypes.get('MotorControllerUnitModule::NodejsRequestMessage')
    )
  );

  /* DataReader */
  ddsBridge.CreateDataReader(
    ddsBridge.CreateTopic(
      'motor_output_topic',
      messageTypes.get('MotorControllerUnitModule::MotorOutputMessage')
    )
  );

  /* Check Matching Publication with Waitset */
  await ddsBridge.FindMatchedPublication();

  /* Send Message */
  let requestMessage = {request: "RequestMsgMotorOutput"};
  ddsBridge.SendMessage(requestMessage);

  /* Wait for replaying message */
  let sample = await ddsBridge.WaitForReply();

  /* Cleanup Resources */
  ddsBridge.DeleteDomainParticipant();
}
