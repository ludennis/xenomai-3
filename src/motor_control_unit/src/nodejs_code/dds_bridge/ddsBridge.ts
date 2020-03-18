import path = require("path");
import dds = require("vortexdds");

main();

function main() {
  /* finds and loads the IDL file */
  const idlFileDirectory = "/../../../idl/"
  const idlFilename = "MotorControllerUnitModule.idl";
  const idlPath = __dirname + idlFileDirectory + idlFilename;

  const messageTypes = LoadIDLFile(idlPath);


}

async function LoadIDLFile(idlPath: string) {
  console.log("looking file at path: " + idlPath);

  try {
    const result = await dds.importIDL(idlPath);
    console.log(await result);
  }
  catch (err) {
    console.log('dds.importIDL failed', err);
  }
}
