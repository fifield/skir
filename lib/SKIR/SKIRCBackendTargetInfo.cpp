//===-- CBackendTargetInfo.cpp - CBackend Target Implementation -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SKIRCTargetMachine.h"
#include "llvm/Module.h"
#include "llvm/Target/TargetRegistry.h"
using namespace llvm;

Target llvm::TheSKIRCBackendTarget;

extern "C" void LLVMInitializeSKIRCBackendTargetInfo() { 
  RegisterTarget<> X(TheSKIRCBackendTarget, "c", "C backend");
}
