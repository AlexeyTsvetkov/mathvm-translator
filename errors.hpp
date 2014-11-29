#ifndef ERRORS_HPP
#define ERRORS_HPP

#include "ast.h"

#include <exception>

namespace mathvm {

class TranslationException : public std::exception {
  AstNode* node_;
  const char* message_;

public:
  TranslationException(const char* message, AstNode* node)
    : node_(node),
      message_(message) {}

  virtual const char* what() const throw() {
    return message_;
  }

  AstNode* at() const {
    return node_;
  }
};

} // namespace mathvm

#endif