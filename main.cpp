#include "mathvm.h"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>

#include <exception>
#include <iostream>
#include <string>

using namespace mathvm;
using namespace std;

Translator* Translator::create(const string& impl) {
  if (impl == "bytecode_translator") {
    return new BytecodeTranslatorImpl();
  } 

  return NULL;
}

int main(int argc, char** argv) {
  string program;

  for (int i = 0; i < argc; ++i) {
    string arg = argv[i];

    if (arg == "-e" && i + 1 < argc) {
        program = argv[++i];
        continue;
    }

    program = loadFile(arg.c_str());
  }

  if (program.empty()) { 
    cerr << "Could not load program\n"
    << "Usage:\n"
    << "mvm PATH_TO_SOURCE\n"
    << "mvm -e SCRIPT" << endl; 
    return EXIT_FAILURE;
  }    

  Translator* translator = Translator::create("bytecode_translator");

  if (translator == NULL) { 
    cerr << "Define translator impl at factory" << endl;
    return EXIT_FAILURE;
  }

  Code* code = NULL;
  Status* translateStatus = translator->translate(program, &code);

  if (translateStatus->isError()) {
    uint32_t position = translateStatus->getPosition();
    uint32_t line = 0, offset = 0;
    positionToLineOffset(program, position, line, offset);
    cerr << "Cannot translate program: "
         << "at line: "  << line 
         << ", offset: " << offset 
         << ", error: "  << translateStatus->getError().c_str() 
         << endl;
    return EXIT_FAILURE;
  }

  if (code != NULL) {
    delete code;
  }

  delete translator;
  delete translateStatus;

  return 0;
}