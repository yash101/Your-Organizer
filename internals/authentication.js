(function authenticationjs() {
  var crypto = require('crypto');
  var ret = ret || {};

  ret.generateSessionKey = function generateRandomSessionKey() {
    return crypto.createHash("sha256").update(Math.random().toString() + new Date().toISOString()).digest("hex");
  };

  ret.loginUser = function loginUser(dbPool, req, res, username, password) {
    if(!(pool && username && password) && typeof(username) != "string" && typeof(password) != "string") {
    }
  };

  module.exports = ret;
})();
