import path = require("path");
import dds = require("vortexdds");

main();

function main() {
  /* finds and loads the IDL file */
  const idlFileDirectory = "../../idl/"
  const idlFilename = "MotorControllerUnitModule.idl";
  LoadIDLFile(idlFileDirectory, idlFilename);

  console.log('finished');
  process.exit(0);
}

function LoadIDLFile(idlFileDirectory: string, idlFilename: string) {
  console.log("looking into directory: " + idlFileDirectory);
  console.log("looking for file: " + idlFilename);

  return dds.importIDL(idlFileDirectory + idlFilename);
}
