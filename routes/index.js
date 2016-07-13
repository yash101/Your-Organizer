var express = require('express');
var router = express.Router();

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
  res.render('index');
});

router.post('/auth/register', function(req, res, next) {
});

module.exports = router;
