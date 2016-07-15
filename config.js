(function config() {
  var conf = conf || {};
  conf.dbHost = "localhost";
  conf.dbPort = 5432;
  conf.dbName = "your-organizer";
  conf.dbUser = "your-organizer";
  conf.dbPass = "password";

  module.exports = conf;
})();
