//===-- CTargetMachine.h - TargetMachine for the C backend ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the TargetMachine that is used by the C backend.
//
//===----------------------------------------------------------------------===//

#ifndef SKIRCTARGETMACHINE_H
#define SKIRCTARGETMACHINE_H

#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetData.h"

namespace llvm {

struct SKIRCTargetMachine : public TargetMachine {
  SKIRCTargetMachine(const Target &T, const std::string &TT, const std::string &FS)
    : TargetMachine(T) {}

  virtual bool WantsWholeFile() const { return false; }
  virtual bool addPassesToEmitWholeFile(PassManager &PM,
                                        formatted_raw_ostream &Out,
                                        CodeGenFileType FileType,
                                        CodeGenOpt::Level OptLevel,
                                        bool DisableVerify) {}
  
  virtual const TargetData *getTargetData() const { return 0; }
};

extern Target TheSKIRCBackendTarget;

} // End llvm namespace


#endif
