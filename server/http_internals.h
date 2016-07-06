#ifndef HTTPSUPPORT_H
#define HTTPSUPPORT_H
#include <string>
#include <exception>
namespace http
{
  class Exception;

  std::string encodeURI(std::string in);
  std::string decodeURI(std::string in);


  //Error handling. Everything's inline to make stuff faster.
  //Below here: possibly unreadable code! BEWARE!
  class Code
  {
    friend class http::Exception;
  protected:
    const char* _msg;
    const int _code;
    const char* _sourceFile;
    const unsigned long long _lineNumber;
  public:
    inline Code() :
      _msg(""),
      _code(0),
      _sourceFile(__FILE__),
      _lineNumber(__LINE__)
    {}

    inline Code(const char* msg, const int code, const char* sourceFile, const unsigned long long lineNumber) :
      _msg(msg),
      _code(code),
      _sourceFile(sourceFile),
      _lineNumber(lineNumber)
    {}

    inline virtual ~Code()
    {}

    inline const char* message()
    {
      return _msg;
    }

    inline int code()
    {
      return _code;
    }

    inline const char* source()
    {
      return _sourceFile;
    }

    inline unsigned long long line()
    {
      return _lineNumber;
    }
  };


  class Exception : public std::exception
  {
  private:
    const char* _msg;
    const http::Code _code;
    const int _httpReturnCode;

  public:
    inline Exception() :
      _msg("An error ocurred while processing your request!"),
      _code(
        "An error ocurred while processing your request!",
        500,
        __FILE__,
        __LINE__
      ),
      _httpReturnCode(500)
    {}

    inline Exception(http::Code code) :
      _msg(code._msg),
      _code(code),
      _httpReturnCode(code._code)
    {}

    inline Exception(http::Code code, int status) :
      _msg(code._msg),
      _code(code),
      _httpReturnCode(status)
    {}

    inline const http::Code getCode()
    {
      return _code;
    }

    inline int getStatus()
    {
      return _httpReturnCode;
    }

    inline virtual const char* what() const throw()
    {
      return _msg;
    }

    inline virtual ~Exception()
    {}
  };

#define HTTPEXCEPT(msg, code, status) (http::Exception(http::Code(msg, code, __FILE__, __LINE__), status))
}
#endif // HTTPSUPPORT_H
