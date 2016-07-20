var express = require('express');
var router = express.Router();

var uver = require('../internals/userverif.js');
var ureg = require('../internals/userregist.js');

router.get('/', function(req, res, next) {
  res.render('index', {});
});

router.get('/login', function(req, res, next) {
  res.render('auth/login', { title: "Log In" });
});

router.get('/register', function(req, res, next) {
  res.render('auth/register', { title: "Register" });
});

router.get('/auth/register', function(req, res, next) {
  res.render('index', {});
});

router.post('/auth/register', function(req, res, next) {
  console.log(req.body);
  var username = req.body.username;
  var password = req.body.password;
  var cpassword = req.body.cpasswd;
  var email = req.body.email;

  ureg.addUser(req.dbPool, username, password, cpassword, email, function(success, error) {
    if(success) {
      res.send("1: " + error);
    } else {
      res.send("0: " + error);
    }

    return;
  });
});

module.exports = router;
