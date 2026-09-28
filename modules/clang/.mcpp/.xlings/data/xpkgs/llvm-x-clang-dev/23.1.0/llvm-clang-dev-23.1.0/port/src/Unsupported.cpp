//===- Unsupported.cpp - facilities this package leaves out --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// llvm-clang-dev port. Two upstream sources are left out because they need LLVM's DWARF reader,
// which the frontend libraries do not otherwise carry:
//
//   llvm/lib/ProfileData/InstrProfCorrelator.cpp  correlating a raw profile with debug info
//   llvm/lib/MC/MCSFrame.cpp                     emitting an .sframe section
//
// Objects that stay in (InstrProfReader, MCObjectStreamer, MCAssembler) still name a few of their
// members. Those are defined here to report, in the error channel the caller already handles,
// that the facility is not in this build -- never to pretend to have done the work.
//
//===----------------------------------------------------------------------===//

#include "llvm/MC/MCSFrame.h"
#include "llvm/ProfileData/InstrProf.h"
#include "llvm/ProfileData/InstrProfCorrelator.h"

using namespace llvm;

Expected<std::unique_ptr<InstrProfCorrelator>>
InstrProfCorrelator::get(StringRef, ProfCorrelatorKind, const object::BuildIDFetcher *,
                         const ArrayRef<object::BuildID>) {
  return make_error<InstrProfError>(instrprof_error::unable_to_correlate_profile,
                                    "profile correlation is not part of llvm-clang-dev");
}

// No correlator is ever created (get above always fails), so none is of either kind.
template <> bool InstrProfCorrelatorImpl<uint32_t>::classof(const InstrProfCorrelator *) { return false; }
template <> bool InstrProfCorrelatorImpl<uint64_t>::classof(const InstrProfCorrelator *) { return false; }

// Reached only when an object file is written with .sframe requested; this package emits no
// object files. Emitting nothing leaves the section out rather than writing a wrong one.
void MCSFrameEmitter::emit(MCObjectStreamer &) {}
void MCSFrameEmitter::encodeFuncOffset(MCContext &, uint64_t, SmallVectorImpl<char> &, MCFragment *) {}
