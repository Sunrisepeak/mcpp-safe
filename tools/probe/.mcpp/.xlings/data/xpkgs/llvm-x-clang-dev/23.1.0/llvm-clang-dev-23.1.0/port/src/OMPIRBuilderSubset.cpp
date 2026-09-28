//===- OMPIRBuilderSubset.cpp - the part of OMPIRBuilder the frontend uses ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// llvm-clang-dev port: llvm/lib/Frontend/OpenMP/OMPIRBuilder.cpp is left out of this package
// (it needs LLVM's optimizer, which the frontend libraries do not), but clang's ASTContext calls
// one static member of it. That member is reproduced here verbatim from upstream
// llvmorg-23.1.0, OMPIRBuilder.cpp lines 7272-7287.
//
//===----------------------------------------------------------------------===//

#include "llvm/Frontend/OpenMP/OMPIRBuilder.h"

using namespace llvm;

unsigned
OpenMPIRBuilder::getOpenMPDefaultSimdAlign(const Triple &TargetTriple,
                                           const StringMap<bool> &Features) {
  if (TargetTriple.isX86()) {
    if (Features.lookup("avx512f"))
      return 512;
    else if (Features.lookup("avx"))
      return 256;
    return 128;
  }
  if (TargetTriple.isPPC())
    return 128;
  if (TargetTriple.isWasm())
    return 128;
  return 0;
}
