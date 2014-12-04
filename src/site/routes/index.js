var express = require('express');
var MongoClient = require('mongodb').MongoClient;
var router = express.Router();
var uuid = require('node-uuid');
var fs = require('fs');
var lazy = require("lazy"); 

var images = [];
var correctImages = {};
new lazy(fs.createReadStream('./map.txt')).lines.forEach(function(line){
  var str = line.toString();
  var obj = {};
  obj[str.split(":")[0]] = str.split(":")[1]; 
  images.push(obj);
  correctImages[str.split(":")[1]] = true;
});

/* GET home page. */
router.get('/', function(req, res) {
  // Set a unique cookie if we haven't seen this person before
  if (!req.cookies.id) {
    res.cookie('id', uuid.v4(), { maxAge: 31536000730 });
  }
  var imagePair = images[Math.floor(Math.random()*images.length)];
  var frame1, frame2;
  if (Math.floor(Math.random()*2) == 1) {
    frame1 = Object.keys(imagePair)[0];
    frame2 = imagePair[Object.keys(imagePair)[0]];
  } else {
    frame2 = Object.keys(imagePair)[0];
    frame1 = imagePair[Object.keys(imagePair)[0]];
  }
  res.render('index', { 
    title: 'Stegasis',
    frame1: frame1,
    frame2: frame2
  });
});

router.post('/submit/:img', function(req, res) {
  if (!req.cookies.id) { 
    // We haven't seen this person before...
    res.redirect("/");
  }
  console.log("img: " + req.param("img"));
  if (correctImages[req.param("img")]) {
    console.log("CORRECT!");
  }
  console.log("cookie: " + req.cookies.id);
  MongoClient.connect("mongodb://localhost:27017/stegasis", function(err, db) {
    if(!err) {
      console.log("We are connected");
      res.redirect("/");
    } else {
      // Database problem, silently redirect user
      console.log("Database connection issue...");
      res.redirect("/");
    }
  }); 
});

module.exports = router;
