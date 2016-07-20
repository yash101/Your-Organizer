(function userverifjs() {
  var ret = ret || {};

  var checkBeginEnd = function checkBeginEnd(string, chr) {
    if(string.charAt(0) == chr || string.charAt(string.length - 1) == chr)
      return false;
    return true;
  };

  var checkBeginEndEach = function checkBeginEndEach(string, chars) {
    for(var i = 0; i < chars.length; i++) {
      if(!checkBeginEnd(string, chars[i]))
        return false;
    }
    return true;
  };

  var notInDictionary = function notInDictionary(str, dict) {
    var good = true;
    for(var i = 0; i < str.length; i++) {
      var good = false;
      for(var j = 0; j < dict.length; j++) {
        if(str[i] == dict[j]) {
          good = true;
          break;
        }
      }
      if(good == false) return false;
    }
    return true;
  };

  ret.checkEmail = function checkEmail(addr) {
    if(addr == null || addr == undefined) return false;
    if(addr.length > 128 || addr.length < 6) return false;

    var allowedchars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@!#$%&*+-=_|~.,:";
    if(!notInDictionary(addr, allowedchars)) return false;

    if(addr.split('@').length - 1 != 1) return false;
    if(addr.split('..').length - 1 > 0) return false;
    if(addr.charAt(0) == '.' || addr.charAt(addr.indexOf('@')) == '.') return false;
    if(addr.charAt(0) == '@' || addr.charAt(addr.length - 1) == '@') return false;

    return true;
  };

  ret.checkUsername = function checkUsername(username) {
    if(username.length > 32 || username.length < 3) return false;
    var allowedChars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-.\"*^&#$%!~";
    if(!notInDictionary(username, allowedChars)) return false;

    if(!checkBeginEndEach(username, "_-\"*^&#$%!~")) return false;
    console.log("DictSearchGood");

    return true;
  };

  ret.checkPassword = function checkPassword(password) {
    //Just an optimization. If password's too long/short, skip the regex!
    if(password.length < 5 || password.length > 32) return false;
/*
    var regex = /^(?=.*[0-9])[a-zA-Z0-9!@#$%^&*~`]{5,32}/;
    return regex.test(password); */
    return true;
  };

  module.exports = ret;
})();
