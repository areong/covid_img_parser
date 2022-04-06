function writeJsonFile(object, filename) {
  var fs = require('fs');
  fs.writeFileSync(filename, JSON.stringify(object), function(err) {
    if (err) {
      console.log(err);
    }
  });
}

async function detectText(imageFilename) {
  const vision = require('@google-cloud/vision');
  
  // Create a client.
  const client = new vision.ImageAnnotatorClient();

  // Detect text.
  console.log("Detecting text...");
  const [result] = await client.documentTextDetection(imageFilename);
  // console.log("Result:");
  // console.log(result);
  return result;
}

// Check input.
if (process.argv.length != 3) {
  throw 'Invalid arguments!\nCorrect arguments: <imageFilename>';
}
const inputFilename = process.argv[2];
const outputFilename = inputFilename.substring(0, inputFilename.lastIndexOf('.') + 1) + 'json';

console.log('Input filename: ' + inputFilename);
console.log('Output filename: ' + outputFilename);

detectText(inputFilename)
  .then(detections => {
    // console.log(detections);
    writeJsonFile(detections, outputFilename);
    console.log("Detection results are written to the output file.");
  })
  .catch(err => {
    console.log
  });
