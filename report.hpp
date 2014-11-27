#ifndef REPORT_HPP
#define REPORT_HPP 

#include "mathvm.h"

#include <cassert>
#include <cstdlib>

namespace mathvm {

class Report {
  Status* status_;
public:
  Report(): status_(Status::Ok()) {}
  
  ~Report() {
    if (status_ != NULL) {
      delete status_;
    }
  }

  Status* release() {
    assert(status_ != NULL);
    Status* status = status_;
    status_ = NULL;
    return status;
  }

  bool isError() {
    return status_->isError();
  }

  void error(const char* msg) {
    setStatus(Status::Error(msg));
  }

  void error(const char* msg, AstNode* node) {
    setStatus(Status::Error(msg, node->position()));
  }
private:
  void setStatus(Status* status) {
    if (!status_->isOk()) {
      delete status;
      return;
    }

    delete status_;
    status_ = status;
  } 
};

}

#endif