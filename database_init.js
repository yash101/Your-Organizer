(function dbInit() {
  module.exports = function dbInitialize(pool)
  {
    pool.query('CREATE TABLE IF NOT EXISTS users (' +
      'uid bigserial,' +
      'username varchar(32),' +
      'email varchar(128),'
      'password varchar(128),' +
      'fname varchar(128),' +
      'lname varchar(128)' +
      ')'
    );
  };
})();
