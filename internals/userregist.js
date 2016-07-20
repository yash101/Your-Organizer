(function userregistjs() {
  var uv = require("./userverif.js");
  var ret = ret || {};

  ret.addUser = function addUser(pool, username, password, cpassword, email, callback) {
    if(!(pool && username && password && cpassword && email)) {
      return "Undefined parameters!";
      callback(false, "Undefined Parameters!");
    }
    if(!(typeof(username) == "string" && typeof(password) == "string" && typeof(cpassword) == typeof(password) && typeof(email) == "string")) {
      return "Invalid parameters!";
      callback(false, "Invalid parameters!");
    }


    if(password != cpassword) callback(false, "Passwords don't match!");
    if(!uv.checkEmail(email)) callback(false, "Email invalid!");
    if(!uv.checkPassword(password)) callback(false, "Password invalid! Must be 5-32 in length, with 1+ numbers and 1+ special symbols");
    if(!uv.checkUsername(username)) callback(false, "Username invalid!");

    email = email.toLowerCase();
    username = username.toLowerCase();

    //Connect
    pool.connect(function conn(err, client, gc) {
      if(err) {
        gc();
        callback(false, "Could not establish database connection!");
        return;
      }

      //Check if username exists
      client.query("SELECT * FROM users WHERE username=$1 OR email=$1",
        [username], function cu(err, result)
      {
        if(err) {
          gc();
          callback(false, "Error: " + err);
          return;
        }

        if(result && result.rows.length != 0) {
          gc();
          callback(false, "User or email address already in use!");
          return;
        }

        client.query("INSERT INTO users (username, password, email) VALUES ($1, $2, $3)",
          [username, password, email], function(err, result)
        {
          if(err) {
            gc();
            callback(false, "Error: " + err);
            return;
          }

          gc();
          callback(true, "User registration successful!");
          return;
        });

      });
    });

    return true;
  };

  module.exports = ret;
})();
