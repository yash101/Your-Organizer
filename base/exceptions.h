#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>

class Code;
class Exception;

class Code
{
  friend class Exception;
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
  const Code _code;
public:
  inline Exception() :
    _msg("An Exception was thrown!"),
    _code(
      "A CyException was thrown!",
      0,
      __FILE__,
      __LINE__
    )
  {}

  inline Exception(Code code) :
    _msg(code._msg),
    _code(code)
  {}

  inline const Code getCode()
  {
    return _code;
  }

  inline virtual const char* what() const throw()
  {
    return _msg;
  }

  inline virtual ~Exception()
  {}
};

#define CODE(msg, code) (Code(msg, code, __FILE__, __LINE__))
#define EXCEPTION(msg, code) (Exception(Code(msg, code, __FILE__, __LINE__)))

#endif
