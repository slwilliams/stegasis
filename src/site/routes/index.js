var express = require('express');
var MongoClient = require('mongodb').MongoClient;
var router = express.Router();
var uuid = require('node-uuid');
var fs = require('fs');
var lazy = require('lazy'); 

var images = [];
var correctImages = {};
new lazy(fs.createReadStream('./map.txt')).lines.forEach(function(line){
  var str = line.toString();
  var obj = {};
  obj[str.split(':')[0]] = str.split(':')[1]; 
  images.push(obj);
  correctImages[str.split(':')[1]] = true;
});

/* GET home page. */
router.get('/', function(req, res) {
  // Set a unique cookie if we haven't seen this person before
  if (!req.cookies.id) {
    res.cookie('id', uuid.v4(), { maxAge: 31536000730 });
  }
  // Randomly select an image pair
  var imagePair = images[Math.floor(Math.random()*images.length)];
  var frame1, frame2;
  // Randomly swap the images left / right
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


function updateTotal(db, correct) {
  db.collection('total', function(err, collection) {
    collection.findOne({}, function(err, item) {
      if (correct) {
        item.correct ++;
        item.string += '1';
      }
      else {
        item.incorrect ++;
        item.string += '0';
      }
      item.total ++;
      collection.update({}, item, function(err, cnt, stat) {
        if (!err) { 
          console.log('Updated total');
        }
      });
    });
  });
}

router.post('/submit/:img', function(req, res) {
  if (!req.cookies.id) { 
    // We haven't seen this person before...
    res.redirect('/');
  }
  console.log('img: ' + req.param('img'));
  console.log('cookie: ' + req.cookies.id);
  MongoClient.connect('mongodb://localhost:27017/stegasis', function(err, db) {
    if (!err) {
      db.collection('raw', function(err, collection) {
        collection.findOne({user: req.cookies.id}, function(err, item) {
          if (!item) {
            var newObj;
            if (correctImages[req.param('img')]) {
              updateTotal(db, true);
              newObj = {user: req.cookies.id, total: 1, correct: 1, incorrect: 0, string: '1'};
            } else {
              updateTotal(db, false);
              newObj = {user: req.cookies.id, total: 1, correct: 0, incorrect: 1, string: '0'};
            }
            collection.insert(newObj, function(err, rec) {
              if (!err) {
                console.log('Added new user.');
              }
              res.redirect('/');
            });
          } else {
            var total = item.total + 1;
            var correct = item.correct;
            var str = item.string;
            var incorrect = item.incorrect;
            if (correctImages[req.param('img')]) {
              updateTotal(db, true);
              correct ++;
              str += '1';
            } else {
              updateTotal(db, false);
              incorrect ++;
              str += '0';
            }
            var newObj = {user: req.cookies.id, total: total, correct: correct, incorrect: incorrect, string: str};
            collection.update({user: req.cookies.id}, newObj, function(err, cn, stat) {
              if (!err) {
                console.log('Updated user');
              }
              res.redirect('/');
            });
          }
        });
      });
    } else {
      // Database problem, silently redirect user
      console.log('Database connection issue...');
      res.redirect('/');
    }
  }); 
});

module.exports = router;
