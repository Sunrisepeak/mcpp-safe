# port

Code this package adds to upstream, as openkal-musl's `port/` does. Upstream sources under
`../llvm/` are never edited; what they need and this build leaves out is supplied here.

| File | What | Why |
|---|---|---|
| `src/OMPIRBuilderSubset.cpp` | `OpenMPIRBuilder::getOpenMPDefaultSimdAlign`, verbatim from upstream | `OMPIRBuilder.cpp` needs LLVM's optimizer; clang's `ASTContext` calls only this static member |
| `src/Unsupported.cpp` | `InstrProfCorrelator::get` (fails with `unable_to_correlate_profile`), `InstrProfCorrelatorImpl<T>::classof` (false), `MCSFrameEmitter::emit`/`encodeFuncOffset` (no .sframe) | `InstrProfCorrelator.cpp` and `MCSFrame.cpp` need LLVM's DWARF reader; the frontend never correlates profiles or writes object files |
