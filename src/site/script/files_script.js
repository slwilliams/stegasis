var fs = require('fs');
var uuid = require('node-uuid');

var cleanDir = process.argv[2];
var dirtyDir = process.argv[3];

var cleanFiles = fs.readdirSync(cleanDir);
var dirtyFiles = fs.readdirSync(dirtyDir);

var map = {};

for (var i = 0; i < cleanFiles.length; i ++) {
 var cleanNew = uuid.v4() + ".jpeg"; 
 var dirtyNew = uuid.v4() + ".jpeg";
 map[cleanNew] = dirtyNew;
 fs.renameSync(cleanDir + "/" + cleanFiles[i], cleanDir + "/" + cleanNew);
 fs.renameSync(dirtyDir + "/" + dirtyFiles[i], dirtyDir + "/" + dirtyNew);
}

var outputFile = process.argv[4];

for (var k in map) {
  fs.appendFileSync(outputFile, k+":"+map[k]+"\n"); 
}

console.log("Finished...");
