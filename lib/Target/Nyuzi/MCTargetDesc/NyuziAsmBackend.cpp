//===-- NyuziASMBackend.cpp - Nyuzi Asm Backend  -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the NyuziAsmBackend class.
//
//===----------------------------------------------------------------------===//
//

#include "MCTargetDesc/NyuziMCTargetDesc.h"
#include "NyuziFixupKinds.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
class NyuziAsmBackend : public MCAsmBackend {
  Triple::OSType OSType;

public:
  NyuziAsmBackend(const Target &T, Triple::OSType _OSType)
      : MCAsmBackend(), OSType(_OSType) {}

  MCObjectWriter *createObjectWriter(raw_pwrite_stream &OS) const override {
    return createNyuziELFObjectWriter(
        OS, MCELFObjectTargetWriter::getOSABI(OSType));
  }

  void applyFixup(const MCFixup &Fixup, char *Data, unsigned DataSize,
                  uint64_t Value, bool IsPCRel) const override {
    MCFixupKind Kind = Fixup.getKind();
    Value = adjustFixupValue((unsigned)Kind, Value);
    unsigned Offset = Fixup.getOffset();
    unsigned NumBytes = 4;

    uint64_t CurVal = 0;
    for (unsigned i = 0; i != NumBytes; ++i) {
      CurVal |= (uint64_t)((uint8_t)Data[Offset + i]) << (i * 8);
    }

    uint64_t Mask =
        ((uint64_t)(-1) >> (64 - getFixupKindInfo(Kind).TargetSize));

    Value <<= getFixupKindInfo(Kind).TargetOffset;
    Mask <<= getFixupKindInfo(Kind).TargetOffset;
    CurVal |= Value & Mask;

    // Write out the fixed up bytes back to the code/data bits.
    for (unsigned i = 0; i != NumBytes; ++i) {
      Data[Offset + i] = (uint8_t)((CurVal >> (i * 8)) & 0xff);
    }
  }

  const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const override {
    const static MCFixupKindInfo Infos[Nyuzi::NumTargetFixupKinds] = {
        // This table *must* be in same the order of fixup_* kinds in
        // NyuziFixupKinds.h.
        //
        // name                          offset  bits  flags
        {"fixup_Nyuzi_PCRel_MemAccExt", 10, 15, MCFixupKindInfo::FKF_IsPCRel},
        {"fixup_Nyuzi_PCRel_MemAcc", 15, 10, MCFixupKindInfo::FKF_IsPCRel},
        {"fixup_Nyuzi_PCRel_Branch", 5, 20, MCFixupKindInfo::FKF_IsPCRel},
        {"fixup_Nyuzi_PCRel_ComputeLabelAddress", 10, 13,
         MCFixupKindInfo::FKF_IsPCRel}};

    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);

    assert(unsigned(Kind - FirstTargetFixupKind) < getNumFixupKinds() &&
           "Invalid kind!");
    return Infos[Kind - FirstTargetFixupKind];
  }

  bool mayNeedRelaxation(const MCInst &Inst) const override { return false; }

  /// fixupNeedsRelaxation - Target specific predicate for whether a given
  /// fixup requires the associated instruction to be relaxed.
  bool fixupNeedsRelaxation(const MCFixup &Fixup, uint64_t Value,
                            const MCRelaxableFragment *DF,
                            const MCAsmLayout &Layout) const override {
    return false;
  }

  void relaxInstruction(const MCInst &Inst, MCInst &Res) const override {
    assert(0 && "relaxInstruction() unimplemented");
  }

  // This gets called to align. If strings are emitted in the text area,
  // this may not be a multiple of the instruction size. Just fill with
  // zeroes.
  bool writeNopData(uint64_t Count, MCObjectWriter *OW) const override {
    OW->WriteZeros(Count);
    return true;
  }

private:
  unsigned getNumFixupKinds() const { return Nyuzi::NumTargetFixupKinds; }

  static unsigned adjustFixupValue(unsigned Kind, uint64_t Value) {
    switch (Kind) {
    case Nyuzi::fixup_Nyuzi_PCRel_MemAccExt:
    case Nyuzi::fixup_Nyuzi_PCRel_MemAcc:
    case Nyuzi::fixup_Nyuzi_PCRel_Branch:
    case Nyuzi::fixup_Nyuzi_PCRel_ComputeLabelAddress:
      Value -= 4; // source location is PC + 4
      break;
    }

    return Value;
  }
}; // class NyuziAsmBackend

} // namespace

// MCAsmBackend
MCAsmBackend *llvm::createNyuziAsmBackend(const Target &T,
                                          const MCRegisterInfo &MRI,
                                          const Triple &TT, StringRef CPU) {
  return new NyuziAsmBackend(T, Triple(TT).getOS());
}
