var express = require('express');
var MongoClient = require('mongodb').MongoClient;
var router = express.Router();
var uuid = require('node-uuid');

/* GET home page. */
router.get('/', function(req, res) {
  // Set a unique cookie if we haven't seen this person before
  if (!req.cookies.id) {
    res.cookie('id', uuid.v4(), { maxAge: 31536000730 });
  }
  res.render('index', { 
    title: 'Stegasis',
    frame1: "frame",
    frame2: "frame"
  });
});

router.post('/submit/:img', function(req, res) {
  if (!req.cookies.id) { 
    // We haven't seen this person before...
    res.redirect("/");
  }
  console.log("img: " + req.param("img"));
  console.log("cookie: " + req.cookies.id);
  MongoClient.connect("mongodb://localhost:27017/stegasis", function(err, db) {
    if(!err) {
      console.log("We are connected");
      res.redirect("/");
    }
  }); 
});

module.exports = router;
