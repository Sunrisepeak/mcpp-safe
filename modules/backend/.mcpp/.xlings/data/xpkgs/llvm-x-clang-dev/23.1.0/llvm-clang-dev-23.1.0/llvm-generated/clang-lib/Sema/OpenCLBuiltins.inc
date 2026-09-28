/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* OpenCL Builtin handling                                                    *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|* From: OpenCLBuiltins.td                                                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "llvm/ADT/StringRef.h"
using namespace clang;

enum OpenCLTypeID {
  OCLT_atomic_double,
  OCLT_atomic_flag,
  OCLT_atomic_float,
  OCLT_atomic_half,
  OCLT_atomic_int,
  OCLT_atomic_intptr_t,
  OCLT_atomic_long,
  OCLT_atomic_ptrdiff_t,
  OCLT_atomic_size_t,
  OCLT_atomic_uint,
  OCLT_atomic_uintptr_t,
  OCLT_atomic_ulong,
  OCLT_bool,
  OCLT_char,
  OCLT_clk_event_t,
  OCLT_clk_profiling_info,
  OCLT_double,
  OCLT_event_t,
  OCLT_float,
  OCLT_half,
  OCLT___half,
  OCLT_image1d_t,
  OCLT_image1d_array_t,
  OCLT_image1d_buffer_t,
  OCLT_image2d_t,
  OCLT_image2d_array_t,
  OCLT_image2d_array_depth_t,
  OCLT_image2d_array_msaa_t,
  OCLT_image2d_array_msaa_depth_t,
  OCLT_image2d_depth_t,
  OCLT_image2d_msaa_t,
  OCLT_image2d_msaa_depth_t,
  OCLT_image3d_t,
  OCLT_int,
  OCLT_intptr_t,
  OCLT_long,
  OCLT_cl_mem_fence_flags,
  OCLT_memory_order,
  OCLT_memory_scope,
  OCLT_ndrange_t,
  OCLT_ptrdiff_t,
  OCLT_queue_t,
  OCLT_reserve_id_t,
  OCLT_sampler_t,
  OCLT_short,
  OCLT_size_t,
  OCLT_uchar,
  OCLT_uint,
  OCLT_uintptr_t,
  OCLT_ulong,
  OCLT_ushort,
  OCLT_void,
  OCLT_AGenType1,
  OCLT_AGenTypeN,
  OCLT_AGenTypeNNoScalar,
  OCLT_AI2UGenTypeN,
  OCLT_AIGenType1,
  OCLT_AIGenTypeN,
  OCLT_AIGenTypeNNoScalar,
  OCLT_CharShortGenType1,
  OCLT_FGenTypeN,
  OCLT_GenTypeCharVecAndScalar,
  OCLT_GenTypeCharVecAndScalarTo8,
  OCLT_GenTypeCharVecNoScalar,
  OCLT_GenTypeDoubleVec1234,
  OCLT_GenTypeDoubleVecAndScalar,
  OCLT_GenTypeDoubleVecNoScalar,
  OCLT_GenTypeFloatVec1234,
  OCLT_GenTypeFloatVecAndScalar,
  OCLT_GenTypeFloatVecNoScalar,
  OCLT_GenTypeHalfVec1234,
  OCLT_GenTypeHalfVecAndScalar,
  OCLT_GenTypeHalfVecNoScalar,
  OCLT_GenTypeIntVecAndScalar,
  OCLT_GenTypeIntVecNoScalar,
  OCLT_GenTypeLongVecAndScalar,
  OCLT_GenTypeLongVecNoScalar,
  OCLT_GenTypeShortVecAndScalar,
  OCLT_GenTypeShortVecAndScalarTo8,
  OCLT_GenTypeShortVecNoScalar,
  OCLT_GenTypeUCharVecAndScalar,
  OCLT_GenTypeUCharVecAndScalarTo8,
  OCLT_GenTypeUCharVecNoScalar,
  OCLT_GenTypeUIntVecAndScalar,
  OCLT_GenTypeUIntVecNoScalar,
  OCLT_GenTypeULongVecAndScalar,
  OCLT_GenTypeULongVecNoScalar,
  OCLT_GenTypeUShortVecAndScalar,
  OCLT_GenTypeUShortVecAndScalarTo8,
  OCLT_GenTypeUShortVecNoScalar,
  OCLT_IntLongFloatGenType1,
  OCLT_SGenTypeN,
  OCLT_UGenTypeN,
};

// Image access qualifier.
enum OpenCLAccessQual : unsigned char {
  OCLAQ_None,
  OCLAQ_ReadOnly,
  OCLAQ_WriteOnly,
  OCLAQ_ReadWrite
};

// Represents a return type or argument type.
struct OpenCLTypeStruct {
  // A type (e.g. float, int, ...).
  const OpenCLTypeID ID;
  // Vector size (if applicable; 0 for scalars and generic types).
  const unsigned VectorWidth;
  // 0 if the type is not a pointer.
  const bool IsPointer : 1;
  // 0 if the type is not const.
  const bool IsConst : 1;
  // 0 if the type is not volatile.
  const bool IsVolatile : 1;
  // Access qualifier.
  const OpenCLAccessQual AccessQualifier;
  // Address space of the pointer (if applicable).
  const LangAS AS;
};

// One overload of an OpenCL builtin function.
struct OpenCLBuiltinStruct {
  // Index of the signature in the OpenCLTypeStruct table.
  const unsigned SigTableIndex;
  // Entries between index SigTableIndex and (SigTableIndex + NumTypes - 1) in
  // the SignatureTable represent the complete signature.  The first type at
  // index SigTableIndex is the return type.
  const unsigned NumTypes;
  // Function attribute __attribute__((pure))
  const bool IsPure : 1;
  // Function attribute __attribute__((const))
  const bool IsConst : 1;
  // Function attribute __attribute__((convergent))
  const bool IsConv : 1;
  // OpenCL extension(s) required for this overload.
  const unsigned short Extension;
  // OpenCL versions in which this overload is available.
  const unsigned short Versions;
};

static const char *FunctionExtensionTable[] = {
  // 0: AMDMediaOps
  "cl_amd_media_ops",
  // 1: AMDMediaOps2
  "cl_amd_media_ops2",
  // 2: ArmIntegerDotProductAccumulateInt8
  "cl_arm_integer_dot_product_accumulate_int8",
  // 3: ArmIntegerDotProductAccumulateInt16
  "cl_arm_integer_dot_product_accumulate_int16",
  // 4: ArmIntegerDotProductAccumulateSaturateInt8
  "cl_arm_integer_dot_product_accumulate_saturate_int8",
  // 5: ArmIntegerDotProductInt8
  "cl_arm_integer_dot_product_int8",
  // 6: FuncExtFloatAtomicsFp16GenericASAdd
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_add __opencl_c_ext_fp16_global_atomic_add",
  // 7: FuncExtFloatAtomicsFp16GenericASLoadStore
  "cl_ext_float_atomics __opencl_c_ext_fp16_global_atomic_load_store __opencl_c_ext_fp16_local_atomic_load_store",
  // 8: FuncExtFloatAtomicsFp16GenericASMinMax
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_min_max __opencl_c_ext_fp16_global_atomic_min_max",
  // 9: FuncExtFloatAtomicsFp16GlobalASAdd
  "cl_ext_float_atomics __opencl_c_ext_fp16_global_atomic_add",
  // 10: FuncExtFloatAtomicsFp16GlobalASLoadStore
  "cl_ext_float_atomics __opencl_c_ext_fp16_global_atomic_load_store",
  // 11: FuncExtFloatAtomicsFp16GlobalASMinMax
  "cl_ext_float_atomics __opencl_c_ext_fp16_global_atomic_min_max",
  // 12: FuncExtFloatAtomicsFp16LocalASAdd
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_add",
  // 13: FuncExtFloatAtomicsFp16LocalASLoadStore
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_load_store",
  // 14: FuncExtFloatAtomicsFp16LocalASMinMax
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_min_max",
  // 15: FuncExtFloatAtomicsFp32GenericASAdd
  "cl_ext_float_atomics __opencl_c_ext_fp32_local_atomic_add __opencl_c_ext_fp32_global_atomic_add",
  // 16: FuncExtFloatAtomicsFp32GenericASMinMax
  "cl_ext_float_atomics __opencl_c_ext_fp32_local_atomic_min_max __opencl_c_ext_fp32_global_atomic_min_max",
  // 17: FuncExtFloatAtomicsFp32GlobalASAdd
  "cl_ext_float_atomics __opencl_c_ext_fp32_global_atomic_add",
  // 18: FuncExtFloatAtomicsFp32GlobalASMinMax
  "cl_ext_float_atomics __opencl_c_ext_fp32_global_atomic_min_max",
  // 19: FuncExtFloatAtomicsFp32LocalASAdd
  "cl_ext_float_atomics __opencl_c_ext_fp32_local_atomic_add",
  // 20: FuncExtFloatAtomicsFp32LocalASMinMax
  "cl_ext_float_atomics __opencl_c_ext_fp32_local_atomic_min_max",
  // 21: FuncExtFloatAtomicsFp64GenericASAdd
  "cl_ext_float_atomics __opencl_c_ext_fp64_local_atomic_add __opencl_c_ext_fp64_global_atomic_add",
  // 22: FuncExtFloatAtomicsFp64GenericASMinMax
  "cl_ext_float_atomics __opencl_c_ext_fp64_local_atomic_min_max __opencl_c_ext_fp64_global_atomic_min_max",
  // 23: FuncExtFloatAtomicsFp64GlobalASAdd
  "cl_ext_float_atomics __opencl_c_ext_fp64_global_atomic_add",
  // 24: FuncExtFloatAtomicsFp64GlobalASMinMax
  "cl_ext_float_atomics __opencl_c_ext_fp64_global_atomic_min_max",
  // 25: FuncExtFloatAtomicsFp64LocalASAdd
  "cl_ext_float_atomics __opencl_c_ext_fp64_local_atomic_add",
  // 26: FuncExtFloatAtomicsFp64LocalASMinMax
  "cl_ext_float_atomics __opencl_c_ext_fp64_local_atomic_min_max",
  // 27: FuncExtIntelBfloat16Conversions
  "cl_intel_bfloat16_conversions",
  // 28: FuncExtIntelSplitWorkGroupBarrier
  "cl_intel_split_work_group_barrier",
  // 29: FuncExtIntelSubgroupBufferPrefetch
  "cl_intel_subgroup_buffer_prefetch",
  // 30: FuncExtIntelSubgroupLocalBlockIO
  "cl_intel_subgroup_local_block_io",
  // 31: FuncExtIntelSubgroups
  "cl_intel_subgroups",
  // 32: FuncExtIntelSubgroupsChar
  "cl_intel_subgroups_char",
  // 33: FuncExtIntelSubgroupsCharLocalBlockIO
  "cl_intel_subgroups_char cl_intel_subgroup_local_block_io",
  // 34: FuncExtIntelSubgroupsCharPrefetch
  "cl_intel_subgroups_char cl_intel_subgroup_buffer_prefetch",
  // 35: FuncExtIntelSubgroupsCharRWImages
  "cl_intel_subgroups_char __opencl_c_read_write_images",
  // 36: FuncExtIntelSubgroupsLong
  "cl_intel_subgroups_long",
  // 37: FuncExtIntelSubgroupsLongLocalBlockIO
  "cl_intel_subgroups_long cl_intel_subgroup_local_block_io",
  // 38: FuncExtIntelSubgroupsLongPrefetch
  "cl_intel_subgroups_long cl_intel_subgroup_buffer_prefetch",
  // 39: FuncExtIntelSubgroupsLongRWImages
  "cl_intel_subgroups_long __opencl_c_read_write_images",
  // 40: FuncExtIntelSubgroupsPrefetch
  "cl_intel_subgroups cl_intel_subgroup_buffer_prefetch",
  // 41: FuncExtIntelSubgroupsRWImages
  "cl_intel_subgroups __opencl_c_read_write_images",
  // 42: FuncExtIntelSubgroupsShort
  "cl_intel_subgroups_short",
  // 43: FuncExtIntelSubgroupsShortLocalBlockIO
  "cl_intel_subgroups_short cl_intel_subgroup_local_block_io",
  // 44: FuncExtIntelSubgroupsShortPrefetch
  "cl_intel_subgroups_short cl_intel_subgroup_buffer_prefetch",
  // 45: FuncExtIntelSubgroupsShortRWImages
  "cl_intel_subgroups_short __opencl_c_read_write_images",
  // 46: FuncExtKhrExtendedBitOps
  "cl_khr_extended_bit_ops",
  // 47: FuncExtKhrGlMsaaSharing
  "cl_khr_gl_msaa_sharing",
  // 48: FuncExtKhrGlobalInt32BaseAtomics
  "cl_khr_global_int32_base_atomics",
  // 49: FuncExtKhrGlobalInt32ExtendedAtomics
  "cl_khr_global_int32_extended_atomics",
  // 50: FuncExtKhrInt64BaseAtomics
  "cl_khr_int64_base_atomics",
  // 51: FuncExtKhrInt64ExtendedAtomics
  "cl_khr_int64_extended_atomics",
  // 52: FuncExtKhrLocalInt32BaseAtomics
  "cl_khr_local_int32_base_atomics",
  // 53: FuncExtKhrLocalInt32ExtendedAtomics
  "cl_khr_local_int32_extended_atomics",
  // 54: FuncExtKhrMipmapImage
  "cl_khr_mipmap_image",
  // 55: FuncExtKhrMipmapImageWrites
  "cl_khr_mipmap_image_writes",
  // 56: FuncExtKhrSubgroupBallot
  "cl_khr_subgroup_ballot",
  // 57: FuncExtKhrSubgroupClusteredReduce
  "cl_khr_subgroup_clustered_reduce",
  // 58: FuncExtKhrSubgroupExtendedTypes
  "cl_khr_subgroup_extended_types",
  // 59: FuncExtKhrSubgroupNonUniformArithmetic
  "cl_khr_subgroup_non_uniform_arithmetic",
  // 60: FuncExtKhrSubgroupNonUniformVote
  "cl_khr_subgroup_non_uniform_vote",
  // 61: FuncExtKhrSubgroupShuffle
  "cl_khr_subgroup_shuffle",
  // 62: FuncExtKhrSubgroupShuffleRelative
  "cl_khr_subgroup_shuffle_relative",
  // 63: FuncExtKhrSubgroups
  "__opencl_subgroup_builtins",
  // 64: FuncExtNone
  "",
  // 65: FuncExtOpenCLCDeviceEnqueue
  "__opencl_c_device_enqueue",
  // 66: FuncExtOpenCLCGenericAddressSpace
  "__opencl_c_generic_address_space",
  // 67: FuncExtOpenCLCNamedAddressSpaceBuiltins
  "__opencl_c_named_address_space_builtins",
  // 68: FuncExtOpenCLCPipes
  "__opencl_c_pipes",
  // 69: FuncExtOpenCLCReadWriteImages
  "__opencl_c_read_write_images",
  // 70: FuncExtOpenCLCWGCollectiveFunctions
  "__opencl_c_work_group_collective_functions",
  // 71: FuncExtOpenCLCxx
  "__cplusplus",
  // 72: anonymous_8064
  "__opencl_c_generic_address_space __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 73: anonymous_8066
  "__opencl_c_generic_address_space __opencl_c_atomic_scope_device",
  // 74: anonymous_8262
  "__opencl_c_named_address_space_builtins __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 75: anonymous_8264
  "__opencl_c_named_address_space_builtins __opencl_c_atomic_scope_device",
  // 76: anonymous_8821
  "cl_ext_float_atomics __opencl_c_ext_fp16_global_atomic_load_store __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 77: anonymous_8824
  "cl_ext_float_atomics __opencl_c_ext_fp16_global_atomic_load_store __opencl_c_atomic_scope_device",
  // 78: anonymous_8834
  "cl_ext_float_atomics __opencl_c_ext_fp16_global_atomic_add __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 79: anonymous_8837
  "cl_ext_float_atomics __opencl_c_ext_fp16_global_atomic_add __opencl_c_atomic_scope_device",
  // 80: anonymous_8841
  "cl_ext_float_atomics __opencl_c_ext_fp32_global_atomic_add __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 81: anonymous_8844
  "cl_ext_float_atomics __opencl_c_ext_fp32_global_atomic_add __opencl_c_atomic_scope_device",
  // 82: anonymous_8848
  "cl_ext_float_atomics __opencl_c_ext_fp64_global_atomic_add __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 83: anonymous_8851
  "cl_ext_float_atomics __opencl_c_ext_fp64_global_atomic_add __opencl_c_atomic_scope_device",
  // 84: anonymous_8864
  "cl_ext_float_atomics __opencl_c_ext_fp16_global_atomic_min_max __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 85: anonymous_8867
  "cl_ext_float_atomics __opencl_c_ext_fp16_global_atomic_min_max __opencl_c_atomic_scope_device",
  // 86: anonymous_8871
  "cl_ext_float_atomics __opencl_c_ext_fp32_global_atomic_min_max __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 87: anonymous_8874
  "cl_ext_float_atomics __opencl_c_ext_fp32_global_atomic_min_max __opencl_c_atomic_scope_device",
  // 88: anonymous_8878
  "cl_ext_float_atomics __opencl_c_ext_fp64_global_atomic_min_max __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 89: anonymous_8881
  "cl_ext_float_atomics __opencl_c_ext_fp64_global_atomic_min_max __opencl_c_atomic_scope_device",
  // 90: anonymous_8895
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_load_store __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 91: anonymous_8898
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_load_store __opencl_c_atomic_scope_device",
  // 92: anonymous_8908
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_add __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 93: anonymous_8911
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_add __opencl_c_atomic_scope_device",
  // 94: anonymous_8915
  "cl_ext_float_atomics __opencl_c_ext_fp32_local_atomic_add __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 95: anonymous_8918
  "cl_ext_float_atomics __opencl_c_ext_fp32_local_atomic_add __opencl_c_atomic_scope_device",
  // 96: anonymous_8922
  "cl_ext_float_atomics __opencl_c_ext_fp64_local_atomic_add __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 97: anonymous_8925
  "cl_ext_float_atomics __opencl_c_ext_fp64_local_atomic_add __opencl_c_atomic_scope_device",
  // 98: anonymous_8938
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_min_max __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 99: anonymous_8941
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_min_max __opencl_c_atomic_scope_device",
  // 100: anonymous_8945
  "cl_ext_float_atomics __opencl_c_ext_fp32_local_atomic_min_max __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 101: anonymous_8948
  "cl_ext_float_atomics __opencl_c_ext_fp32_local_atomic_min_max __opencl_c_atomic_scope_device",
  // 102: anonymous_8952
  "cl_ext_float_atomics __opencl_c_ext_fp64_local_atomic_min_max __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 103: anonymous_8955
  "cl_ext_float_atomics __opencl_c_ext_fp64_local_atomic_min_max __opencl_c_atomic_scope_device",
  // 104: anonymous_8969
  "cl_ext_float_atomics __opencl_c_ext_fp16_global_atomic_load_store __opencl_c_ext_fp16_local_atomic_load_store __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 105: anonymous_8972
  "cl_ext_float_atomics __opencl_c_ext_fp16_global_atomic_load_store __opencl_c_ext_fp16_local_atomic_load_store __opencl_c_atomic_scope_device",
  // 106: anonymous_8982
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_add __opencl_c_ext_fp16_global_atomic_add __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 107: anonymous_8985
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_add __opencl_c_ext_fp16_global_atomic_add __opencl_c_atomic_scope_device",
  // 108: anonymous_8989
  "cl_ext_float_atomics __opencl_c_ext_fp32_local_atomic_add __opencl_c_ext_fp32_global_atomic_add __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 109: anonymous_8992
  "cl_ext_float_atomics __opencl_c_ext_fp32_local_atomic_add __opencl_c_ext_fp32_global_atomic_add __opencl_c_atomic_scope_device",
  // 110: anonymous_8996
  "cl_ext_float_atomics __opencl_c_ext_fp64_local_atomic_add __opencl_c_ext_fp64_global_atomic_add __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 111: anonymous_8999
  "cl_ext_float_atomics __opencl_c_ext_fp64_local_atomic_add __opencl_c_ext_fp64_global_atomic_add __opencl_c_atomic_scope_device",
  // 112: anonymous_9012
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_min_max __opencl_c_ext_fp16_global_atomic_min_max __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 113: anonymous_9015
  "cl_ext_float_atomics __opencl_c_ext_fp16_local_atomic_min_max __opencl_c_ext_fp16_global_atomic_min_max __opencl_c_atomic_scope_device",
  // 114: anonymous_9019
  "cl_ext_float_atomics __opencl_c_ext_fp32_local_atomic_min_max __opencl_c_ext_fp32_global_atomic_min_max __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 115: anonymous_9022
  "cl_ext_float_atomics __opencl_c_ext_fp32_local_atomic_min_max __opencl_c_ext_fp32_global_atomic_min_max __opencl_c_atomic_scope_device",
  // 116: anonymous_9026
  "cl_ext_float_atomics __opencl_c_ext_fp64_local_atomic_min_max __opencl_c_ext_fp64_global_atomic_min_max __opencl_c_atomic_order_seq_cst __opencl_c_atomic_scope_device",
  // 117: anonymous_9029
  "cl_ext_float_atomics __opencl_c_ext_fp64_local_atomic_min_max __opencl_c_ext_fp64_global_atomic_min_max __opencl_c_atomic_scope_device",
  // 118: anonymous_10101
  "__opencl_c_integer_dot_product_input_4x8bit",
  // 119: anonymous_10110
  "__opencl_c_integer_dot_product_input_4x8bit_packed",
  // 120: anonymous_10119
  "cl_khr_subgroup_rotate",
  // 121: anonymous_10122
  "cl_khr_kernel_clock __opencl_c_kernel_clock_scope_device",
  // 122: anonymous_10125
  "cl_khr_kernel_clock __opencl_c_kernel_clock_scope_work_group",
  // 123: anonymous_10128
  "cl_khr_kernel_clock __opencl_c_kernel_clock_scope_sub_group",
};

static const OpenCLTypeStruct TypeTable[] = {
  // 0
  {OCLT_AGenType1, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 1
  {OCLT_AGenTypeN, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 2
  {OCLT_AGenTypeNNoScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 3
  {OCLT_AI2UGenTypeN, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 4
  {OCLT_AIGenType1, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 5
  {OCLT_AIGenTypeN, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 6
  {OCLT_AIGenTypeNNoScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 7
  {OCLT_atomic_double, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 8
  {OCLT_atomic_flag, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 9
  {OCLT_atomic_float, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 10
  {OCLT_atomic_half, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 11
  {OCLT_atomic_int, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 12
  {OCLT_atomic_intptr_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 13
  {OCLT_atomic_long, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 14
  {OCLT_atomic_ptrdiff_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 15
  {OCLT_atomic_size_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 16
  {OCLT_atomic_uint, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 17
  {OCLT_atomic_uintptr_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 18
  {OCLT_atomic_ulong, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 19
  {OCLT_bool, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 20
  {OCLT_char, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 21
  {OCLT_CharShortGenType1, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 22
  {OCLT_clk_event_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 23
  {OCLT_clk_profiling_info, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 24
  {OCLT_double, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 25
  {OCLT_event_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 26
  {OCLT_FGenTypeN, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 27
  {OCLT_float, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 28
  {OCLT_GenTypeCharVecAndScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 29
  {OCLT_GenTypeCharVecAndScalarTo8, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 30
  {OCLT_GenTypeCharVecNoScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 31
  {OCLT_GenTypeDoubleVec1234, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 32
  {OCLT_GenTypeDoubleVecAndScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 33
  {OCLT_GenTypeDoubleVecNoScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 34
  {OCLT_GenTypeFloatVec1234, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 35
  {OCLT_GenTypeFloatVecAndScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 36
  {OCLT_GenTypeFloatVecNoScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 37
  {OCLT_GenTypeHalfVec1234, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 38
  {OCLT_GenTypeHalfVecAndScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 39
  {OCLT_GenTypeHalfVecNoScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 40
  {OCLT_GenTypeIntVecAndScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 41
  {OCLT_GenTypeIntVecNoScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 42
  {OCLT_GenTypeLongVecAndScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 43
  {OCLT_GenTypeLongVecNoScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 44
  {OCLT_GenTypeShortVecAndScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 45
  {OCLT_GenTypeShortVecAndScalarTo8, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 46
  {OCLT_GenTypeShortVecNoScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 47
  {OCLT_GenTypeUCharVecAndScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 48
  {OCLT_GenTypeUCharVecAndScalarTo8, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 49
  {OCLT_GenTypeUCharVecNoScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 50
  {OCLT_GenTypeUIntVecAndScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 51
  {OCLT_GenTypeUIntVecNoScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 52
  {OCLT_GenTypeULongVecAndScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 53
  {OCLT_GenTypeULongVecNoScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 54
  {OCLT_GenTypeUShortVecAndScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 55
  {OCLT_GenTypeUShortVecAndScalarTo8, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 56
  {OCLT_GenTypeUShortVecNoScalar, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 57
  {OCLT_half, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 58
  {OCLT___half, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_constant},
  // 59
  {OCLT___half, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 60
  {OCLT___half, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 61
  {OCLT___half, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 62
  {OCLT___half, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 63
  {OCLT___half, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_constant},
  // 64
  {OCLT___half, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 65
  {OCLT___half, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 66
  {OCLT___half, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 67
  {OCLT___half, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 68
  {OCLT_image1d_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 69
  {OCLT_image1d_array_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 70
  {OCLT_image1d_buffer_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 71
  {OCLT_image2d_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 72
  {OCLT_image2d_array_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 73
  {OCLT_image2d_array_depth_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 74
  {OCLT_image2d_array_msaa_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 75
  {OCLT_image2d_array_msaa_depth_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 76
  {OCLT_image2d_depth_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 77
  {OCLT_image2d_msaa_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 78
  {OCLT_image2d_msaa_depth_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 79
  {OCLT_image3d_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 80
  {OCLT_int, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 81
  {OCLT_IntLongFloatGenType1, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 82
  {OCLT_intptr_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 83
  {OCLT_long, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 84
  {OCLT_cl_mem_fence_flags, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 85
  {OCLT_memory_order, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 86
  {OCLT_memory_scope, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 87
  {OCLT_ndrange_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 88
  {OCLT_ptrdiff_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 89
  {OCLT_queue_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 90
  {OCLT_reserve_id_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 91
  {OCLT_SGenTypeN, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 92
  {OCLT_sampler_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 93
  {OCLT_short, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 94
  {OCLT_size_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 95
  {OCLT_uchar, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 96
  {OCLT_UGenTypeN, 0, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 97
  {OCLT_uint, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 98
  {OCLT_uintptr_t, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 99
  {OCLT_ulong, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 100
  {OCLT_ushort, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 101
  {OCLT_void, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 102
  {OCLT___half, 1, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 103
  {OCLT___half, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 104
  {OCLT_float, 2, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 105
  {OCLT_float, 3, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 106
  {OCLT_float, 4, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 107
  {OCLT_float, 8, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 108
  {OCLT_float, 16, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 109
  {OCLT_double, 2, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 110
  {OCLT_double, 3, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 111
  {OCLT_double, 4, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 112
  {OCLT_double, 8, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 113
  {OCLT_double, 16, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 114
  {OCLT_half, 2, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 115
  {OCLT_half, 3, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 116
  {OCLT_half, 4, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 117
  {OCLT_half, 8, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 118
  {OCLT_half, 16, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 119
  {OCLT_char, 2, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 120
  {OCLT_char, 3, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 121
  {OCLT_char, 4, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 122
  {OCLT_char, 8, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 123
  {OCLT_char, 16, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 124
  {OCLT_uchar, 2, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 125
  {OCLT_uchar, 3, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 126
  {OCLT_uchar, 4, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 127
  {OCLT_uchar, 8, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 128
  {OCLT_uchar, 16, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 129
  {OCLT_short, 2, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 130
  {OCLT_short, 3, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 131
  {OCLT_short, 4, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 132
  {OCLT_short, 8, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 133
  {OCLT_short, 16, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 134
  {OCLT_ushort, 2, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 135
  {OCLT_ushort, 3, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 136
  {OCLT_ushort, 4, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 137
  {OCLT_ushort, 8, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 138
  {OCLT_ushort, 16, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 139
  {OCLT_int, 2, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 140
  {OCLT_int, 3, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 141
  {OCLT_int, 4, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 142
  {OCLT_int, 8, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 143
  {OCLT_int, 16, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 144
  {OCLT_uint, 2, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 145
  {OCLT_uint, 3, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 146
  {OCLT_uint, 4, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 147
  {OCLT_uint, 8, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 148
  {OCLT_uint, 16, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 149
  {OCLT_long, 2, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 150
  {OCLT_long, 3, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 151
  {OCLT_long, 4, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 152
  {OCLT_long, 8, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 153
  {OCLT_long, 16, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 154
  {OCLT_ulong, 2, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 155
  {OCLT_ulong, 3, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 156
  {OCLT_ulong, 4, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 157
  {OCLT_ulong, 8, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 158
  {OCLT_ulong, 16, 0, 0, 0, OCLAQ_None, clang::LangAS::Default},
  // 159
  {OCLT_FGenTypeN, 0, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 160
  {OCLT_GenTypeIntVecAndScalar, 0, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 161
  {OCLT_FGenTypeN, 0, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 162
  {OCLT_GenTypeIntVecAndScalar, 0, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 163
  {OCLT_FGenTypeN, 0, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 164
  {OCLT_GenTypeIntVecAndScalar, 0, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 165
  {OCLT_FGenTypeN, 0, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 166
  {OCLT_GenTypeIntVecAndScalar, 0, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 167
  {OCLT_char, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 168
  {OCLT_uchar, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 169
  {OCLT_short, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 170
  {OCLT_ushort, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 171
  {OCLT_int, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 172
  {OCLT_uint, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 173
  {OCLT_long, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 174
  {OCLT_ulong, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 175
  {OCLT_float, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 176
  {OCLT_double, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 177
  {OCLT_half, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 178
  {OCLT_char, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 179
  {OCLT_uchar, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 180
  {OCLT_short, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 181
  {OCLT_ushort, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 182
  {OCLT_int, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 183
  {OCLT_uint, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 184
  {OCLT_long, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 185
  {OCLT_ulong, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 186
  {OCLT_float, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 187
  {OCLT_double, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 188
  {OCLT_half, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 189
  {OCLT_char, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 190
  {OCLT_uchar, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 191
  {OCLT_short, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 192
  {OCLT_ushort, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 193
  {OCLT_int, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 194
  {OCLT_uint, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 195
  {OCLT_long, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 196
  {OCLT_ulong, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 197
  {OCLT_float, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 198
  {OCLT_double, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 199
  {OCLT_half, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 200
  {OCLT_char, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 201
  {OCLT_uchar, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 202
  {OCLT_short, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 203
  {OCLT_ushort, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 204
  {OCLT_int, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 205
  {OCLT_uint, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 206
  {OCLT_long, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 207
  {OCLT_ulong, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 208
  {OCLT_float, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 209
  {OCLT_double, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 210
  {OCLT_half, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 211
  {OCLT_char, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 212
  {OCLT_uchar, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 213
  {OCLT_short, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 214
  {OCLT_ushort, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 215
  {OCLT_int, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 216
  {OCLT_uint, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 217
  {OCLT_long, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 218
  {OCLT_ulong, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 219
  {OCLT_float, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 220
  {OCLT_double, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 221
  {OCLT_half, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 222
  {OCLT_char, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 223
  {OCLT_uchar, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 224
  {OCLT_short, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 225
  {OCLT_ushort, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 226
  {OCLT_int, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 227
  {OCLT_uint, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 228
  {OCLT_long, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 229
  {OCLT_ulong, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 230
  {OCLT_float, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 231
  {OCLT_double, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 232
  {OCLT_half, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 233
  {OCLT_char, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 234
  {OCLT_uchar, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 235
  {OCLT_short, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 236
  {OCLT_ushort, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 237
  {OCLT_int, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 238
  {OCLT_uint, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 239
  {OCLT_long, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 240
  {OCLT_ulong, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 241
  {OCLT_float, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 242
  {OCLT_double, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 243
  {OCLT_half, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 244
  {OCLT_char, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 245
  {OCLT_uchar, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 246
  {OCLT_short, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 247
  {OCLT_ushort, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 248
  {OCLT_int, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 249
  {OCLT_uint, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 250
  {OCLT_long, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 251
  {OCLT_ulong, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 252
  {OCLT_float, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 253
  {OCLT_double, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 254
  {OCLT_half, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 255
  {OCLT_char, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 256
  {OCLT_uchar, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 257
  {OCLT_short, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 258
  {OCLT_ushort, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 259
  {OCLT_int, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 260
  {OCLT_uint, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 261
  {OCLT_long, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 262
  {OCLT_ulong, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 263
  {OCLT_float, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 264
  {OCLT_double, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 265
  {OCLT_half, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 266
  {OCLT_char, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_constant},
  // 267
  {OCLT_uchar, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_constant},
  // 268
  {OCLT_short, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_constant},
  // 269
  {OCLT_ushort, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_constant},
  // 270
  {OCLT_int, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_constant},
  // 271
  {OCLT_uint, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_constant},
  // 272
  {OCLT_long, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_constant},
  // 273
  {OCLT_ulong, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_constant},
  // 274
  {OCLT_float, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_constant},
  // 275
  {OCLT_double, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_constant},
  // 276
  {OCLT_half, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_constant},
  // 277
  {OCLT_void, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 278
  {OCLT_void, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 279
  {OCLT_void, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 280
  {OCLT_AGenTypeN, 0, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 281
  {OCLT_AGenTypeN, 0, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 282
  {OCLT_AGenTypeN, 0, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 283
  {OCLT_AGenTypeN, 0, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 284
  {OCLT_AGenTypeN, 0, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_local},
  // 285
  {OCLT_event_t, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 286
  {OCLT_event_t, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 287
  {OCLT_int, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 288
  {OCLT_int, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 289
  {OCLT_uint, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 290
  {OCLT_uint, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 291
  {OCLT_int, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 292
  {OCLT_uint, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 293
  {OCLT_long, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 294
  {OCLT_long, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 295
  {OCLT_ulong, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 296
  {OCLT_ulong, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 297
  {OCLT_long, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 298
  {OCLT_ulong, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 299
  {OCLT_float, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 300
  {OCLT_float, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 301
  {OCLT_float, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 302
  {OCLT_int, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_generic},
  // 303
  {OCLT_uint, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_generic},
  // 304
  {OCLT_atomic_int, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 305
  {OCLT_atomic_uint, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 306
  {OCLT_atomic_long, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 307
  {OCLT_atomic_ulong, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 308
  {OCLT_atomic_float, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 309
  {OCLT_atomic_double, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 310
  {OCLT_atomic_uintptr_t, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 311
  {OCLT_atomic_flag, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 312
  {OCLT_atomic_int, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_generic},
  // 313
  {OCLT_atomic_uint, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_generic},
  // 314
  {OCLT_atomic_long, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_generic},
  // 315
  {OCLT_atomic_ulong, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_generic},
  // 316
  {OCLT_atomic_float, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_generic},
  // 317
  {OCLT_atomic_double, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_generic},
  // 318
  {OCLT_atomic_uintptr_t, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_generic},
  // 319
  {OCLT_atomic_flag, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_generic},
  // 320
  {OCLT_atomic_int, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 321
  {OCLT_atomic_uint, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 322
  {OCLT_atomic_long, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 323
  {OCLT_atomic_ulong, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 324
  {OCLT_atomic_float, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 325
  {OCLT_atomic_double, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 326
  {OCLT_atomic_uintptr_t, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 327
  {OCLT_atomic_flag, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 328
  {OCLT_atomic_int, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 329
  {OCLT_atomic_uint, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 330
  {OCLT_atomic_long, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 331
  {OCLT_atomic_ulong, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 332
  {OCLT_atomic_float, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 333
  {OCLT_atomic_double, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 334
  {OCLT_atomic_uintptr_t, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 335
  {OCLT_atomic_flag, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 336
  {OCLT_atomic_half, 1, 0, 0, 1, OCLAQ_None, clang::LangAS::Default},
  // 337
  {OCLT_atomic_half, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_global},
  // 338
  {OCLT_atomic_half, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_local},
  // 339
  {OCLT_atomic_half, 1, 1, 0, 1, OCLAQ_None, clang::LangAS::opencl_generic},
  // 340
  {OCLT_image1d_t, 0, 0, 0, 0, OCLAQ_ReadOnly, clang::LangAS::Default},
  // 341
  {OCLT_image2d_t, 0, 0, 0, 0, OCLAQ_ReadOnly, clang::LangAS::Default},
  // 342
  {OCLT_image1d_array_t, 0, 0, 0, 0, OCLAQ_ReadOnly, clang::LangAS::Default},
  // 343
  {OCLT_image3d_t, 0, 0, 0, 0, OCLAQ_ReadOnly, clang::LangAS::Default},
  // 344
  {OCLT_image2d_array_t, 0, 0, 0, 0, OCLAQ_ReadOnly, clang::LangAS::Default},
  // 345
  {OCLT_image2d_depth_t, 0, 0, 0, 0, OCLAQ_ReadOnly, clang::LangAS::Default},
  // 346
  {OCLT_image2d_array_depth_t, 0, 0, 0, 0, OCLAQ_ReadOnly, clang::LangAS::Default},
  // 347
  {OCLT_image1d_buffer_t, 0, 0, 0, 0, OCLAQ_ReadOnly, clang::LangAS::Default},
  // 348
  {OCLT_image2d_t, 0, 0, 0, 0, OCLAQ_ReadWrite, clang::LangAS::Default},
  // 349
  {OCLT_image1d_array_t, 0, 0, 0, 0, OCLAQ_ReadWrite, clang::LangAS::Default},
  // 350
  {OCLT_image3d_t, 0, 0, 0, 0, OCLAQ_ReadWrite, clang::LangAS::Default},
  // 351
  {OCLT_image2d_array_t, 0, 0, 0, 0, OCLAQ_ReadWrite, clang::LangAS::Default},
  // 352
  {OCLT_image1d_t, 0, 0, 0, 0, OCLAQ_ReadWrite, clang::LangAS::Default},
  // 353
  {OCLT_image1d_buffer_t, 0, 0, 0, 0, OCLAQ_ReadWrite, clang::LangAS::Default},
  // 354
  {OCLT_image2d_depth_t, 0, 0, 0, 0, OCLAQ_ReadWrite, clang::LangAS::Default},
  // 355
  {OCLT_image2d_array_depth_t, 0, 0, 0, 0, OCLAQ_ReadWrite, clang::LangAS::Default},
  // 356
  {OCLT_image2d_t, 0, 0, 0, 0, OCLAQ_WriteOnly, clang::LangAS::Default},
  // 357
  {OCLT_image2d_array_t, 0, 0, 0, 0, OCLAQ_WriteOnly, clang::LangAS::Default},
  // 358
  {OCLT_image1d_t, 0, 0, 0, 0, OCLAQ_WriteOnly, clang::LangAS::Default},
  // 359
  {OCLT_image1d_buffer_t, 0, 0, 0, 0, OCLAQ_WriteOnly, clang::LangAS::Default},
  // 360
  {OCLT_image1d_array_t, 0, 0, 0, 0, OCLAQ_WriteOnly, clang::LangAS::Default},
  // 361
  {OCLT_image3d_t, 0, 0, 0, 0, OCLAQ_WriteOnly, clang::LangAS::Default},
  // 362
  {OCLT_image2d_depth_t, 0, 0, 0, 0, OCLAQ_WriteOnly, clang::LangAS::Default},
  // 363
  {OCLT_image2d_array_depth_t, 0, 0, 0, 0, OCLAQ_WriteOnly, clang::LangAS::Default},
  // 364
  {OCLT_clk_event_t, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 365
  {OCLT_clk_event_t, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 366
  {OCLT_clk_event_t, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_generic},
  // 367
  {OCLT_void, 1, 1, 0, 0, OCLAQ_None, clang::LangAS::opencl_global},
  // 368
  {OCLT_size_t, 1, 0, 1, 0, OCLAQ_None, clang::LangAS::Default},
  // 369
  {OCLT_size_t, 1, 1, 1, 0, OCLAQ_None, clang::LangAS::opencl_private},
  // 370
  {OCLT_image2d_msaa_t, 0, 0, 0, 0, OCLAQ_ReadOnly, clang::LangAS::Default},
  // 371
  {OCLT_image2d_array_msaa_t, 0, 0, 0, 0, OCLAQ_ReadOnly, clang::LangAS::Default},
  // 372
  {OCLT_image2d_msaa_depth_t, 0, 0, 0, 0, OCLAQ_ReadOnly, clang::LangAS::Default},
  // 373
  {OCLT_image2d_array_msaa_depth_t, 0, 0, 0, 0, OCLAQ_ReadOnly, clang::LangAS::Default},
  // 374
  {OCLT_image2d_msaa_t, 0, 0, 0, 0, OCLAQ_WriteOnly, clang::LangAS::Default},
  // 375
  {OCLT_image2d_array_msaa_t, 0, 0, 0, 0, OCLAQ_WriteOnly, clang::LangAS::Default},
  // 376
  {OCLT_image2d_msaa_depth_t, 0, 0, 0, 0, OCLAQ_WriteOnly, clang::LangAS::Default},
  // 377
  {OCLT_image2d_array_msaa_depth_t, 0, 0, 0, 0, OCLAQ_WriteOnly, clang::LangAS::Default},
  // 378
  {OCLT_image2d_msaa_t, 0, 0, 0, 0, OCLAQ_ReadWrite, clang::LangAS::Default},
  // 379
  {OCLT_image2d_array_msaa_t, 0, 0, 0, 0, OCLAQ_ReadWrite, clang::LangAS::Default},
  // 380
  {OCLT_image2d_msaa_depth_t, 0, 0, 0, 0, OCLAQ_ReadWrite, clang::LangAS::Default},
  // 381
  {OCLT_image2d_array_msaa_depth_t, 0, 0, 0, 0, OCLAQ_ReadWrite, clang::LangAS::Default},
};

static const unsigned short SignatureTable[] = {
  // 0
  27, 27, 
  // 2
  104, 104, 
  // 4
  105, 105, 
  // 6
  106, 106, 
  // 8
  107, 107, 
  // 10
  108, 108, 
  // 12
  27, 24, 
  // 14
  104, 109, 
  // 16
  105, 110, 
  // 18
  106, 111, 
  // 20
  107, 112, 
  // 22
  108, 113, 
  // 24
  27, 57, 
  // 26
  104, 114, 
  // 28
  105, 115, 
  // 30
  106, 116, 
  // 32
  107, 117, 
  // 34
  108, 118, 
  // 36
  27, 20, 
  // 38
  104, 119, 
  // 40
  105, 120, 
  // 42
  106, 121, 
  // 44
  107, 122, 
  // 46
  108, 123, 
  // 48
  27, 95, 
  // 50
  104, 124, 
  // 52
  105, 125, 
  // 54
  106, 126, 
  // 56
  107, 127, 
  // 58
  108, 128, 
  // 60
  27, 93, 
  // 62
  104, 129, 
  // 64
  105, 130, 
  // 66
  106, 131, 
  // 68
  107, 132, 
  // 70
  108, 133, 
  // 72
  27, 100, 
  // 74
  104, 134, 
  // 76
  105, 135, 
  // 78
  106, 136, 
  // 80
  107, 137, 
  // 82
  108, 138, 
  // 84
  27, 80, 
  // 86
  104, 139, 
  // 88
  105, 140, 
  // 90
  106, 141, 
  // 92
  107, 142, 
  // 94
  108, 143, 
  // 96
  27, 97, 
  // 98
  104, 144, 
  // 100
  105, 145, 
  // 102
  106, 146, 
  // 104
  107, 147, 
  // 106
  108, 148, 
  // 108
  27, 83, 
  // 110
  104, 149, 
  // 112
  105, 150, 
  // 114
  106, 151, 
  // 116
  107, 152, 
  // 118
  108, 153, 
  // 120
  27, 99, 
  // 122
  104, 154, 
  // 124
  105, 155, 
  // 126
  106, 156, 
  // 128
  107, 157, 
  // 130
  108, 158, 
  // 132
  24, 27, 
  // 134
  109, 104, 
  // 136
  110, 105, 
  // 138
  111, 106, 
  // 140
  112, 107, 
  // 142
  113, 108, 
  // 144
  24, 24, 
  // 146
  109, 109, 
  // 148
  110, 110, 
  // 150
  111, 111, 
  // 152
  112, 112, 
  // 154
  113, 113, 
  // 156
  24, 57, 
  // 158
  109, 114, 
  // 160
  110, 115, 
  // 162
  111, 116, 
  // 164
  112, 117, 
  // 166
  113, 118, 
  // 168
  24, 20, 
  // 170
  109, 119, 
  // 172
  110, 120, 
  // 174
  111, 121, 
  // 176
  112, 122, 
  // 178
  113, 123, 
  // 180
  24, 95, 
  // 182
  109, 124, 
  // 184
  110, 125, 
  // 186
  111, 126, 
  // 188
  112, 127, 
  // 190
  113, 128, 
  // 192
  24, 93, 
  // 194
  109, 129, 
  // 196
  110, 130, 
  // 198
  111, 131, 
  // 200
  112, 132, 
  // 202
  113, 133, 
  // 204
  24, 100, 
  // 206
  109, 134, 
  // 208
  110, 135, 
  // 210
  111, 136, 
  // 212
  112, 137, 
  // 214
  113, 138, 
  // 216
  24, 80, 
  // 218
  109, 139, 
  // 220
  110, 140, 
  // 222
  111, 141, 
  // 224
  112, 142, 
  // 226
  113, 143, 
  // 228
  24, 97, 
  // 230
  109, 144, 
  // 232
  110, 145, 
  // 234
  111, 146, 
  // 236
  112, 147, 
  // 238
  113, 148, 
  // 240
  24, 83, 
  // 242
  109, 149, 
  // 244
  110, 150, 
  // 246
  111, 151, 
  // 248
  112, 152, 
  // 250
  113, 153, 
  // 252
  24, 99, 
  // 254
  109, 154, 
  // 256
  110, 155, 
  // 258
  111, 156, 
  // 260
  112, 157, 
  // 262
  113, 158, 
  // 264
  57, 27, 
  // 266
  114, 104, 
  // 268
  115, 105, 
  // 270
  116, 106, 
  // 272
  117, 107, 
  // 274
  118, 108, 
  // 276
  57, 24, 
  // 278
  114, 109, 
  // 280
  115, 110, 
  // 282
  116, 111, 
  // 284
  117, 112, 
  // 286
  118, 113, 
  // 288
  57, 57, 
  // 290
  114, 114, 
  // 292
  115, 115, 
  // 294
  116, 116, 
  // 296
  117, 117, 
  // 298
  118, 118, 
  // 300
  57, 20, 
  // 302
  114, 119, 
  // 304
  115, 120, 
  // 306
  116, 121, 
  // 308
  117, 122, 
  // 310
  118, 123, 
  // 312
  57, 95, 
  // 314
  114, 124, 
  // 316
  115, 125, 
  // 318
  116, 126, 
  // 320
  117, 127, 
  // 322
  118, 128, 
  // 324
  57, 93, 
  // 326
  114, 129, 
  // 328
  115, 130, 
  // 330
  116, 131, 
  // 332
  117, 132, 
  // 334
  118, 133, 
  // 336
  57, 100, 
  // 338
  114, 134, 
  // 340
  115, 135, 
  // 342
  116, 136, 
  // 344
  117, 137, 
  // 346
  118, 138, 
  // 348
  57, 80, 
  // 350
  114, 139, 
  // 352
  115, 140, 
  // 354
  116, 141, 
  // 356
  117, 142, 
  // 358
  118, 143, 
  // 360
  57, 97, 
  // 362
  114, 144, 
  // 364
  115, 145, 
  // 366
  116, 146, 
  // 368
  117, 147, 
  // 370
  118, 148, 
  // 372
  57, 83, 
  // 374
  114, 149, 
  // 376
  115, 150, 
  // 378
  116, 151, 
  // 380
  117, 152, 
  // 382
  118, 153, 
  // 384
  57, 99, 
  // 386
  114, 154, 
  // 388
  115, 155, 
  // 390
  116, 156, 
  // 392
  117, 157, 
  // 394
  118, 158, 
  // 396
  20, 27, 
  // 398
  119, 104, 
  // 400
  120, 105, 
  // 402
  121, 106, 
  // 404
  122, 107, 
  // 406
  123, 108, 
  // 408
  20, 24, 
  // 410
  119, 109, 
  // 412
  120, 110, 
  // 414
  121, 111, 
  // 416
  122, 112, 
  // 418
  123, 113, 
  // 420
  20, 57, 
  // 422
  119, 114, 
  // 424
  120, 115, 
  // 426
  121, 116, 
  // 428
  122, 117, 
  // 430
  123, 118, 
  // 432
  20, 20, 
  // 434
  119, 119, 
  // 436
  120, 120, 
  // 438
  121, 121, 
  // 440
  122, 122, 
  // 442
  123, 123, 
  // 444
  20, 95, 
  // 446
  119, 124, 
  // 448
  120, 125, 
  // 450
  121, 126, 
  // 452
  122, 127, 
  // 454
  123, 128, 
  // 456
  20, 93, 
  // 458
  119, 129, 
  // 460
  120, 130, 
  // 462
  121, 131, 
  // 464
  122, 132, 
  // 466
  123, 133, 
  // 468
  20, 100, 
  // 470
  119, 134, 
  // 472
  120, 135, 
  // 474
  121, 136, 
  // 476
  122, 137, 
  // 478
  123, 138, 
  // 480
  20, 80, 
  // 482
  119, 139, 
  // 484
  120, 140, 
  // 486
  121, 141, 
  // 488
  122, 142, 
  // 490
  123, 143, 
  // 492
  20, 97, 
  // 494
  119, 144, 
  // 496
  120, 145, 
  // 498
  121, 146, 
  // 500
  122, 147, 
  // 502
  123, 148, 
  // 504
  20, 83, 
  // 506
  119, 149, 
  // 508
  120, 150, 
  // 510
  121, 151, 
  // 512
  122, 152, 
  // 514
  123, 153, 
  // 516
  20, 99, 
  // 518
  119, 154, 
  // 520
  120, 155, 
  // 522
  121, 156, 
  // 524
  122, 157, 
  // 526
  123, 158, 
  // 528
  95, 27, 
  // 530
  124, 104, 
  // 532
  125, 105, 
  // 534
  126, 106, 
  // 536
  127, 107, 
  // 538
  128, 108, 
  // 540
  95, 24, 
  // 542
  124, 109, 
  // 544
  125, 110, 
  // 546
  126, 111, 
  // 548
  127, 112, 
  // 550
  128, 113, 
  // 552
  95, 57, 
  // 554
  124, 114, 
  // 556
  125, 115, 
  // 558
  126, 116, 
  // 560
  127, 117, 
  // 562
  128, 118, 
  // 564
  95, 20, 
  // 566
  124, 119, 
  // 568
  125, 120, 
  // 570
  126, 121, 
  // 572
  127, 122, 
  // 574
  128, 123, 
  // 576
  95, 95, 
  // 578
  124, 124, 
  // 580
  125, 125, 
  // 582
  126, 126, 
  // 584
  127, 127, 
  // 586
  128, 128, 
  // 588
  95, 93, 
  // 590
  124, 129, 
  // 592
  125, 130, 
  // 594
  126, 131, 
  // 596
  127, 132, 
  // 598
  128, 133, 
  // 600
  95, 100, 
  // 602
  124, 134, 
  // 604
  125, 135, 
  // 606
  126, 136, 
  // 608
  127, 137, 
  // 610
  128, 138, 
  // 612
  95, 80, 
  // 614
  124, 139, 
  // 616
  125, 140, 
  // 618
  126, 141, 
  // 620
  127, 142, 
  // 622
  128, 143, 
  // 624
  95, 97, 
  // 626
  124, 144, 
  // 628
  125, 145, 
  // 630
  126, 146, 
  // 632
  127, 147, 
  // 634
  128, 148, 
  // 636
  95, 83, 
  // 638
  124, 149, 
  // 640
  125, 150, 
  // 642
  126, 151, 
  // 644
  127, 152, 
  // 646
  128, 153, 
  // 648
  95, 99, 
  // 650
  124, 154, 
  // 652
  125, 155, 
  // 654
  126, 156, 
  // 656
  127, 157, 
  // 658
  128, 158, 
  // 660
  93, 27, 
  // 662
  129, 104, 
  // 664
  130, 105, 
  // 666
  131, 106, 
  // 668
  132, 107, 
  // 670
  133, 108, 
  // 672
  93, 24, 
  // 674
  129, 109, 
  // 676
  130, 110, 
  // 678
  131, 111, 
  // 680
  132, 112, 
  // 682
  133, 113, 
  // 684
  93, 57, 
  // 686
  129, 114, 
  // 688
  130, 115, 
  // 690
  131, 116, 
  // 692
  132, 117, 
  // 694
  133, 118, 
  // 696
  93, 20, 
  // 698
  129, 119, 
  // 700
  130, 120, 
  // 702
  131, 121, 
  // 704
  132, 122, 
  // 706
  133, 123, 
  // 708
  93, 95, 
  // 710
  129, 124, 
  // 712
  130, 125, 
  // 714
  131, 126, 
  // 716
  132, 127, 
  // 718
  133, 128, 
  // 720
  93, 93, 
  // 722
  129, 129, 
  // 724
  130, 130, 
  // 726
  131, 131, 
  // 728
  132, 132, 
  // 730
  133, 133, 
  // 732
  93, 100, 
  // 734
  129, 134, 
  // 736
  130, 135, 
  // 738
  131, 136, 
  // 740
  132, 137, 
  // 742
  133, 138, 
  // 744
  93, 80, 
  // 746
  129, 139, 
  // 748
  130, 140, 
  // 750
  131, 141, 
  // 752
  132, 142, 
  // 754
  133, 143, 
  // 756
  93, 97, 
  // 758
  129, 144, 
  // 760
  130, 145, 
  // 762
  131, 146, 
  // 764
  132, 147, 
  // 766
  133, 148, 
  // 768
  93, 83, 
  // 770
  129, 149, 
  // 772
  130, 150, 
  // 774
  131, 151, 
  // 776
  132, 152, 
  // 778
  133, 153, 
  // 780
  93, 99, 
  // 782
  129, 154, 
  // 784
  130, 155, 
  // 786
  131, 156, 
  // 788
  132, 157, 
  // 790
  133, 158, 
  // 792
  100, 27, 
  // 794
  134, 104, 
  // 796
  135, 105, 
  // 798
  136, 106, 
  // 800
  137, 107, 
  // 802
  138, 108, 
  // 804
  100, 24, 
  // 806
  134, 109, 
  // 808
  135, 110, 
  // 810
  136, 111, 
  // 812
  137, 112, 
  // 814
  138, 113, 
  // 816
  100, 57, 
  // 818
  134, 114, 
  // 820
  135, 115, 
  // 822
  136, 116, 
  // 824
  137, 117, 
  // 826
  138, 118, 
  // 828
  100, 20, 
  // 830
  134, 119, 
  // 832
  135, 120, 
  // 834
  136, 121, 
  // 836
  137, 122, 
  // 838
  138, 123, 
  // 840
  100, 95, 
  // 842
  134, 124, 
  // 844
  135, 125, 
  // 846
  136, 126, 
  // 848
  137, 127, 
  // 850
  138, 128, 
  // 852
  100, 93, 
  // 854
  134, 129, 
  // 856
  135, 130, 
  // 858
  136, 131, 
  // 860
  137, 132, 
  // 862
  138, 133, 
  // 864
  100, 100, 
  // 866
  134, 134, 
  // 868
  135, 135, 
  // 870
  136, 136, 
  // 872
  137, 137, 
  // 874
  138, 138, 
  // 876
  100, 80, 
  // 878
  134, 139, 
  // 880
  135, 140, 
  // 882
  136, 141, 
  // 884
  137, 142, 
  // 886
  138, 143, 
  // 888
  100, 97, 
  // 890
  134, 144, 
  // 892
  135, 145, 
  // 894
  136, 146, 
  // 896
  137, 147, 
  // 898
  138, 148, 
  // 900
  100, 83, 
  // 902
  134, 149, 
  // 904
  135, 150, 
  // 906
  136, 151, 
  // 908
  137, 152, 
  // 910
  138, 153, 
  // 912
  100, 99, 
  // 914
  134, 154, 
  // 916
  135, 155, 
  // 918
  136, 156, 
  // 920
  137, 157, 
  // 922
  138, 158, 
  // 924
  80, 27, 
  // 926
  139, 104, 
  // 928
  140, 105, 
  // 930
  141, 106, 
  // 932
  142, 107, 
  // 934
  143, 108, 
  // 936
  80, 24, 
  // 938
  139, 109, 
  // 940
  140, 110, 
  // 942
  141, 111, 
  // 944
  142, 112, 
  // 946
  143, 113, 
  // 948
  80, 57, 
  // 950
  139, 114, 
  // 952
  140, 115, 
  // 954
  141, 116, 
  // 956
  142, 117, 
  // 958
  143, 118, 
  // 960
  80, 20, 
  // 962
  139, 119, 
  // 964
  140, 120, 
  // 966
  141, 121, 
  // 968
  142, 122, 
  // 970
  143, 123, 
  // 972
  80, 95, 
  // 974
  139, 124, 
  // 976
  140, 125, 
  // 978
  141, 126, 
  // 980
  142, 127, 
  // 982
  143, 128, 
  // 984
  80, 93, 
  // 986
  139, 129, 
  // 988
  140, 130, 
  // 990
  141, 131, 
  // 992
  142, 132, 
  // 994
  143, 133, 
  // 996
  80, 100, 
  // 998
  139, 134, 
  // 1000
  140, 135, 
  // 1002
  141, 136, 
  // 1004
  142, 137, 
  // 1006
  143, 138, 
  // 1008
  80, 80, 
  // 1010
  139, 139, 
  // 1012
  140, 140, 
  // 1014
  141, 141, 
  // 1016
  142, 142, 
  // 1018
  143, 143, 
  // 1020
  80, 97, 
  // 1022
  139, 144, 
  // 1024
  140, 145, 
  // 1026
  141, 146, 
  // 1028
  142, 147, 
  // 1030
  143, 148, 
  // 1032
  80, 83, 
  // 1034
  139, 149, 
  // 1036
  140, 150, 
  // 1038
  141, 151, 
  // 1040
  142, 152, 
  // 1042
  143, 153, 
  // 1044
  80, 99, 
  // 1046
  139, 154, 
  // 1048
  140, 155, 
  // 1050
  141, 156, 
  // 1052
  142, 157, 
  // 1054
  143, 158, 
  // 1056
  97, 27, 
  // 1058
  144, 104, 
  // 1060
  145, 105, 
  // 1062
  146, 106, 
  // 1064
  147, 107, 
  // 1066
  148, 108, 
  // 1068
  97, 24, 
  // 1070
  144, 109, 
  // 1072
  145, 110, 
  // 1074
  146, 111, 
  // 1076
  147, 112, 
  // 1078
  148, 113, 
  // 1080
  97, 57, 
  // 1082
  144, 114, 
  // 1084
  145, 115, 
  // 1086
  146, 116, 
  // 1088
  147, 117, 
  // 1090
  148, 118, 
  // 1092
  97, 20, 
  // 1094
  144, 119, 
  // 1096
  145, 120, 
  // 1098
  146, 121, 
  // 1100
  147, 122, 
  // 1102
  148, 123, 
  // 1104
  97, 95, 
  // 1106
  144, 124, 
  // 1108
  145, 125, 
  // 1110
  146, 126, 
  // 1112
  147, 127, 
  // 1114
  148, 128, 
  // 1116
  97, 93, 
  // 1118
  144, 129, 
  // 1120
  145, 130, 
  // 1122
  146, 131, 
  // 1124
  147, 132, 
  // 1126
  148, 133, 
  // 1128
  97, 100, 
  // 1130
  144, 134, 
  // 1132
  145, 135, 
  // 1134
  146, 136, 
  // 1136
  147, 137, 
  // 1138
  148, 138, 
  // 1140
  97, 80, 
  // 1142
  144, 139, 
  // 1144
  145, 140, 
  // 1146
  146, 141, 
  // 1148
  147, 142, 
  // 1150
  148, 143, 
  // 1152
  97, 97, 
  // 1154
  144, 144, 
  // 1156
  145, 145, 
  // 1158
  146, 146, 
  // 1160
  147, 147, 
  // 1162
  148, 148, 
  // 1164
  97, 83, 
  // 1166
  144, 149, 
  // 1168
  145, 150, 
  // 1170
  146, 151, 
  // 1172
  147, 152, 
  // 1174
  148, 153, 
  // 1176
  97, 99, 
  // 1178
  144, 154, 
  // 1180
  145, 155, 
  // 1182
  146, 156, 
  // 1184
  147, 157, 
  // 1186
  148, 158, 
  // 1188
  83, 27, 
  // 1190
  149, 104, 
  // 1192
  150, 105, 
  // 1194
  151, 106, 
  // 1196
  152, 107, 
  // 1198
  153, 108, 
  // 1200
  83, 24, 
  // 1202
  149, 109, 
  // 1204
  150, 110, 
  // 1206
  151, 111, 
  // 1208
  152, 112, 
  // 1210
  153, 113, 
  // 1212
  83, 57, 
  // 1214
  149, 114, 
  // 1216
  150, 115, 
  // 1218
  151, 116, 
  // 1220
  152, 117, 
  // 1222
  153, 118, 
  // 1224
  83, 20, 
  // 1226
  149, 119, 
  // 1228
  150, 120, 
  // 1230
  151, 121, 
  // 1232
  152, 122, 
  // 1234
  153, 123, 
  // 1236
  83, 95, 
  // 1238
  149, 124, 
  // 1240
  150, 125, 
  // 1242
  151, 126, 
  // 1244
  152, 127, 
  // 1246
  153, 128, 
  // 1248
  83, 93, 
  // 1250
  149, 129, 
  // 1252
  150, 130, 
  // 1254
  151, 131, 
  // 1256
  152, 132, 
  // 1258
  153, 133, 
  // 1260
  83, 100, 
  // 1262
  149, 134, 
  // 1264
  150, 135, 
  // 1266
  151, 136, 
  // 1268
  152, 137, 
  // 1270
  153, 138, 
  // 1272
  83, 80, 
  // 1274
  149, 139, 
  // 1276
  150, 140, 
  // 1278
  151, 141, 
  // 1280
  152, 142, 
  // 1282
  153, 143, 
  // 1284
  83, 97, 
  // 1286
  149, 144, 
  // 1288
  150, 145, 
  // 1290
  151, 146, 
  // 1292
  152, 147, 
  // 1294
  153, 148, 
  // 1296
  83, 83, 
  // 1298
  149, 149, 
  // 1300
  150, 150, 
  // 1302
  151, 151, 
  // 1304
  152, 152, 
  // 1306
  153, 153, 
  // 1308
  83, 99, 
  // 1310
  149, 154, 
  // 1312
  150, 155, 
  // 1314
  151, 156, 
  // 1316
  152, 157, 
  // 1318
  153, 158, 
  // 1320
  99, 27, 
  // 1322
  154, 104, 
  // 1324
  155, 105, 
  // 1326
  156, 106, 
  // 1328
  157, 107, 
  // 1330
  158, 108, 
  // 1332
  99, 24, 
  // 1334
  154, 109, 
  // 1336
  155, 110, 
  // 1338
  156, 111, 
  // 1340
  157, 112, 
  // 1342
  158, 113, 
  // 1344
  99, 57, 
  // 1346
  154, 114, 
  // 1348
  155, 115, 
  // 1350
  156, 116, 
  // 1352
  157, 117, 
  // 1354
  158, 118, 
  // 1356
  99, 20, 
  // 1358
  154, 119, 
  // 1360
  155, 120, 
  // 1362
  156, 121, 
  // 1364
  157, 122, 
  // 1366
  158, 123, 
  // 1368
  99, 95, 
  // 1370
  154, 124, 
  // 1372
  155, 125, 
  // 1374
  156, 126, 
  // 1376
  157, 127, 
  // 1378
  158, 128, 
  // 1380
  99, 93, 
  // 1382
  154, 129, 
  // 1384
  155, 130, 
  // 1386
  156, 131, 
  // 1388
  157, 132, 
  // 1390
  158, 133, 
  // 1392
  99, 100, 
  // 1394
  154, 134, 
  // 1396
  155, 135, 
  // 1398
  156, 136, 
  // 1400
  157, 137, 
  // 1402
  158, 138, 
  // 1404
  99, 80, 
  // 1406
  154, 139, 
  // 1408
  155, 140, 
  // 1410
  156, 141, 
  // 1412
  157, 142, 
  // 1414
  158, 143, 
  // 1416
  99, 97, 
  // 1418
  154, 144, 
  // 1420
  155, 145, 
  // 1422
  156, 146, 
  // 1424
  157, 147, 
  // 1426
  158, 148, 
  // 1428
  99, 83, 
  // 1430
  154, 149, 
  // 1432
  155, 150, 
  // 1434
  156, 151, 
  // 1436
  157, 152, 
  // 1438
  158, 153, 
  // 1440
  99, 99, 
  // 1442
  154, 154, 
  // 1444
  155, 155, 
  // 1446
  156, 156, 
  // 1448
  157, 157, 
  // 1450
  158, 158, 
  // 1452
  97, 
  // 1453
  94, 97, 
  // 1455
  94, 
  // 1456
  26, 26, 
  // 1458
  35, 50, 
  // 1460
  32, 52, 
  // 1462
  38, 54, 
  // 1464
  26, 26, 26, 
  // 1467
  36, 36, 27, 
  // 1470
  33, 33, 24, 
  // 1473
  39, 39, 57, 
  // 1476
  40, 35, 
  // 1478
  40, 32, 
  // 1480
  40, 38, 
  // 1482
  35, 35, 40, 
  // 1485
  36, 36, 80, 
  // 1488
  32, 32, 40, 
  // 1491
  33, 33, 80, 
  // 1494
  38, 38, 40, 
  // 1497
  39, 39, 80, 
  // 1500
  26, 26, 26, 26, 
  // 1504
  26, 26, 159, 
  // 1507
  35, 35, 160, 
  // 1510
  35, 35, 35, 160, 
  // 1514
  32, 32, 160, 
  // 1517
  38, 38, 160, 
  // 1520
  32, 32, 32, 160, 
  // 1524
  38, 38, 38, 160, 
  // 1528
  26, 26, 161, 
  // 1531
  35, 35, 162, 
  // 1534
  32, 32, 162, 
  // 1537
  38, 38, 162, 
  // 1540
  35, 35, 35, 162, 
  // 1544
  32, 32, 32, 162, 
  // 1548
  38, 38, 38, 162, 
  // 1552
  26, 26, 163, 
  // 1555
  35, 35, 164, 
  // 1558
  32, 32, 164, 
  // 1561
  38, 38, 164, 
  // 1564
  35, 35, 35, 164, 
  // 1568
  32, 32, 32, 164, 
  // 1572
  38, 38, 38, 164, 
  // 1576
  26, 26, 165, 
  // 1579
  35, 35, 166, 
  // 1582
  32, 32, 166, 
  // 1585
  38, 38, 166, 
  // 1588
  35, 35, 35, 166, 
  // 1592
  32, 32, 32, 166, 
  // 1596
  38, 38, 38, 166, 
  // 1600
  35, 35, 
  // 1602
  35, 35, 35, 
  // 1605
  3, 5, 
  // 1607
  5, 5, 
  // 1609
  3, 5, 5, 
  // 1612
  5, 5, 5, 
  // 1615
  6, 6, 4, 
  // 1618
  44, 28, 47, 
  // 1621
  54, 47, 47, 
  // 1624
  40, 44, 54, 
  // 1627
  50, 54, 54, 
  // 1630
  42, 40, 50, 
  // 1633
  52, 50, 50, 
  // 1636
  5, 5, 5, 5, 
  // 1640
  6, 6, 4, 4, 
  // 1644
  40, 40, 40, 40, 
  // 1648
  50, 50, 50, 50, 
  // 1652
  40, 40, 40, 
  // 1655
  50, 50, 50, 
  // 1658
  36, 27, 36, 
  // 1661
  33, 24, 33, 
  // 1664
  39, 57, 39, 
  // 1667
  36, 36, 27, 27, 
  // 1671
  33, 33, 24, 24, 
  // 1675
  39, 39, 57, 57, 
  // 1679
  36, 36, 36, 27, 
  // 1683
  33, 33, 33, 24, 
  // 1687
  39, 39, 39, 57, 
  // 1691
  36, 27, 27, 36, 
  // 1695
  33, 24, 24, 33, 
  // 1699
  39, 57, 57, 39, 
  // 1703
  27, 34, 
  // 1705
  24, 31, 
  // 1707
  57, 37, 
  // 1709
  34, 34, 
  // 1711
  31, 31, 
  // 1713
  37, 37, 
  // 1715
  105, 105, 105, 
  // 1718
  110, 110, 110, 
  // 1721
  115, 115, 115, 
  // 1724
  106, 106, 106, 
  // 1727
  111, 111, 111, 
  // 1730
  116, 116, 116, 
  // 1733
  27, 34, 34, 
  // 1736
  24, 31, 31, 
  // 1739
  57, 37, 37, 
  // 1742
  43, 33, 
  // 1744
  46, 39, 
  // 1746
  80, 91, 
  // 1748
  40, 35, 35, 
  // 1751
  80, 24, 24, 
  // 1754
  43, 33, 33, 
  // 1757
  80, 57, 57, 
  // 1760
  46, 39, 39, 
  // 1763
  1, 1, 1, 1, 
  // 1767
  91, 91, 91, 91, 
  // 1771
  91, 91, 91, 96, 
  // 1775
  96, 96, 96, 96, 
  // 1779
  96, 96, 96, 91, 
  // 1783
  35, 35, 35, 40, 
  // 1787
  35, 35, 35, 50, 
  // 1791
  32, 32, 32, 42, 
  // 1795
  32, 32, 32, 52, 
  // 1799
  38, 38, 38, 44, 
  // 1803
  38, 38, 38, 54, 
  // 1807
  119, 94, 178, 
  // 1810
  124, 94, 179, 
  // 1813
  129, 94, 180, 
  // 1816
  134, 94, 181, 
  // 1819
  139, 94, 182, 
  // 1822
  144, 94, 183, 
  // 1825
  149, 94, 184, 
  // 1828
  154, 94, 185, 
  // 1831
  104, 94, 186, 
  // 1834
  109, 94, 187, 
  // 1837
  114, 94, 188, 
  // 1840
  101, 119, 94, 189, 
  // 1844
  101, 124, 94, 190, 
  // 1848
  101, 129, 94, 191, 
  // 1852
  101, 134, 94, 192, 
  // 1856
  101, 139, 94, 193, 
  // 1860
  101, 144, 94, 194, 
  // 1864
  101, 149, 94, 195, 
  // 1868
  101, 154, 94, 196, 
  // 1872
  101, 104, 94, 197, 
  // 1876
  101, 109, 94, 198, 
  // 1880
  101, 114, 94, 199, 
  // 1884
  120, 94, 178, 
  // 1887
  125, 94, 179, 
  // 1890
  130, 94, 180, 
  // 1893
  135, 94, 181, 
  // 1896
  140, 94, 182, 
  // 1899
  145, 94, 183, 
  // 1902
  150, 94, 184, 
  // 1905
  155, 94, 185, 
  // 1908
  105, 94, 186, 
  // 1911
  110, 94, 187, 
  // 1914
  115, 94, 188, 
  // 1917
  101, 120, 94, 189, 
  // 1921
  101, 125, 94, 190, 
  // 1925
  101, 130, 94, 191, 
  // 1929
  101, 135, 94, 192, 
  // 1933
  101, 140, 94, 193, 
  // 1937
  101, 145, 94, 194, 
  // 1941
  101, 150, 94, 195, 
  // 1945
  101, 155, 94, 196, 
  // 1949
  101, 105, 94, 197, 
  // 1953
  101, 110, 94, 198, 
  // 1957
  101, 115, 94, 199, 
  // 1961
  121, 94, 178, 
  // 1964
  126, 94, 179, 
  // 1967
  131, 94, 180, 
  // 1970
  136, 94, 181, 
  // 1973
  141, 94, 182, 
  // 1976
  146, 94, 183, 
  // 1979
  151, 94, 184, 
  // 1982
  156, 94, 185, 
  // 1985
  106, 94, 186, 
  // 1988
  111, 94, 187, 
  // 1991
  116, 94, 188, 
  // 1994
  101, 121, 94, 189, 
  // 1998
  101, 126, 94, 190, 
  // 2002
  101, 131, 94, 191, 
  // 2006
  101, 136, 94, 192, 
  // 2010
  101, 141, 94, 193, 
  // 2014
  101, 146, 94, 194, 
  // 2018
  101, 151, 94, 195, 
  // 2022
  101, 156, 94, 196, 
  // 2026
  101, 106, 94, 197, 
  // 2030
  101, 111, 94, 198, 
  // 2034
  101, 116, 94, 199, 
  // 2038
  122, 94, 178, 
  // 2041
  127, 94, 179, 
  // 2044
  132, 94, 180, 
  // 2047
  137, 94, 181, 
  // 2050
  142, 94, 182, 
  // 2053
  147, 94, 183, 
  // 2056
  152, 94, 184, 
  // 2059
  157, 94, 185, 
  // 2062
  107, 94, 186, 
  // 2065
  112, 94, 187, 
  // 2068
  117, 94, 188, 
  // 2071
  101, 122, 94, 189, 
  // 2075
  101, 127, 94, 190, 
  // 2079
  101, 132, 94, 191, 
  // 2083
  101, 137, 94, 192, 
  // 2087
  101, 142, 94, 193, 
  // 2091
  101, 147, 94, 194, 
  // 2095
  101, 152, 94, 195, 
  // 2099
  101, 157, 94, 196, 
  // 2103
  101, 107, 94, 197, 
  // 2107
  101, 112, 94, 198, 
  // 2111
  101, 117, 94, 199, 
  // 2115
  123, 94, 178, 
  // 2118
  128, 94, 179, 
  // 2121
  133, 94, 180, 
  // 2124
  138, 94, 181, 
  // 2127
  143, 94, 182, 
  // 2130
  148, 94, 183, 
  // 2133
  153, 94, 184, 
  // 2136
  158, 94, 185, 
  // 2139
  108, 94, 186, 
  // 2142
  113, 94, 187, 
  // 2145
  118, 94, 188, 
  // 2148
  101, 123, 94, 189, 
  // 2152
  101, 128, 94, 190, 
  // 2156
  101, 133, 94, 191, 
  // 2160
  101, 138, 94, 192, 
  // 2164
  101, 143, 94, 193, 
  // 2168
  101, 148, 94, 194, 
  // 2172
  101, 153, 94, 195, 
  // 2176
  101, 158, 94, 196, 
  // 2180
  101, 108, 94, 197, 
  // 2184
  101, 113, 94, 198, 
  // 2188
  101, 118, 94, 199, 
  // 2192
  119, 94, 200, 
  // 2195
  124, 94, 201, 
  // 2198
  129, 94, 202, 
  // 2201
  134, 94, 203, 
  // 2204
  139, 94, 204, 
  // 2207
  144, 94, 205, 
  // 2210
  149, 94, 206, 
  // 2213
  154, 94, 207, 
  // 2216
  104, 94, 208, 
  // 2219
  109, 94, 209, 
  // 2222
  114, 94, 210, 
  // 2225
  101, 119, 94, 211, 
  // 2229
  101, 124, 94, 212, 
  // 2233
  101, 129, 94, 213, 
  // 2237
  101, 134, 94, 214, 
  // 2241
  101, 139, 94, 215, 
  // 2245
  101, 144, 94, 216, 
  // 2249
  101, 149, 94, 217, 
  // 2253
  101, 154, 94, 218, 
  // 2257
  101, 104, 94, 219, 
  // 2261
  101, 109, 94, 220, 
  // 2265
  101, 114, 94, 221, 
  // 2269
  120, 94, 200, 
  // 2272
  125, 94, 201, 
  // 2275
  130, 94, 202, 
  // 2278
  135, 94, 203, 
  // 2281
  140, 94, 204, 
  // 2284
  145, 94, 205, 
  // 2287
  150, 94, 206, 
  // 2290
  155, 94, 207, 
  // 2293
  105, 94, 208, 
  // 2296
  110, 94, 209, 
  // 2299
  115, 94, 210, 
  // 2302
  101, 120, 94, 211, 
  // 2306
  101, 125, 94, 212, 
  // 2310
  101, 130, 94, 213, 
  // 2314
  101, 135, 94, 214, 
  // 2318
  101, 140, 94, 215, 
  // 2322
  101, 145, 94, 216, 
  // 2326
  101, 150, 94, 217, 
  // 2330
  101, 155, 94, 218, 
  // 2334
  101, 105, 94, 219, 
  // 2338
  101, 110, 94, 220, 
  // 2342
  101, 115, 94, 221, 
  // 2346
  121, 94, 200, 
  // 2349
  126, 94, 201, 
  // 2352
  131, 94, 202, 
  // 2355
  136, 94, 203, 
  // 2358
  141, 94, 204, 
  // 2361
  146, 94, 205, 
  // 2364
  151, 94, 206, 
  // 2367
  156, 94, 207, 
  // 2370
  106, 94, 208, 
  // 2373
  111, 94, 209, 
  // 2376
  116, 94, 210, 
  // 2379
  101, 121, 94, 211, 
  // 2383
  101, 126, 94, 212, 
  // 2387
  101, 131, 94, 213, 
  // 2391
  101, 136, 94, 214, 
  // 2395
  101, 141, 94, 215, 
  // 2399
  101, 146, 94, 216, 
  // 2403
  101, 151, 94, 217, 
  // 2407
  101, 156, 94, 218, 
  // 2411
  101, 106, 94, 219, 
  // 2415
  101, 111, 94, 220, 
  // 2419
  101, 116, 94, 221, 
  // 2423
  122, 94, 200, 
  // 2426
  127, 94, 201, 
  // 2429
  132, 94, 202, 
  // 2432
  137, 94, 203, 
  // 2435
  142, 94, 204, 
  // 2438
  147, 94, 205, 
  // 2441
  152, 94, 206, 
  // 2444
  157, 94, 207, 
  // 2447
  107, 94, 208, 
  // 2450
  112, 94, 209, 
  // 2453
  117, 94, 210, 
  // 2456
  101, 122, 94, 211, 
  // 2460
  101, 127, 94, 212, 
  // 2464
  101, 132, 94, 213, 
  // 2468
  101, 137, 94, 214, 
  // 2472
  101, 142, 94, 215, 
  // 2476
  101, 147, 94, 216, 
  // 2480
  101, 152, 94, 217, 
  // 2484
  101, 157, 94, 218, 
  // 2488
  101, 107, 94, 219, 
  // 2492
  101, 112, 94, 220, 
  // 2496
  101, 117, 94, 221, 
  // 2500
  123, 94, 200, 
  // 2503
  128, 94, 201, 
  // 2506
  133, 94, 202, 
  // 2509
  138, 94, 203, 
  // 2512
  143, 94, 204, 
  // 2515
  148, 94, 205, 
  // 2518
  153, 94, 206, 
  // 2521
  158, 94, 207, 
  // 2524
  108, 94, 208, 
  // 2527
  113, 94, 209, 
  // 2530
  118, 94, 210, 
  // 2533
  101, 123, 94, 211, 
  // 2537
  101, 128, 94, 212, 
  // 2541
  101, 133, 94, 213, 
  // 2545
  101, 138, 94, 214, 
  // 2549
  101, 143, 94, 215, 
  // 2553
  101, 148, 94, 216, 
  // 2557
  101, 153, 94, 217, 
  // 2561
  101, 158, 94, 218, 
  // 2565
  101, 108, 94, 219, 
  // 2569
  101, 113, 94, 220, 
  // 2573
  101, 118, 94, 221, 
  // 2577
  119, 94, 222, 
  // 2580
  124, 94, 223, 
  // 2583
  129, 94, 224, 
  // 2586
  134, 94, 225, 
  // 2589
  139, 94, 226, 
  // 2592
  144, 94, 227, 
  // 2595
  149, 94, 228, 
  // 2598
  154, 94, 229, 
  // 2601
  104, 94, 230, 
  // 2604
  109, 94, 231, 
  // 2607
  114, 94, 232, 
  // 2610
  101, 119, 94, 233, 
  // 2614
  101, 124, 94, 234, 
  // 2618
  101, 129, 94, 235, 
  // 2622
  101, 134, 94, 236, 
  // 2626
  101, 139, 94, 237, 
  // 2630
  101, 144, 94, 238, 
  // 2634
  101, 149, 94, 239, 
  // 2638
  101, 154, 94, 240, 
  // 2642
  101, 104, 94, 241, 
  // 2646
  101, 109, 94, 242, 
  // 2650
  101, 114, 94, 243, 
  // 2654
  120, 94, 222, 
  // 2657
  125, 94, 223, 
  // 2660
  130, 94, 224, 
  // 2663
  135, 94, 225, 
  // 2666
  140, 94, 226, 
  // 2669
  145, 94, 227, 
  // 2672
  150, 94, 228, 
  // 2675
  155, 94, 229, 
  // 2678
  105, 94, 230, 
  // 2681
  110, 94, 231, 
  // 2684
  115, 94, 232, 
  // 2687
  101, 120, 94, 233, 
  // 2691
  101, 125, 94, 234, 
  // 2695
  101, 130, 94, 235, 
  // 2699
  101, 135, 94, 236, 
  // 2703
  101, 140, 94, 237, 
  // 2707
  101, 145, 94, 238, 
  // 2711
  101, 150, 94, 239, 
  // 2715
  101, 155, 94, 240, 
  // 2719
  101, 105, 94, 241, 
  // 2723
  101, 110, 94, 242, 
  // 2727
  101, 115, 94, 243, 
  // 2731
  121, 94, 222, 
  // 2734
  126, 94, 223, 
  // 2737
  131, 94, 224, 
  // 2740
  136, 94, 225, 
  // 2743
  141, 94, 226, 
  // 2746
  146, 94, 227, 
  // 2749
  151, 94, 228, 
  // 2752
  156, 94, 229, 
  // 2755
  106, 94, 230, 
  // 2758
  111, 94, 231, 
  // 2761
  116, 94, 232, 
  // 2764
  101, 121, 94, 233, 
  // 2768
  101, 126, 94, 234, 
  // 2772
  101, 131, 94, 235, 
  // 2776
  101, 136, 94, 236, 
  // 2780
  101, 141, 94, 237, 
  // 2784
  101, 146, 94, 238, 
  // 2788
  101, 151, 94, 239, 
  // 2792
  101, 156, 94, 240, 
  // 2796
  101, 106, 94, 241, 
  // 2800
  101, 111, 94, 242, 
  // 2804
  101, 116, 94, 243, 
  // 2808
  122, 94, 222, 
  // 2811
  127, 94, 223, 
  // 2814
  132, 94, 224, 
  // 2817
  137, 94, 225, 
  // 2820
  142, 94, 226, 
  // 2823
  147, 94, 227, 
  // 2826
  152, 94, 228, 
  // 2829
  157, 94, 229, 
  // 2832
  107, 94, 230, 
  // 2835
  112, 94, 231, 
  // 2838
  117, 94, 232, 
  // 2841
  101, 122, 94, 233, 
  // 2845
  101, 127, 94, 234, 
  // 2849
  101, 132, 94, 235, 
  // 2853
  101, 137, 94, 236, 
  // 2857
  101, 142, 94, 237, 
  // 2861
  101, 147, 94, 238, 
  // 2865
  101, 152, 94, 239, 
  // 2869
  101, 157, 94, 240, 
  // 2873
  101, 107, 94, 241, 
  // 2877
  101, 112, 94, 242, 
  // 2881
  101, 117, 94, 243, 
  // 2885
  123, 94, 222, 
  // 2888
  128, 94, 223, 
  // 2891
  133, 94, 224, 
  // 2894
  138, 94, 225, 
  // 2897
  143, 94, 226, 
  // 2900
  148, 94, 227, 
  // 2903
  153, 94, 228, 
  // 2906
  158, 94, 229, 
  // 2909
  108, 94, 230, 
  // 2912
  113, 94, 231, 
  // 2915
  118, 94, 232, 
  // 2918
  101, 123, 94, 233, 
  // 2922
  101, 128, 94, 234, 
  // 2926
  101, 133, 94, 235, 
  // 2930
  101, 138, 94, 236, 
  // 2934
  101, 143, 94, 237, 
  // 2938
  101, 148, 94, 238, 
  // 2942
  101, 153, 94, 239, 
  // 2946
  101, 158, 94, 240, 
  // 2950
  101, 108, 94, 241, 
  // 2954
  101, 113, 94, 242, 
  // 2958
  101, 118, 94, 243, 
  // 2962
  119, 94, 244, 
  // 2965
  124, 94, 245, 
  // 2968
  129, 94, 246, 
  // 2971
  134, 94, 247, 
  // 2974
  139, 94, 248, 
  // 2977
  144, 94, 249, 
  // 2980
  149, 94, 250, 
  // 2983
  154, 94, 251, 
  // 2986
  104, 94, 252, 
  // 2989
  109, 94, 253, 
  // 2992
  114, 94, 254, 
  // 2995
  101, 119, 94, 255, 
  // 2999
  101, 124, 94, 256, 
  // 3003
  101, 129, 94, 257, 
  // 3007
  101, 134, 94, 258, 
  // 3011
  101, 139, 94, 259, 
  // 3015
  101, 144, 94, 260, 
  // 3019
  101, 149, 94, 261, 
  // 3023
  101, 154, 94, 262, 
  // 3027
  101, 104, 94, 263, 
  // 3031
  101, 109, 94, 264, 
  // 3035
  101, 114, 94, 265, 
  // 3039
  120, 94, 244, 
  // 3042
  125, 94, 245, 
  // 3045
  130, 94, 246, 
  // 3048
  135, 94, 247, 
  // 3051
  140, 94, 248, 
  // 3054
  145, 94, 249, 
  // 3057
  150, 94, 250, 
  // 3060
  155, 94, 251, 
  // 3063
  105, 94, 252, 
  // 3066
  110, 94, 253, 
  // 3069
  115, 94, 254, 
  // 3072
  101, 120, 94, 255, 
  // 3076
  101, 125, 94, 256, 
  // 3080
  101, 130, 94, 257, 
  // 3084
  101, 135, 94, 258, 
  // 3088
  101, 140, 94, 259, 
  // 3092
  101, 145, 94, 260, 
  // 3096
  101, 150, 94, 261, 
  // 3100
  101, 155, 94, 262, 
  // 3104
  101, 105, 94, 263, 
  // 3108
  101, 110, 94, 264, 
  // 3112
  101, 115, 94, 265, 
  // 3116
  121, 94, 244, 
  // 3119
  126, 94, 245, 
  // 3122
  131, 94, 246, 
  // 3125
  136, 94, 247, 
  // 3128
  141, 94, 248, 
  // 3131
  146, 94, 249, 
  // 3134
  151, 94, 250, 
  // 3137
  156, 94, 251, 
  // 3140
  106, 94, 252, 
  // 3143
  111, 94, 253, 
  // 3146
  116, 94, 254, 
  // 3149
  101, 121, 94, 255, 
  // 3153
  101, 126, 94, 256, 
  // 3157
  101, 131, 94, 257, 
  // 3161
  101, 136, 94, 258, 
  // 3165
  101, 141, 94, 259, 
  // 3169
  101, 146, 94, 260, 
  // 3173
  101, 151, 94, 261, 
  // 3177
  101, 156, 94, 262, 
  // 3181
  101, 106, 94, 263, 
  // 3185
  101, 111, 94, 264, 
  // 3189
  101, 116, 94, 265, 
  // 3193
  122, 94, 244, 
  // 3196
  127, 94, 245, 
  // 3199
  132, 94, 246, 
  // 3202
  137, 94, 247, 
  // 3205
  142, 94, 248, 
  // 3208
  147, 94, 249, 
  // 3211
  152, 94, 250, 
  // 3214
  157, 94, 251, 
  // 3217
  107, 94, 252, 
  // 3220
  112, 94, 253, 
  // 3223
  117, 94, 254, 
  // 3226
  101, 122, 94, 255, 
  // 3230
  101, 127, 94, 256, 
  // 3234
  101, 132, 94, 257, 
  // 3238
  101, 137, 94, 258, 
  // 3242
  101, 142, 94, 259, 
  // 3246
  101, 147, 94, 260, 
  // 3250
  101, 152, 94, 261, 
  // 3254
  101, 157, 94, 262, 
  // 3258
  101, 107, 94, 263, 
  // 3262
  101, 112, 94, 264, 
  // 3266
  101, 117, 94, 265, 
  // 3270
  123, 94, 244, 
  // 3273
  128, 94, 245, 
  // 3276
  133, 94, 246, 
  // 3279
  138, 94, 247, 
  // 3282
  143, 94, 248, 
  // 3285
  148, 94, 249, 
  // 3288
  153, 94, 250, 
  // 3291
  158, 94, 251, 
  // 3294
  108, 94, 252, 
  // 3297
  113, 94, 253, 
  // 3300
  118, 94, 254, 
  // 3303
  101, 123, 94, 255, 
  // 3307
  101, 128, 94, 256, 
  // 3311
  101, 133, 94, 257, 
  // 3315
  101, 138, 94, 258, 
  // 3319
  101, 143, 94, 259, 
  // 3323
  101, 148, 94, 260, 
  // 3327
  101, 153, 94, 261, 
  // 3331
  101, 158, 94, 262, 
  // 3335
  101, 108, 94, 263, 
  // 3339
  101, 113, 94, 264, 
  // 3343
  101, 118, 94, 265, 
  // 3347
  119, 94, 266, 
  // 3350
  124, 94, 267, 
  // 3353
  129, 94, 268, 
  // 3356
  134, 94, 269, 
  // 3359
  139, 94, 270, 
  // 3362
  144, 94, 271, 
  // 3365
  149, 94, 272, 
  // 3368
  154, 94, 273, 
  // 3371
  104, 94, 274, 
  // 3374
  109, 94, 275, 
  // 3377
  114, 94, 276, 
  // 3380
  120, 94, 266, 
  // 3383
  125, 94, 267, 
  // 3386
  130, 94, 268, 
  // 3389
  135, 94, 269, 
  // 3392
  140, 94, 270, 
  // 3395
  145, 94, 271, 
  // 3398
  150, 94, 272, 
  // 3401
  155, 94, 273, 
  // 3404
  105, 94, 274, 
  // 3407
  110, 94, 275, 
  // 3410
  115, 94, 276, 
  // 3413
  121, 94, 266, 
  // 3416
  126, 94, 267, 
  // 3419
  131, 94, 268, 
  // 3422
  136, 94, 269, 
  // 3425
  141, 94, 270, 
  // 3428
  146, 94, 271, 
  // 3431
  151, 94, 272, 
  // 3434
  156, 94, 273, 
  // 3437
  106, 94, 274, 
  // 3440
  111, 94, 275, 
  // 3443
  116, 94, 276, 
  // 3446
  122, 94, 266, 
  // 3449
  127, 94, 267, 
  // 3452
  132, 94, 268, 
  // 3455
  137, 94, 269, 
  // 3458
  142, 94, 270, 
  // 3461
  147, 94, 271, 
  // 3464
  152, 94, 272, 
  // 3467
  157, 94, 273, 
  // 3470
  107, 94, 274, 
  // 3473
  112, 94, 275, 
  // 3476
  117, 94, 276, 
  // 3479
  123, 94, 266, 
  // 3482
  128, 94, 267, 
  // 3485
  133, 94, 268, 
  // 3488
  138, 94, 269, 
  // 3491
  143, 94, 270, 
  // 3494
  148, 94, 271, 
  // 3497
  153, 94, 272, 
  // 3500
  158, 94, 273, 
  // 3503
  108, 94, 274, 
  // 3506
  113, 94, 275, 
  // 3509
  118, 94, 276, 
  // 3512
  27, 94, 60, 
  // 3515
  104, 94, 60, 
  // 3518
  101, 27, 94, 65, 
  // 3522
  101, 24, 94, 65, 
  // 3526
  101, 104, 94, 65, 
  // 3530
  101, 109, 94, 65, 
  // 3534
  105, 94, 60, 
  // 3537
  106, 94, 60, 
  // 3540
  107, 94, 60, 
  // 3543
  108, 94, 60, 
  // 3546
  101, 105, 94, 65, 
  // 3550
  101, 110, 94, 65, 
  // 3554
  101, 106, 94, 65, 
  // 3558
  101, 111, 94, 65, 
  // 3562
  101, 107, 94, 65, 
  // 3566
  101, 112, 94, 65, 
  // 3570
  101, 108, 94, 65, 
  // 3574
  101, 113, 94, 65, 
  // 3578
  27, 94, 61, 
  // 3581
  104, 94, 61, 
  // 3584
  105, 94, 61, 
  // 3587
  106, 94, 61, 
  // 3590
  107, 94, 61, 
  // 3593
  108, 94, 61, 
  // 3596
  101, 27, 94, 66, 
  // 3600
  101, 24, 94, 66, 
  // 3604
  101, 104, 94, 66, 
  // 3608
  101, 109, 94, 66, 
  // 3612
  101, 105, 94, 66, 
  // 3616
  101, 110, 94, 66, 
  // 3620
  101, 106, 94, 66, 
  // 3624
  101, 111, 94, 66, 
  // 3628
  101, 107, 94, 66, 
  // 3632
  101, 112, 94, 66, 
  // 3636
  101, 108, 94, 66, 
  // 3640
  101, 113, 94, 66, 
  // 3644
  27, 94, 62, 
  // 3647
  104, 94, 62, 
  // 3650
  105, 94, 62, 
  // 3653
  106, 94, 62, 
  // 3656
  107, 94, 62, 
  // 3659
  108, 94, 62, 
  // 3662
  101, 27, 94, 67, 
  // 3666
  101, 24, 94, 67, 
  // 3670
  101, 104, 94, 67, 
  // 3674
  101, 109, 94, 67, 
  // 3678
  101, 105, 94, 67, 
  // 3682
  101, 110, 94, 67, 
  // 3686
  101, 106, 94, 67, 
  // 3690
  101, 111, 94, 67, 
  // 3694
  101, 107, 94, 67, 
  // 3698
  101, 112, 94, 67, 
  // 3702
  101, 108, 94, 67, 
  // 3706
  101, 113, 94, 67, 
  // 3710
  27, 94, 59, 
  // 3713
  104, 94, 59, 
  // 3716
  105, 94, 59, 
  // 3719
  106, 94, 59, 
  // 3722
  107, 94, 59, 
  // 3725
  108, 94, 59, 
  // 3728
  101, 27, 94, 64, 
  // 3732
  101, 24, 94, 64, 
  // 3736
  101, 104, 94, 64, 
  // 3740
  101, 109, 94, 64, 
  // 3744
  101, 105, 94, 64, 
  // 3748
  101, 110, 94, 64, 
  // 3752
  101, 106, 94, 64, 
  // 3756
  101, 111, 94, 64, 
  // 3760
  101, 107, 94, 64, 
  // 3764
  101, 112, 94, 64, 
  // 3768
  101, 108, 94, 64, 
  // 3772
  101, 113, 94, 64, 
  // 3776
  27, 94, 58, 
  // 3779
  104, 94, 58, 
  // 3782
  105, 94, 58, 
  // 3785
  106, 94, 58, 
  // 3788
  107, 94, 58, 
  // 3791
  108, 94, 58, 
  // 3794
  101, 84, 
  // 3796
  101, 84, 86, 
  // 3799
  84, 277, 
  // 3801
  84, 279, 
  // 3803
  25, 280, 282, 94, 25, 
  // 3808
  25, 283, 284, 94, 25, 
  // 3813
  25, 280, 282, 94, 94, 25, 
  // 3819
  25, 283, 284, 94, 94, 25, 
  // 3825
  101, 282, 94, 
  // 3828
  101, 80, 285, 
  // 3831
  101, 80, 286, 
  // 3834
  80, 288, 80, 
  // 3837
  80, 288, 
  // 3839
  80, 288, 80, 80, 
  // 3843
  97, 290, 97, 
  // 3846
  97, 290, 
  // 3848
  97, 290, 97, 97, 
  // 3852
  80, 291, 80, 
  // 3855
  80, 291, 
  // 3857
  80, 291, 80, 80, 
  // 3861
  97, 292, 97, 
  // 3864
  97, 292, 
  // 3866
  97, 292, 97, 97, 
  // 3870
  83, 294, 83, 
  // 3873
  83, 294, 
  // 3875
  83, 294, 83, 83, 
  // 3879
  99, 296, 99, 
  // 3882
  99, 296, 
  // 3884
  99, 296, 99, 99, 
  // 3888
  83, 297, 83, 
  // 3891
  83, 297, 
  // 3893
  83, 297, 83, 83, 
  // 3897
  99, 298, 99, 
  // 3900
  99, 298, 
  // 3902
  99, 298, 99, 99, 
  // 3906
  27, 300, 27, 
  // 3909
  27, 301, 27, 
  // 3912
  80, 302, 80, 
  // 3915
  80, 302, 
  // 3917
  80, 302, 80, 80, 
  // 3921
  97, 303, 97, 
  // 3924
  97, 303, 
  // 3926
  97, 303, 97, 97, 
  // 3930
  101, 312, 80, 
  // 3933
  101, 312, 80, 85, 
  // 3937
  101, 312, 80, 85, 86, 
  // 3942
  19, 312, 259, 80, 
  // 3946
  19, 312, 259, 80, 85, 85, 
  // 3952
  19, 312, 259, 80, 85, 85, 86, 
  // 3959
  101, 84, 85, 86, 
  // 3963
  80, 312, 
  // 3965
  80, 312, 85, 
  // 3968
  80, 312, 85, 86, 
  // 3972
  80, 312, 80, 
  // 3975
  80, 312, 80, 85, 
  // 3979
  80, 312, 80, 85, 86, 
  // 3984
  101, 313, 97, 
  // 3987
  101, 313, 97, 85, 
  // 3991
  101, 313, 97, 85, 86, 
  // 3996
  97, 313, 
  // 3998
  97, 313, 85, 
  // 4001
  97, 313, 85, 86, 
  // 4005
  97, 313, 97, 
  // 4008
  97, 313, 97, 85, 
  // 4012
  97, 313, 97, 85, 86, 
  // 4017
  19, 313, 260, 97, 
  // 4021
  19, 313, 260, 97, 85, 85, 
  // 4027
  19, 313, 260, 97, 85, 85, 86, 
  // 4034
  101, 314, 83, 
  // 4037
  101, 314, 83, 85, 
  // 4041
  101, 314, 83, 85, 86, 
  // 4046
  83, 314, 
  // 4048
  83, 314, 85, 
  // 4051
  83, 314, 85, 86, 
  // 4055
  83, 314, 83, 
  // 4058
  83, 314, 83, 85, 
  // 4062
  83, 314, 83, 85, 86, 
  // 4067
  19, 314, 261, 83, 
  // 4071
  19, 314, 261, 83, 85, 85, 
  // 4077
  19, 314, 261, 83, 85, 85, 86, 
  // 4084
  101, 315, 99, 
  // 4087
  101, 315, 99, 85, 
  // 4091
  101, 315, 99, 85, 86, 
  // 4096
  99, 315, 
  // 4098
  99, 315, 85, 
  // 4101
  99, 315, 85, 86, 
  // 4105
  99, 315, 99, 
  // 4108
  99, 315, 99, 85, 
  // 4112
  99, 315, 99, 85, 86, 
  // 4117
  19, 315, 262, 99, 
  // 4121
  19, 315, 262, 99, 85, 85, 
  // 4127
  19, 315, 262, 99, 85, 85, 86, 
  // 4134
  101, 316, 27, 
  // 4137
  101, 316, 27, 85, 
  // 4141
  101, 316, 27, 85, 86, 
  // 4146
  27, 316, 
  // 4148
  27, 316, 85, 
  // 4151
  27, 316, 85, 86, 
  // 4155
  27, 316, 27, 
  // 4158
  27, 316, 27, 85, 
  // 4162
  27, 316, 27, 85, 86, 
  // 4167
  19, 316, 263, 27, 
  // 4171
  19, 316, 263, 27, 85, 85, 
  // 4177
  19, 316, 263, 27, 85, 85, 86, 
  // 4184
  101, 317, 24, 
  // 4187
  101, 317, 24, 85, 
  // 4191
  101, 317, 24, 85, 86, 
  // 4196
  24, 317, 
  // 4198
  24, 317, 85, 
  // 4201
  24, 317, 85, 86, 
  // 4205
  24, 317, 24, 
  // 4208
  24, 317, 24, 85, 
  // 4212
  24, 317, 24, 85, 86, 
  // 4217
  19, 317, 264, 24, 
  // 4221
  19, 317, 264, 24, 85, 85, 
  // 4227
  19, 317, 264, 24, 85, 85, 86, 
  // 4234
  98, 318, 88, 
  // 4237
  98, 318, 88, 85, 
  // 4241
  98, 318, 88, 85, 86, 
  // 4246
  101, 319, 
  // 4248
  101, 319, 85, 
  // 4251
  101, 319, 85, 86, 
  // 4255
  19, 319, 
  // 4257
  19, 319, 85, 
  // 4260
  19, 319, 85, 86, 
  // 4264
  101, 320, 80, 
  // 4267
  101, 320, 80, 85, 
  // 4271
  101, 320, 80, 85, 86, 
  // 4276
  80, 320, 
  // 4278
  80, 320, 85, 
  // 4281
  80, 320, 85, 86, 
  // 4285
  80, 320, 80, 
  // 4288
  80, 320, 80, 85, 
  // 4292
  80, 320, 80, 85, 86, 
  // 4297
  19, 320, 193, 80, 
  // 4301
  19, 320, 193, 80, 85, 85, 
  // 4307
  19, 320, 193, 80, 85, 85, 86, 
  // 4314
  19, 320, 215, 80, 
  // 4318
  19, 320, 215, 80, 85, 85, 
  // 4324
  19, 320, 215, 80, 85, 85, 86, 
  // 4331
  19, 320, 237, 80, 
  // 4335
  19, 320, 237, 80, 85, 85, 
  // 4341
  19, 320, 237, 80, 85, 85, 86, 
  // 4348
  101, 321, 97, 
  // 4351
  101, 321, 97, 85, 
  // 4355
  101, 321, 97, 85, 86, 
  // 4360
  97, 321, 
  // 4362
  97, 321, 85, 
  // 4365
  97, 321, 85, 86, 
  // 4369
  97, 321, 97, 
  // 4372
  97, 321, 97, 85, 
  // 4376
  97, 321, 97, 85, 86, 
  // 4381
  19, 321, 194, 97, 
  // 4385
  19, 321, 194, 97, 85, 85, 
  // 4391
  19, 321, 194, 97, 85, 85, 86, 
  // 4398
  19, 321, 216, 97, 
  // 4402
  19, 321, 216, 97, 85, 85, 
  // 4408
  19, 321, 216, 97, 85, 85, 86, 
  // 4415
  19, 321, 238, 97, 
  // 4419
  19, 321, 238, 97, 85, 85, 
  // 4425
  19, 321, 238, 97, 85, 85, 86, 
  // 4432
  101, 322, 83, 
  // 4435
  101, 322, 83, 85, 
  // 4439
  101, 322, 83, 85, 86, 
  // 4444
  83, 322, 
  // 4446
  83, 322, 85, 
  // 4449
  83, 322, 85, 86, 
  // 4453
  83, 322, 83, 
  // 4456
  83, 322, 83, 85, 
  // 4460
  83, 322, 83, 85, 86, 
  // 4465
  19, 322, 195, 83, 
  // 4469
  19, 322, 195, 83, 85, 85, 
  // 4475
  19, 322, 195, 83, 85, 85, 86, 
  // 4482
  19, 322, 217, 83, 
  // 4486
  19, 322, 217, 83, 85, 85, 
  // 4492
  19, 322, 217, 83, 85, 85, 86, 
  // 4499
  19, 322, 239, 83, 
  // 4503
  19, 322, 239, 83, 85, 85, 
  // 4509
  19, 322, 239, 83, 85, 85, 86, 
  // 4516
  101, 323, 99, 
  // 4519
  101, 323, 99, 85, 
  // 4523
  101, 323, 99, 85, 86, 
  // 4528
  99, 323, 
  // 4530
  99, 323, 85, 
  // 4533
  99, 323, 85, 86, 
  // 4537
  99, 323, 99, 
  // 4540
  99, 323, 99, 85, 
  // 4544
  99, 323, 99, 85, 86, 
  // 4549
  19, 323, 196, 99, 
  // 4553
  19, 323, 196, 99, 85, 85, 
  // 4559
  19, 323, 196, 99, 85, 85, 86, 
  // 4566
  19, 323, 218, 99, 
  // 4570
  19, 323, 218, 99, 85, 85, 
  // 4576
  19, 323, 218, 99, 85, 85, 86, 
  // 4583
  19, 323, 240, 99, 
  // 4587
  19, 323, 240, 99, 85, 85, 
  // 4593
  19, 323, 240, 99, 85, 85, 86, 
  // 4600
  101, 324, 27, 
  // 4603
  101, 324, 27, 85, 
  // 4607
  101, 324, 27, 85, 86, 
  // 4612
  27, 324, 
  // 4614
  27, 324, 85, 
  // 4617
  27, 324, 85, 86, 
  // 4621
  27, 324, 27, 
  // 4624
  27, 324, 27, 85, 
  // 4628
  27, 324, 27, 85, 86, 
  // 4633
  19, 324, 197, 27, 
  // 4637
  19, 324, 197, 27, 85, 85, 
  // 4643
  19, 324, 197, 27, 85, 85, 86, 
  // 4650
  19, 324, 219, 27, 
  // 4654
  19, 324, 219, 27, 85, 85, 
  // 4660
  19, 324, 219, 27, 85, 85, 86, 
  // 4667
  19, 324, 241, 27, 
  // 4671
  19, 324, 241, 27, 85, 85, 
  // 4677
  19, 324, 241, 27, 85, 85, 86, 
  // 4684
  101, 325, 24, 
  // 4687
  101, 325, 24, 85, 
  // 4691
  101, 325, 24, 85, 86, 
  // 4696
  24, 325, 
  // 4698
  24, 325, 85, 
  // 4701
  24, 325, 85, 86, 
  // 4705
  24, 325, 24, 
  // 4708
  24, 325, 24, 85, 
  // 4712
  24, 325, 24, 85, 86, 
  // 4717
  19, 325, 198, 24, 
  // 4721
  19, 325, 198, 24, 85, 85, 
  // 4727
  19, 325, 198, 24, 85, 85, 86, 
  // 4734
  19, 325, 220, 24, 
  // 4738
  19, 325, 220, 24, 85, 85, 
  // 4744
  19, 325, 220, 24, 85, 85, 86, 
  // 4751
  19, 325, 242, 24, 
  // 4755
  19, 325, 242, 24, 85, 85, 
  // 4761
  19, 325, 242, 24, 85, 85, 86, 
  // 4768
  98, 326, 88, 
  // 4771
  98, 326, 88, 85, 
  // 4775
  98, 326, 88, 85, 86, 
  // 4780
  101, 327, 
  // 4782
  101, 327, 85, 
  // 4785
  101, 327, 85, 86, 
  // 4789
  19, 327, 
  // 4791
  19, 327, 85, 
  // 4794
  19, 327, 85, 86, 
  // 4798
  101, 328, 80, 
  // 4801
  101, 328, 80, 85, 
  // 4805
  101, 328, 80, 85, 86, 
  // 4810
  80, 328, 
  // 4812
  80, 328, 85, 
  // 4815
  80, 328, 85, 86, 
  // 4819
  80, 328, 80, 
  // 4822
  80, 328, 80, 85, 
  // 4826
  80, 328, 80, 85, 86, 
  // 4831
  19, 328, 193, 80, 
  // 4835
  19, 328, 193, 80, 85, 85, 
  // 4841
  19, 328, 193, 80, 85, 85, 86, 
  // 4848
  19, 328, 215, 80, 
  // 4852
  19, 328, 215, 80, 85, 85, 
  // 4858
  19, 328, 215, 80, 85, 85, 86, 
  // 4865
  19, 328, 237, 80, 
  // 4869
  19, 328, 237, 80, 85, 85, 
  // 4875
  19, 328, 237, 80, 85, 85, 86, 
  // 4882
  101, 329, 97, 
  // 4885
  101, 329, 97, 85, 
  // 4889
  101, 329, 97, 85, 86, 
  // 4894
  97, 329, 
  // 4896
  97, 329, 85, 
  // 4899
  97, 329, 85, 86, 
  // 4903
  97, 329, 97, 
  // 4906
  97, 329, 97, 85, 
  // 4910
  97, 329, 97, 85, 86, 
  // 4915
  19, 329, 194, 97, 
  // 4919
  19, 329, 194, 97, 85, 85, 
  // 4925
  19, 329, 194, 97, 85, 85, 86, 
  // 4932
  19, 329, 216, 97, 
  // 4936
  19, 329, 216, 97, 85, 85, 
  // 4942
  19, 329, 216, 97, 85, 85, 86, 
  // 4949
  19, 329, 238, 97, 
  // 4953
  19, 329, 238, 97, 85, 85, 
  // 4959
  19, 329, 238, 97, 85, 85, 86, 
  // 4966
  101, 330, 83, 
  // 4969
  101, 330, 83, 85, 
  // 4973
  101, 330, 83, 85, 86, 
  // 4978
  83, 330, 
  // 4980
  83, 330, 85, 
  // 4983
  83, 330, 85, 86, 
  // 4987
  83, 330, 83, 
  // 4990
  83, 330, 83, 85, 
  // 4994
  83, 330, 83, 85, 86, 
  // 4999
  19, 330, 195, 83, 
  // 5003
  19, 330, 195, 83, 85, 85, 
  // 5009
  19, 330, 195, 83, 85, 85, 86, 
  // 5016
  19, 330, 217, 83, 
  // 5020
  19, 330, 217, 83, 85, 85, 
  // 5026
  19, 330, 217, 83, 85, 85, 86, 
  // 5033
  19, 330, 239, 83, 
  // 5037
  19, 330, 239, 83, 85, 85, 
  // 5043
  19, 330, 239, 83, 85, 85, 86, 
  // 5050
  101, 331, 99, 
  // 5053
  101, 331, 99, 85, 
  // 5057
  101, 331, 99, 85, 86, 
  // 5062
  99, 331, 
  // 5064
  99, 331, 85, 
  // 5067
  99, 331, 85, 86, 
  // 5071
  99, 331, 99, 
  // 5074
  99, 331, 99, 85, 
  // 5078
  99, 331, 99, 85, 86, 
  // 5083
  19, 331, 196, 99, 
  // 5087
  19, 331, 196, 99, 85, 85, 
  // 5093
  19, 331, 196, 99, 85, 85, 86, 
  // 5100
  19, 331, 218, 99, 
  // 5104
  19, 331, 218, 99, 85, 85, 
  // 5110
  19, 331, 218, 99, 85, 85, 86, 
  // 5117
  19, 331, 240, 99, 
  // 5121
  19, 331, 240, 99, 85, 85, 
  // 5127
  19, 331, 240, 99, 85, 85, 86, 
  // 5134
  101, 332, 27, 
  // 5137
  101, 332, 27, 85, 
  // 5141
  101, 332, 27, 85, 86, 
  // 5146
  27, 332, 
  // 5148
  27, 332, 85, 
  // 5151
  27, 332, 85, 86, 
  // 5155
  27, 332, 27, 
  // 5158
  27, 332, 27, 85, 
  // 5162
  27, 332, 27, 85, 86, 
  // 5167
  19, 332, 197, 27, 
  // 5171
  19, 332, 197, 27, 85, 85, 
  // 5177
  19, 332, 197, 27, 85, 85, 86, 
  // 5184
  19, 332, 219, 27, 
  // 5188
  19, 332, 219, 27, 85, 85, 
  // 5194
  19, 332, 219, 27, 85, 85, 86, 
  // 5201
  19, 332, 241, 27, 
  // 5205
  19, 332, 241, 27, 85, 85, 
  // 5211
  19, 332, 241, 27, 85, 85, 86, 
  // 5218
  101, 333, 24, 
  // 5221
  101, 333, 24, 85, 
  // 5225
  101, 333, 24, 85, 86, 
  // 5230
  24, 333, 
  // 5232
  24, 333, 85, 
  // 5235
  24, 333, 85, 86, 
  // 5239
  24, 333, 24, 
  // 5242
  24, 333, 24, 85, 
  // 5246
  24, 333, 24, 85, 86, 
  // 5251
  19, 333, 198, 24, 
  // 5255
  19, 333, 198, 24, 85, 85, 
  // 5261
  19, 333, 198, 24, 85, 85, 86, 
  // 5268
  19, 333, 220, 24, 
  // 5272
  19, 333, 220, 24, 85, 85, 
  // 5278
  19, 333, 220, 24, 85, 85, 86, 
  // 5285
  19, 333, 242, 24, 
  // 5289
  19, 333, 242, 24, 85, 85, 
  // 5295
  19, 333, 242, 24, 85, 85, 86, 
  // 5302
  98, 334, 88, 
  // 5305
  98, 334, 88, 85, 
  // 5309
  98, 334, 88, 85, 86, 
  // 5314
  101, 335, 
  // 5316
  101, 335, 85, 
  // 5319
  101, 335, 85, 86, 
  // 5323
  19, 335, 
  // 5325
  19, 335, 85, 
  // 5328
  19, 335, 85, 86, 
  // 5332
  101, 337, 10, 
  // 5335
  101, 337, 10, 85, 
  // 5339
  101, 337, 10, 85, 86, 
  // 5344
  57, 337, 
  // 5346
  57, 337, 85, 
  // 5349
  57, 337, 85, 86, 
  // 5353
  57, 337, 57, 
  // 5356
  57, 337, 57, 85, 
  // 5360
  57, 337, 57, 85, 86, 
  // 5365
  101, 338, 10, 
  // 5368
  101, 338, 10, 85, 
  // 5372
  101, 338, 10, 85, 86, 
  // 5377
  57, 338, 
  // 5379
  57, 338, 85, 
  // 5382
  57, 338, 85, 86, 
  // 5386
  57, 338, 57, 
  // 5389
  57, 338, 57, 85, 
  // 5393
  57, 338, 57, 85, 86, 
  // 5398
  101, 339, 10, 
  // 5401
  101, 339, 10, 85, 
  // 5405
  101, 339, 10, 85, 86, 
  // 5410
  57, 339, 
  // 5412
  57, 339, 85, 
  // 5415
  57, 339, 85, 86, 
  // 5419
  57, 339, 57, 
  // 5422
  57, 339, 57, 85, 
  // 5426
  57, 339, 57, 85, 86, 
  // 5431
  119, 119, 124, 
  // 5434
  124, 124, 124, 
  // 5437
  129, 129, 134, 
  // 5440
  134, 134, 134, 
  // 5443
  139, 139, 144, 
  // 5446
  144, 144, 144, 
  // 5449
  149, 149, 154, 
  // 5452
  154, 154, 154, 
  // 5455
  104, 104, 144, 
  // 5458
  109, 109, 154, 
  // 5461
  114, 114, 134, 
  // 5464
  119, 121, 124, 
  // 5467
  124, 126, 124, 
  // 5470
  129, 131, 134, 
  // 5473
  134, 136, 134, 
  // 5476
  139, 141, 144, 
  // 5479
  144, 146, 144, 
  // 5482
  149, 151, 154, 
  // 5485
  154, 156, 154, 
  // 5488
  104, 106, 144, 
  // 5491
  109, 111, 154, 
  // 5494
  114, 116, 134, 
  // 5497
  119, 122, 124, 
  // 5500
  124, 127, 124, 
  // 5503
  129, 132, 134, 
  // 5506
  134, 137, 134, 
  // 5509
  139, 142, 144, 
  // 5512
  144, 147, 144, 
  // 5515
  149, 152, 154, 
  // 5518
  154, 157, 154, 
  // 5521
  104, 107, 144, 
  // 5524
  109, 112, 154, 
  // 5527
  114, 117, 134, 
  // 5530
  119, 123, 124, 
  // 5533
  124, 128, 124, 
  // 5536
  129, 133, 134, 
  // 5539
  134, 138, 134, 
  // 5542
  139, 143, 144, 
  // 5545
  144, 148, 144, 
  // 5548
  149, 153, 154, 
  // 5551
  154, 158, 154, 
  // 5554
  104, 108, 144, 
  // 5557
  109, 113, 154, 
  // 5560
  114, 118, 134, 
  // 5563
  121, 119, 126, 
  // 5566
  126, 124, 126, 
  // 5569
  131, 129, 136, 
  // 5572
  136, 134, 136, 
  // 5575
  141, 139, 146, 
  // 5578
  146, 144, 146, 
  // 5581
  151, 149, 156, 
  // 5584
  156, 154, 156, 
  // 5587
  106, 104, 146, 
  // 5590
  111, 109, 156, 
  // 5593
  116, 114, 136, 
  // 5596
  121, 121, 126, 
  // 5599
  126, 126, 126, 
  // 5602
  131, 131, 136, 
  // 5605
  136, 136, 136, 
  // 5608
  141, 141, 146, 
  // 5611
  146, 146, 146, 
  // 5614
  151, 151, 156, 
  // 5617
  156, 156, 156, 
  // 5620
  106, 106, 146, 
  // 5623
  111, 111, 156, 
  // 5626
  116, 116, 136, 
  // 5629
  121, 122, 126, 
  // 5632
  126, 127, 126, 
  // 5635
  131, 132, 136, 
  // 5638
  136, 137, 136, 
  // 5641
  141, 142, 146, 
  // 5644
  146, 147, 146, 
  // 5647
  151, 152, 156, 
  // 5650
  156, 157, 156, 
  // 5653
  106, 107, 146, 
  // 5656
  111, 112, 156, 
  // 5659
  116, 117, 136, 
  // 5662
  121, 123, 126, 
  // 5665
  126, 128, 126, 
  // 5668
  131, 133, 136, 
  // 5671
  136, 138, 136, 
  // 5674
  141, 143, 146, 
  // 5677
  146, 148, 146, 
  // 5680
  151, 153, 156, 
  // 5683
  156, 158, 156, 
  // 5686
  106, 108, 146, 
  // 5689
  111, 113, 156, 
  // 5692
  116, 118, 136, 
  // 5695
  122, 119, 127, 
  // 5698
  127, 124, 127, 
  // 5701
  132, 129, 137, 
  // 5704
  137, 134, 137, 
  // 5707
  142, 139, 147, 
  // 5710
  147, 144, 147, 
  // 5713
  152, 149, 157, 
  // 5716
  157, 154, 157, 
  // 5719
  107, 104, 147, 
  // 5722
  112, 109, 157, 
  // 5725
  117, 114, 137, 
  // 5728
  122, 121, 127, 
  // 5731
  127, 126, 127, 
  // 5734
  132, 131, 137, 
  // 5737
  137, 136, 137, 
  // 5740
  142, 141, 147, 
  // 5743
  147, 146, 147, 
  // 5746
  152, 151, 157, 
  // 5749
  157, 156, 157, 
  // 5752
  107, 106, 147, 
  // 5755
  112, 111, 157, 
  // 5758
  117, 116, 137, 
  // 5761
  122, 122, 127, 
  // 5764
  127, 127, 127, 
  // 5767
  132, 132, 137, 
  // 5770
  137, 137, 137, 
  // 5773
  142, 142, 147, 
  // 5776
  147, 147, 147, 
  // 5779
  152, 152, 157, 
  // 5782
  157, 157, 157, 
  // 5785
  107, 107, 147, 
  // 5788
  112, 112, 157, 
  // 5791
  117, 117, 137, 
  // 5794
  122, 123, 127, 
  // 5797
  127, 128, 127, 
  // 5800
  132, 133, 137, 
  // 5803
  137, 138, 137, 
  // 5806
  142, 143, 147, 
  // 5809
  147, 148, 147, 
  // 5812
  152, 153, 157, 
  // 5815
  157, 158, 157, 
  // 5818
  107, 108, 147, 
  // 5821
  112, 113, 157, 
  // 5824
  117, 118, 137, 
  // 5827
  123, 119, 128, 
  // 5830
  128, 124, 128, 
  // 5833
  133, 129, 138, 
  // 5836
  138, 134, 138, 
  // 5839
  143, 139, 148, 
  // 5842
  148, 144, 148, 
  // 5845
  153, 149, 158, 
  // 5848
  158, 154, 158, 
  // 5851
  108, 104, 148, 
  // 5854
  113, 109, 158, 
  // 5857
  118, 114, 138, 
  // 5860
  123, 121, 128, 
  // 5863
  128, 126, 128, 
  // 5866
  133, 131, 138, 
  // 5869
  138, 136, 138, 
  // 5872
  143, 141, 148, 
  // 5875
  148, 146, 148, 
  // 5878
  153, 151, 158, 
  // 5881
  158, 156, 158, 
  // 5884
  108, 106, 148, 
  // 5887
  113, 111, 158, 
  // 5890
  118, 116, 138, 
  // 5893
  123, 122, 128, 
  // 5896
  128, 127, 128, 
  // 5899
  133, 132, 138, 
  // 5902
  138, 137, 138, 
  // 5905
  143, 142, 148, 
  // 5908
  148, 147, 148, 
  // 5911
  153, 152, 158, 
  // 5914
  158, 157, 158, 
  // 5917
  108, 107, 148, 
  // 5920
  113, 112, 158, 
  // 5923
  118, 117, 138, 
  // 5926
  123, 123, 128, 
  // 5929
  128, 128, 128, 
  // 5932
  133, 133, 138, 
  // 5935
  138, 138, 138, 
  // 5938
  143, 143, 148, 
  // 5941
  148, 148, 148, 
  // 5944
  153, 153, 158, 
  // 5947
  158, 158, 158, 
  // 5950
  108, 108, 148, 
  // 5953
  113, 113, 158, 
  // 5956
  118, 118, 138, 
  // 5959
  119, 119, 119, 124, 
  // 5963
  124, 124, 124, 124, 
  // 5967
  129, 129, 129, 134, 
  // 5971
  134, 134, 134, 134, 
  // 5975
  139, 139, 139, 144, 
  // 5979
  144, 144, 144, 144, 
  // 5983
  149, 149, 149, 154, 
  // 5987
  154, 154, 154, 154, 
  // 5991
  104, 104, 104, 144, 
  // 5995
  109, 109, 109, 154, 
  // 5999
  114, 114, 114, 134, 
  // 6003
  119, 121, 121, 124, 
  // 6007
  124, 126, 126, 124, 
  // 6011
  129, 131, 131, 134, 
  // 6015
  134, 136, 136, 134, 
  // 6019
  139, 141, 141, 144, 
  // 6023
  144, 146, 146, 144, 
  // 6027
  149, 151, 151, 154, 
  // 6031
  154, 156, 156, 154, 
  // 6035
  104, 106, 106, 144, 
  // 6039
  109, 111, 111, 154, 
  // 6043
  114, 116, 116, 134, 
  // 6047
  119, 122, 122, 124, 
  // 6051
  124, 127, 127, 124, 
  // 6055
  129, 132, 132, 134, 
  // 6059
  134, 137, 137, 134, 
  // 6063
  139, 142, 142, 144, 
  // 6067
  144, 147, 147, 144, 
  // 6071
  149, 152, 152, 154, 
  // 6075
  154, 157, 157, 154, 
  // 6079
  104, 107, 107, 144, 
  // 6083
  109, 112, 112, 154, 
  // 6087
  114, 117, 117, 134, 
  // 6091
  119, 123, 123, 124, 
  // 6095
  124, 128, 128, 124, 
  // 6099
  129, 133, 133, 134, 
  // 6103
  134, 138, 138, 134, 
  // 6107
  139, 143, 143, 144, 
  // 6111
  144, 148, 148, 144, 
  // 6115
  149, 153, 153, 154, 
  // 6119
  154, 158, 158, 154, 
  // 6123
  104, 108, 108, 144, 
  // 6127
  109, 113, 113, 154, 
  // 6131
  114, 118, 118, 134, 
  // 6135
  121, 119, 119, 126, 
  // 6139
  126, 124, 124, 126, 
  // 6143
  131, 129, 129, 136, 
  // 6147
  136, 134, 134, 136, 
  // 6151
  141, 139, 139, 146, 
  // 6155
  146, 144, 144, 146, 
  // 6159
  151, 149, 149, 156, 
  // 6163
  156, 154, 154, 156, 
  // 6167
  106, 104, 104, 146, 
  // 6171
  111, 109, 109, 156, 
  // 6175
  116, 114, 114, 136, 
  // 6179
  121, 121, 121, 126, 
  // 6183
  126, 126, 126, 126, 
  // 6187
  131, 131, 131, 136, 
  // 6191
  136, 136, 136, 136, 
  // 6195
  141, 141, 141, 146, 
  // 6199
  146, 146, 146, 146, 
  // 6203
  151, 151, 151, 156, 
  // 6207
  156, 156, 156, 156, 
  // 6211
  106, 106, 106, 146, 
  // 6215
  111, 111, 111, 156, 
  // 6219
  116, 116, 116, 136, 
  // 6223
  121, 122, 122, 126, 
  // 6227
  126, 127, 127, 126, 
  // 6231
  131, 132, 132, 136, 
  // 6235
  136, 137, 137, 136, 
  // 6239
  141, 142, 142, 146, 
  // 6243
  146, 147, 147, 146, 
  // 6247
  151, 152, 152, 156, 
  // 6251
  156, 157, 157, 156, 
  // 6255
  106, 107, 107, 146, 
  // 6259
  111, 112, 112, 156, 
  // 6263
  116, 117, 117, 136, 
  // 6267
  121, 123, 123, 126, 
  // 6271
  126, 128, 128, 126, 
  // 6275
  131, 133, 133, 136, 
  // 6279
  136, 138, 138, 136, 
  // 6283
  141, 143, 143, 146, 
  // 6287
  146, 148, 148, 146, 
  // 6291
  151, 153, 153, 156, 
  // 6295
  156, 158, 158, 156, 
  // 6299
  106, 108, 108, 146, 
  // 6303
  111, 113, 113, 156, 
  // 6307
  116, 118, 118, 136, 
  // 6311
  122, 119, 119, 127, 
  // 6315
  127, 124, 124, 127, 
  // 6319
  132, 129, 129, 137, 
  // 6323
  137, 134, 134, 137, 
  // 6327
  142, 139, 139, 147, 
  // 6331
  147, 144, 144, 147, 
  // 6335
  152, 149, 149, 157, 
  // 6339
  157, 154, 154, 157, 
  // 6343
  107, 104, 104, 147, 
  // 6347
  112, 109, 109, 157, 
  // 6351
  117, 114, 114, 137, 
  // 6355
  122, 121, 121, 127, 
  // 6359
  127, 126, 126, 127, 
  // 6363
  132, 131, 131, 137, 
  // 6367
  137, 136, 136, 137, 
  // 6371
  142, 141, 141, 147, 
  // 6375
  147, 146, 146, 147, 
  // 6379
  152, 151, 151, 157, 
  // 6383
  157, 156, 156, 157, 
  // 6387
  107, 106, 106, 147, 
  // 6391
  112, 111, 111, 157, 
  // 6395
  117, 116, 116, 137, 
  // 6399
  122, 122, 122, 127, 
  // 6403
  127, 127, 127, 127, 
  // 6407
  132, 132, 132, 137, 
  // 6411
  137, 137, 137, 137, 
  // 6415
  142, 142, 142, 147, 
  // 6419
  147, 147, 147, 147, 
  // 6423
  152, 152, 152, 157, 
  // 6427
  157, 157, 157, 157, 
  // 6431
  107, 107, 107, 147, 
  // 6435
  112, 112, 112, 157, 
  // 6439
  117, 117, 117, 137, 
  // 6443
  122, 123, 123, 127, 
  // 6447
  127, 128, 128, 127, 
  // 6451
  132, 133, 133, 137, 
  // 6455
  137, 138, 138, 137, 
  // 6459
  142, 143, 143, 147, 
  // 6463
  147, 148, 148, 147, 
  // 6467
  152, 153, 153, 157, 
  // 6471
  157, 158, 158, 157, 
  // 6475
  107, 108, 108, 147, 
  // 6479
  112, 113, 113, 157, 
  // 6483
  117, 118, 118, 137, 
  // 6487
  123, 119, 119, 128, 
  // 6491
  128, 124, 124, 128, 
  // 6495
  133, 129, 129, 138, 
  // 6499
  138, 134, 134, 138, 
  // 6503
  143, 139, 139, 148, 
  // 6507
  148, 144, 144, 148, 
  // 6511
  153, 149, 149, 158, 
  // 6515
  158, 154, 154, 158, 
  // 6519
  108, 104, 104, 148, 
  // 6523
  113, 109, 109, 158, 
  // 6527
  118, 114, 114, 138, 
  // 6531
  123, 121, 121, 128, 
  // 6535
  128, 126, 126, 128, 
  // 6539
  133, 131, 131, 138, 
  // 6543
  138, 136, 136, 138, 
  // 6547
  143, 141, 141, 148, 
  // 6551
  148, 146, 146, 148, 
  // 6555
  153, 151, 151, 158, 
  // 6559
  158, 156, 156, 158, 
  // 6563
  108, 106, 106, 148, 
  // 6567
  113, 111, 111, 158, 
  // 6571
  118, 116, 116, 138, 
  // 6575
  123, 122, 122, 128, 
  // 6579
  128, 127, 127, 128, 
  // 6583
  133, 132, 132, 138, 
  // 6587
  138, 137, 137, 138, 
  // 6591
  143, 142, 142, 148, 
  // 6595
  148, 147, 147, 148, 
  // 6599
  153, 152, 152, 158, 
  // 6603
  158, 157, 157, 158, 
  // 6607
  108, 107, 107, 148, 
  // 6611
  113, 112, 112, 158, 
  // 6615
  118, 117, 117, 138, 
  // 6619
  123, 123, 123, 128, 
  // 6623
  128, 128, 128, 128, 
  // 6627
  133, 133, 133, 138, 
  // 6631
  138, 138, 138, 138, 
  // 6635
  143, 143, 143, 148, 
  // 6639
  148, 148, 148, 148, 
  // 6643
  153, 153, 153, 158, 
  // 6647
  158, 158, 158, 158, 
  // 6651
  108, 108, 108, 148, 
  // 6655
  113, 113, 113, 158, 
  // 6659
  118, 118, 118, 138, 
  // 6663
  106, 340, 92, 80, 
  // 6667
  141, 340, 92, 80, 
  // 6671
  146, 340, 92, 80, 
  // 6675
  106, 340, 92, 27, 
  // 6679
  141, 340, 92, 27, 
  // 6683
  146, 340, 92, 27, 
  // 6687
  106, 341, 92, 139, 
  // 6691
  141, 341, 92, 139, 
  // 6695
  146, 341, 92, 139, 
  // 6699
  106, 341, 92, 104, 
  // 6703
  141, 341, 92, 104, 
  // 6707
  146, 341, 92, 104, 
  // 6711
  106, 342, 92, 139, 
  // 6715
  141, 342, 92, 139, 
  // 6719
  146, 342, 92, 139, 
  // 6723
  106, 342, 92, 104, 
  // 6727
  141, 342, 92, 104, 
  // 6731
  146, 342, 92, 104, 
  // 6735
  106, 343, 92, 141, 
  // 6739
  141, 343, 92, 141, 
  // 6743
  146, 343, 92, 141, 
  // 6747
  106, 343, 92, 106, 
  // 6751
  141, 343, 92, 106, 
  // 6755
  146, 343, 92, 106, 
  // 6759
  106, 344, 92, 141, 
  // 6763
  141, 344, 92, 141, 
  // 6767
  146, 344, 92, 141, 
  // 6771
  106, 344, 92, 106, 
  // 6775
  141, 344, 92, 106, 
  // 6779
  146, 344, 92, 106, 
  // 6783
  27, 345, 92, 139, 
  // 6787
  27, 346, 92, 141, 
  // 6791
  27, 345, 92, 104, 
  // 6795
  27, 346, 92, 106, 
  // 6799
  106, 341, 139, 
  // 6802
  141, 341, 139, 
  // 6805
  146, 341, 139, 
  // 6808
  106, 343, 141, 
  // 6811
  141, 343, 141, 
  // 6814
  146, 343, 141, 
  // 6817
  106, 340, 80, 
  // 6820
  141, 340, 80, 
  // 6823
  146, 340, 80, 
  // 6826
  27, 345, 139, 
  // 6829
  27, 346, 141, 
  // 6832
  106, 342, 139, 
  // 6835
  141, 342, 139, 
  // 6838
  146, 342, 139, 
  // 6841
  106, 344, 141, 
  // 6844
  141, 344, 141, 
  // 6847
  146, 344, 141, 
  // 6850
  106, 347, 80, 
  // 6853
  141, 347, 80, 
  // 6856
  146, 347, 80, 
  // 6859
  106, 348, 139, 
  // 6862
  141, 348, 139, 
  // 6865
  146, 348, 139, 
  // 6868
  106, 349, 139, 
  // 6871
  141, 349, 139, 
  // 6874
  146, 349, 139, 
  // 6877
  106, 350, 141, 
  // 6880
  141, 350, 141, 
  // 6883
  146, 350, 141, 
  // 6886
  106, 351, 141, 
  // 6889
  141, 351, 141, 
  // 6892
  146, 351, 141, 
  // 6895
  106, 352, 80, 
  // 6898
  141, 352, 80, 
  // 6901
  146, 352, 80, 
  // 6904
  106, 353, 80, 
  // 6907
  141, 353, 80, 
  // 6910
  146, 353, 80, 
  // 6913
  27, 354, 139, 
  // 6916
  27, 355, 141, 
  // 6919
  101, 356, 139, 106, 
  // 6923
  101, 356, 139, 141, 
  // 6927
  101, 356, 139, 146, 
  // 6931
  101, 357, 141, 106, 
  // 6935
  101, 357, 141, 141, 
  // 6939
  101, 357, 141, 146, 
  // 6943
  101, 358, 80, 106, 
  // 6947
  101, 358, 80, 141, 
  // 6951
  101, 358, 80, 146, 
  // 6955
  101, 360, 139, 106, 
  // 6959
  101, 360, 139, 141, 
  // 6963
  101, 360, 139, 146, 
  // 6967
  101, 361, 141, 106, 
  // 6971
  101, 361, 141, 141, 
  // 6975
  101, 361, 141, 146, 
  // 6979
  101, 362, 139, 27, 
  // 6983
  101, 363, 141, 27, 
  // 6987
  101, 359, 80, 106, 
  // 6991
  101, 359, 80, 141, 
  // 6995
  101, 359, 80, 146, 
  // 6999
  101, 348, 139, 106, 
  // 7003
  101, 348, 139, 141, 
  // 7007
  101, 348, 139, 146, 
  // 7011
  101, 351, 141, 106, 
  // 7015
  101, 351, 141, 141, 
  // 7019
  101, 351, 141, 146, 
  // 7023
  101, 352, 80, 106, 
  // 7027
  101, 352, 80, 141, 
  // 7031
  101, 352, 80, 146, 
  // 7035
  101, 353, 80, 106, 
  // 7039
  101, 353, 80, 141, 
  // 7043
  101, 353, 80, 146, 
  // 7047
  101, 349, 139, 106, 
  // 7051
  101, 349, 139, 141, 
  // 7055
  101, 349, 139, 146, 
  // 7059
  101, 350, 141, 106, 
  // 7063
  101, 350, 141, 141, 
  // 7067
  101, 350, 141, 146, 
  // 7071
  101, 354, 139, 27, 
  // 7075
  101, 355, 141, 27, 
  // 7079
  80, 340, 
  // 7081
  80, 341, 
  // 7083
  80, 343, 
  // 7085
  139, 341, 
  // 7087
  141, 343, 
  // 7089
  94, 342, 
  // 7091
  80, 347, 
  // 7093
  80, 342, 
  // 7095
  80, 344, 
  // 7097
  80, 345, 
  // 7099
  80, 346, 
  // 7101
  139, 344, 
  // 7103
  139, 345, 
  // 7105
  139, 346, 
  // 7107
  94, 344, 
  // 7109
  94, 346, 
  // 7111
  80, 358, 
  // 7113
  80, 359, 
  // 7115
  80, 356, 
  // 7117
  80, 361, 
  // 7119
  80, 360, 
  // 7121
  80, 357, 
  // 7123
  80, 362, 
  // 7125
  80, 363, 
  // 7127
  139, 356, 
  // 7129
  139, 357, 
  // 7131
  139, 362, 
  // 7133
  139, 363, 
  // 7135
  141, 361, 
  // 7137
  94, 360, 
  // 7139
  94, 357, 
  // 7141
  94, 363, 
  // 7143
  80, 352, 
  // 7145
  80, 353, 
  // 7147
  80, 348, 
  // 7149
  80, 350, 
  // 7151
  80, 349, 
  // 7153
  80, 351, 
  // 7155
  80, 354, 
  // 7157
  80, 355, 
  // 7159
  139, 348, 
  // 7161
  139, 351, 
  // 7163
  139, 354, 
  // 7165
  139, 355, 
  // 7167
  141, 350, 
  // 7169
  94, 349, 
  // 7171
  94, 351, 
  // 7173
  94, 355, 
  // 7175
  116, 341, 92, 139, 
  // 7179
  116, 343, 92, 141, 
  // 7183
  116, 340, 92, 80, 
  // 7187
  116, 342, 92, 139, 
  // 7191
  116, 344, 92, 141, 
  // 7195
  116, 341, 92, 104, 
  // 7199
  116, 342, 92, 104, 
  // 7203
  116, 343, 92, 106, 
  // 7207
  116, 344, 92, 106, 
  // 7211
  116, 340, 92, 27, 
  // 7215
  116, 341, 139, 
  // 7218
  116, 343, 141, 
  // 7221
  116, 340, 80, 
  // 7224
  116, 342, 139, 
  // 7227
  116, 344, 141, 
  // 7230
  116, 347, 80, 
  // 7233
  116, 348, 139, 
  // 7236
  116, 349, 139, 
  // 7239
  116, 350, 141, 
  // 7242
  116, 351, 141, 
  // 7245
  116, 352, 80, 
  // 7248
  116, 353, 80, 
  // 7251
  101, 356, 139, 116, 
  // 7255
  101, 357, 141, 116, 
  // 7259
  101, 358, 80, 116, 
  // 7263
  101, 359, 80, 116, 
  // 7267
  101, 360, 139, 116, 
  // 7271
  101, 361, 141, 116, 
  // 7275
  101, 348, 139, 116, 
  // 7279
  101, 351, 141, 116, 
  // 7283
  101, 352, 80, 116, 
  // 7287
  101, 353, 80, 116, 
  // 7291
  101, 349, 139, 116, 
  // 7295
  101, 350, 141, 116, 
  // 7299
  81, 81, 94, 
  // 7302
  81, 81, 94, 94, 
  // 7306
  81, 81, 94, 94, 94, 
  // 7311
  81, 81, 
  // 7313
  19, 90, 
  // 7315
  80, 89, 97, 365, 366, 
  // 7320
  101, 22, 
  // 7322
  22, 
  // 7323
  19, 22, 
  // 7325
  101, 22, 80, 
  // 7328
  101, 22, 23, 367, 
  // 7332
  89, 
  // 7333
  87, 94, 
  // 7335
  87, 94, 94, 
  // 7338
  87, 94, 94, 94, 
  // 7342
  87, 369, 
  // 7344
  87, 369, 369, 
  // 7347
  87, 369, 369, 369, 
  // 7351
  106, 341, 92, 104, 27, 
  // 7356
  106, 341, 92, 104, 104, 104, 
  // 7362
  141, 341, 92, 104, 27, 
  // 7367
  141, 341, 92, 104, 104, 104, 
  // 7373
  146, 341, 92, 104, 27, 
  // 7378
  146, 341, 92, 104, 104, 104, 
  // 7384
  27, 345, 92, 104, 27, 
  // 7389
  27, 345, 92, 104, 104, 104, 
  // 7395
  106, 340, 92, 27, 27, 
  // 7400
  106, 340, 92, 27, 27, 27, 
  // 7406
  141, 340, 92, 27, 27, 
  // 7411
  141, 340, 92, 27, 27, 27, 
  // 7417
  146, 340, 92, 27, 27, 
  // 7422
  146, 340, 92, 27, 27, 27, 
  // 7428
  106, 343, 92, 106, 106, 106, 
  // 7434
  106, 343, 92, 106, 27, 
  // 7439
  141, 343, 92, 106, 106, 106, 
  // 7445
  141, 343, 92, 106, 27, 
  // 7450
  146, 343, 92, 106, 106, 106, 
  // 7456
  146, 343, 92, 106, 27, 
  // 7461
  106, 342, 92, 104, 27, 
  // 7466
  106, 342, 92, 104, 27, 27, 
  // 7472
  141, 342, 92, 104, 27, 
  // 7477
  141, 342, 92, 104, 27, 27, 
  // 7483
  146, 342, 92, 104, 27, 
  // 7488
  146, 342, 92, 104, 27, 27, 
  // 7494
  106, 344, 92, 106, 27, 
  // 7499
  106, 344, 92, 106, 104, 104, 
  // 7505
  141, 344, 92, 106, 27, 
  // 7510
  141, 344, 92, 106, 104, 104, 
  // 7516
  146, 344, 92, 106, 27, 
  // 7521
  146, 344, 92, 106, 104, 104, 
  // 7527
  27, 346, 92, 106, 27, 
  // 7532
  27, 346, 92, 106, 104, 104, 
  // 7538
  106, 348, 92, 104, 27, 
  // 7543
  106, 348, 92, 104, 104, 104, 
  // 7549
  141, 348, 92, 104, 27, 
  // 7554
  141, 348, 92, 104, 104, 104, 
  // 7560
  146, 348, 92, 104, 27, 
  // 7565
  146, 348, 92, 104, 104, 104, 
  // 7571
  27, 354, 92, 104, 27, 
  // 7576
  27, 354, 92, 104, 104, 104, 
  // 7582
  106, 352, 92, 27, 27, 
  // 7587
  106, 352, 92, 27, 27, 27, 
  // 7593
  141, 352, 92, 27, 27, 
  // 7598
  141, 352, 92, 27, 27, 27, 
  // 7604
  146, 352, 92, 27, 27, 
  // 7609
  146, 352, 92, 27, 27, 27, 
  // 7615
  106, 350, 92, 106, 106, 106, 
  // 7621
  106, 350, 92, 106, 27, 
  // 7626
  141, 350, 92, 106, 106, 106, 
  // 7632
  141, 350, 92, 106, 27, 
  // 7637
  146, 350, 92, 106, 106, 106, 
  // 7643
  146, 350, 92, 106, 27, 
  // 7648
  106, 349, 92, 104, 27, 
  // 7653
  106, 349, 92, 104, 27, 27, 
  // 7659
  141, 349, 92, 104, 27, 
  // 7664
  141, 349, 92, 104, 27, 27, 
  // 7670
  146, 349, 92, 104, 27, 
  // 7675
  146, 349, 92, 104, 27, 27, 
  // 7681
  106, 351, 92, 106, 27, 
  // 7686
  106, 351, 92, 106, 104, 104, 
  // 7692
  141, 351, 92, 106, 27, 
  // 7697
  141, 351, 92, 106, 104, 104, 
  // 7703
  146, 351, 92, 106, 27, 
  // 7708
  146, 351, 92, 106, 104, 104, 
  // 7714
  27, 355, 92, 106, 27, 
  // 7719
  27, 355, 92, 106, 104, 104, 
  // 7725
  101, 356, 139, 80, 106, 
  // 7730
  101, 356, 139, 80, 141, 
  // 7735
  101, 356, 139, 80, 146, 
  // 7740
  101, 362, 139, 80, 27, 
  // 7745
  101, 358, 80, 80, 106, 
  // 7750
  101, 358, 80, 80, 141, 
  // 7755
  101, 358, 80, 80, 146, 
  // 7760
  101, 360, 139, 80, 106, 
  // 7765
  101, 360, 139, 80, 141, 
  // 7770
  101, 360, 139, 80, 146, 
  // 7775
  101, 357, 141, 80, 106, 
  // 7780
  101, 357, 141, 80, 141, 
  // 7785
  101, 357, 141, 80, 146, 
  // 7790
  101, 363, 141, 80, 27, 
  // 7795
  101, 361, 141, 80, 106, 
  // 7800
  101, 361, 141, 80, 141, 
  // 7805
  101, 361, 141, 80, 146, 
  // 7810
  101, 348, 139, 80, 106, 
  // 7815
  101, 348, 139, 80, 141, 
  // 7820
  101, 348, 139, 80, 146, 
  // 7825
  101, 354, 139, 80, 27, 
  // 7830
  101, 352, 80, 80, 106, 
  // 7835
  101, 352, 80, 80, 141, 
  // 7840
  101, 352, 80, 80, 146, 
  // 7845
  101, 349, 139, 80, 106, 
  // 7850
  101, 349, 139, 80, 141, 
  // 7855
  101, 349, 139, 80, 146, 
  // 7860
  101, 351, 141, 80, 106, 
  // 7865
  101, 351, 141, 80, 141, 
  // 7870
  101, 351, 141, 80, 146, 
  // 7875
  101, 355, 141, 80, 27, 
  // 7880
  101, 350, 141, 80, 106, 
  // 7885
  101, 350, 141, 80, 141, 
  // 7890
  101, 350, 141, 80, 146, 
  // 7895
  106, 370, 139, 80, 
  // 7899
  141, 370, 139, 80, 
  // 7903
  146, 370, 139, 80, 
  // 7907
  106, 371, 141, 80, 
  // 7911
  141, 371, 141, 80, 
  // 7915
  146, 371, 141, 80, 
  // 7919
  27, 372, 139, 80, 
  // 7923
  27, 373, 141, 80, 
  // 7927
  80, 370, 
  // 7929
  139, 370, 
  // 7931
  94, 371, 
  // 7933
  80, 371, 
  // 7935
  139, 371, 
  // 7937
  80, 372, 
  // 7939
  139, 372, 
  // 7941
  80, 373, 
  // 7943
  139, 373, 
  // 7945
  94, 373, 
  // 7947
  80, 374, 
  // 7949
  139, 374, 
  // 7951
  80, 375, 
  // 7953
  139, 375, 
  // 7955
  80, 376, 
  // 7957
  139, 376, 
  // 7959
  80, 377, 
  // 7961
  139, 377, 
  // 7963
  94, 375, 
  // 7965
  94, 377, 
  // 7967
  106, 378, 139, 80, 
  // 7971
  141, 378, 139, 80, 
  // 7975
  146, 378, 139, 80, 
  // 7979
  106, 379, 141, 80, 
  // 7983
  141, 379, 141, 80, 
  // 7987
  146, 379, 141, 80, 
  // 7991
  27, 380, 139, 80, 
  // 7995
  27, 381, 141, 80, 
  // 7999
  80, 378, 
  // 8001
  139, 378, 
  // 8003
  80, 379, 
  // 8005
  139, 379, 
  // 8007
  80, 380, 
  // 8009
  139, 380, 
  // 8011
  80, 381, 
  // 8013
  139, 381, 
  // 8015
  94, 379, 
  // 8017
  94, 381, 
  // 8019
  81, 81, 97, 
  // 8022
  21, 21, 97, 
  // 8025
  2, 2, 97, 
  // 8028
  21, 21, 
  // 8030
  80, 
  // 8031
  80, 0, 
  // 8033
  1, 1, 97, 
  // 8036
  0, 0, 
  // 8038
  146, 80, 
  // 8040
  80, 146, 
  // 8042
  80, 146, 97, 
  // 8045
  97, 146, 
  // 8047
  146, 
  // 8048
  4, 4, 
  // 8050
  0, 0, 97, 
  // 8053
  4, 4, 97, 
  // 8056
  80, 80, 97, 
  // 8059
  5, 5, 5, 97, 97, 
  // 8064
  91, 91, 97, 97, 
  // 8068
  91, 96, 97, 97, 
  // 8072
  96, 91, 97, 97, 
  // 8076
  96, 96, 97, 97, 
  // 8080
  97, 126, 126, 
  // 8083
  80, 121, 121, 
  // 8086
  80, 126, 121, 
  // 8089
  80, 121, 126, 
  // 8092
  97, 126, 126, 97, 
  // 8096
  80, 121, 121, 80, 
  // 8100
  80, 126, 121, 80, 
  // 8104
  80, 121, 126, 80, 
  // 8108
  97, 97, 97, 
  // 8111
  80, 97, 97, 
  // 8114
  97, 97, 97, 97, 
  // 8118
  80, 97, 97, 80, 
  // 8122
  0, 0, 80, 
  // 8125
  0, 0, 80, 97, 
  // 8129
  99, 
  // 8130
  144, 
  // 8131
  35, 35, 97, 
  // 8134
  40, 40, 97, 
  // 8137
  50, 50, 97, 
  // 8140
  83, 83, 97, 
  // 8143
  99, 99, 97, 
  // 8146
  57, 57, 97, 
  // 8149
  24, 24, 97, 
  // 8152
  35, 35, 35, 97, 
  // 8156
  40, 40, 40, 97, 
  // 8160
  50, 50, 50, 97, 
  // 8164
  83, 83, 83, 97, 
  // 8168
  99, 99, 99, 97, 
  // 8172
  57, 57, 57, 97, 
  // 8176
  24, 24, 24, 97, 
  // 8180
  97, 341, 139, 
  // 8183
  144, 341, 139, 
  // 8186
  147, 341, 139, 
  // 8189
  97, 183, 
  // 8191
  144, 183, 
  // 8193
  146, 183, 
  // 8195
  147, 183, 
  // 8197
  101, 356, 139, 97, 
  // 8201
  101, 356, 139, 144, 
  // 8205
  101, 356, 139, 147, 
  // 8209
  101, 194, 97, 
  // 8212
  101, 194, 144, 
  // 8215
  101, 194, 146, 
  // 8218
  101, 194, 147, 
  // 8221
  97, 348, 139, 
  // 8224
  144, 348, 139, 
  // 8227
  147, 348, 139, 
  // 8230
  101, 348, 139, 97, 
  // 8234
  101, 348, 139, 144, 
  // 8238
  101, 348, 139, 147, 
  // 8242
  45, 45, 97, 
  // 8245
  55, 55, 97, 
  // 8248
  44, 44, 97, 
  // 8251
  54, 54, 97, 
  // 8254
  44, 44, 44, 97, 
  // 8258
  54, 54, 54, 97, 
  // 8262
  100, 341, 139, 
  // 8265
  134, 341, 139, 
  // 8268
  136, 341, 139, 
  // 8271
  137, 341, 139, 
  // 8274
  138, 341, 139, 
  // 8277
  100, 181, 
  // 8279
  134, 181, 
  // 8281
  136, 181, 
  // 8283
  137, 181, 
  // 8285
  138, 181, 
  // 8287
  101, 356, 139, 100, 
  // 8291
  101, 356, 139, 134, 
  // 8295
  101, 356, 139, 136, 
  // 8299
  101, 356, 139, 137, 
  // 8303
  101, 356, 139, 138, 
  // 8307
  101, 192, 100, 
  // 8310
  101, 192, 134, 
  // 8313
  101, 192, 136, 
  // 8316
  101, 192, 137, 
  // 8319
  101, 192, 138, 
  // 8322
  100, 348, 139, 
  // 8325
  134, 348, 139, 
  // 8328
  136, 348, 139, 
  // 8331
  137, 348, 139, 
  // 8334
  138, 348, 139, 
  // 8337
  101, 348, 139, 100, 
  // 8341
  101, 348, 139, 134, 
  // 8345
  101, 348, 139, 136, 
  // 8349
  101, 348, 139, 137, 
  // 8353
  101, 348, 139, 138, 
  // 8357
  29, 29, 97, 
  // 8360
  48, 48, 97, 
  // 8363
  28, 28, 97, 
  // 8366
  47, 47, 97, 
  // 8369
  28, 28, 28, 97, 
  // 8373
  47, 47, 47, 97, 
  // 8377
  95, 341, 139, 
  // 8380
  124, 341, 139, 
  // 8383
  126, 341, 139, 
  // 8386
  127, 341, 139, 
  // 8389
  128, 341, 139, 
  // 8392
  95, 179, 
  // 8394
  124, 179, 
  // 8396
  126, 179, 
  // 8398
  127, 179, 
  // 8400
  128, 179, 
  // 8402
  101, 356, 139, 95, 
  // 8406
  101, 356, 139, 124, 
  // 8410
  101, 356, 139, 126, 
  // 8414
  101, 356, 139, 127, 
  // 8418
  101, 356, 139, 128, 
  // 8422
  101, 190, 95, 
  // 8425
  101, 190, 124, 
  // 8428
  101, 190, 126, 
  // 8431
  101, 190, 127, 
  // 8434
  101, 190, 128, 
  // 8437
  95, 348, 139, 
  // 8440
  124, 348, 139, 
  // 8443
  126, 348, 139, 
  // 8446
  127, 348, 139, 
  // 8449
  128, 348, 139, 
  // 8452
  101, 348, 139, 95, 
  // 8456
  101, 348, 139, 124, 
  // 8460
  101, 348, 139, 126, 
  // 8464
  101, 348, 139, 127, 
  // 8468
  101, 348, 139, 128, 
  // 8472
  99, 341, 139, 
  // 8475
  154, 341, 139, 
  // 8478
  156, 341, 139, 
  // 8481
  157, 341, 139, 
  // 8484
  158, 341, 139, 
  // 8487
  99, 185, 
  // 8489
  154, 185, 
  // 8491
  156, 185, 
  // 8493
  157, 185, 
  // 8495
  101, 356, 139, 99, 
  // 8499
  101, 356, 139, 154, 
  // 8503
  101, 356, 139, 156, 
  // 8507
  101, 356, 139, 157, 
  // 8511
  101, 356, 139, 158, 
  // 8515
  101, 196, 99, 
  // 8518
  101, 196, 154, 
  // 8521
  101, 196, 156, 
  // 8524
  101, 196, 157, 
  // 8527
  99, 348, 139, 
  // 8530
  154, 348, 139, 
  // 8533
  156, 348, 139, 
  // 8536
  157, 348, 139, 
  // 8539
  158, 348, 139, 
  // 8542
  101, 348, 139, 99, 
  // 8546
  101, 348, 139, 154, 
  // 8550
  101, 348, 139, 156, 
  // 8554
  101, 348, 139, 157, 
  // 8558
  101, 348, 139, 158, 
  // 8562
  101, 183, 
  // 8564
  101, 181, 
  // 8566
  101, 179, 
  // 8568
  101, 185, 
  // 8570
  97, 205, 
  // 8572
  144, 205, 
  // 8574
  146, 205, 
  // 8576
  147, 205, 
  // 8578
  101, 216, 97, 
  // 8581
  101, 216, 144, 
  // 8584
  101, 216, 146, 
  // 8587
  101, 216, 147, 
  // 8590
  95, 201, 
  // 8592
  124, 201, 
  // 8594
  126, 201, 
  // 8596
  127, 201, 
  // 8598
  128, 201, 
  // 8600
  101, 212, 95, 
  // 8603
  101, 212, 124, 
  // 8606
  101, 212, 126, 
  // 8609
  101, 212, 127, 
  // 8612
  101, 212, 128, 
  // 8615
  100, 203, 
  // 8617
  134, 203, 
  // 8619
  136, 203, 
  // 8621
  137, 203, 
  // 8623
  138, 203, 
  // 8625
  101, 214, 100, 
  // 8628
  101, 214, 134, 
  // 8631
  101, 214, 136, 
  // 8634
  101, 214, 137, 
  // 8637
  101, 214, 138, 
  // 8640
  99, 207, 
  // 8642
  154, 207, 
  // 8644
  156, 207, 
  // 8646
  157, 207, 
  // 8648
  101, 218, 99, 
  // 8651
  101, 218, 154, 
  // 8654
  101, 218, 156, 
  // 8657
  101, 218, 157, 
  // 8660
  97, 134, 134, 97, 
  // 8664
  80, 129, 129, 80, 
  // 8668
  97, 106, 
  // 8670
  97, 146, 146, 97, 
  // 8674
  40, 40, 50, 50, 
  // 8678
  35, 35, 35, 35, 
  // 8682
  52, 52, 50, 52, 
};

static const OpenCLBuiltinStruct BuiltinTable[] = {
  // 1: convert_float, convert_float_rte, convert_float_rtn, convert_float_rtp, convert_float_rtz, 
  { 0, 2, 0, 1, 0, 64, 63 },
  { 12, 2, 0, 1, 0, 64, 63 },
  { 24, 2, 0, 1, 0, 64, 63 },
  { 36, 2, 0, 1, 0, 64, 63 },
  { 48, 2, 0, 1, 0, 64, 63 },
  { 60, 2, 0, 1, 0, 64, 63 },
  { 72, 2, 0, 1, 0, 64, 63 },
  { 84, 2, 0, 1, 0, 64, 63 },
  { 96, 2, 0, 1, 0, 64, 63 },
  { 108, 2, 0, 1, 0, 64, 63 },
  { 120, 2, 0, 1, 0, 64, 63 },
  // 12: convert_float2, convert_float2_rte, convert_float2_rtn, convert_float2_rtp, convert_float2_rtz, 
  { 2, 2, 0, 1, 0, 64, 63 },
  { 14, 2, 0, 1, 0, 64, 63 },
  { 26, 2, 0, 1, 0, 64, 63 },
  { 38, 2, 0, 1, 0, 64, 63 },
  { 50, 2, 0, 1, 0, 64, 63 },
  { 62, 2, 0, 1, 0, 64, 63 },
  { 74, 2, 0, 1, 0, 64, 63 },
  { 86, 2, 0, 1, 0, 64, 63 },
  { 98, 2, 0, 1, 0, 64, 63 },
  { 110, 2, 0, 1, 0, 64, 63 },
  { 122, 2, 0, 1, 0, 64, 63 },
  // 23: convert_float3, convert_float3_rte, convert_float3_rtn, convert_float3_rtp, convert_float3_rtz, 
  { 4, 2, 0, 1, 0, 64, 63 },
  { 16, 2, 0, 1, 0, 64, 63 },
  { 28, 2, 0, 1, 0, 64, 63 },
  { 40, 2, 0, 1, 0, 64, 63 },
  { 52, 2, 0, 1, 0, 64, 63 },
  { 64, 2, 0, 1, 0, 64, 63 },
  { 76, 2, 0, 1, 0, 64, 63 },
  { 88, 2, 0, 1, 0, 64, 63 },
  { 100, 2, 0, 1, 0, 64, 63 },
  { 112, 2, 0, 1, 0, 64, 63 },
  { 124, 2, 0, 1, 0, 64, 63 },
  // 34: convert_float4, convert_float4_rte, convert_float4_rtn, convert_float4_rtp, convert_float4_rtz, 
  { 6, 2, 0, 1, 0, 64, 63 },
  { 18, 2, 0, 1, 0, 64, 63 },
  { 30, 2, 0, 1, 0, 64, 63 },
  { 42, 2, 0, 1, 0, 64, 63 },
  { 54, 2, 0, 1, 0, 64, 63 },
  { 66, 2, 0, 1, 0, 64, 63 },
  { 78, 2, 0, 1, 0, 64, 63 },
  { 90, 2, 0, 1, 0, 64, 63 },
  { 102, 2, 0, 1, 0, 64, 63 },
  { 114, 2, 0, 1, 0, 64, 63 },
  { 126, 2, 0, 1, 0, 64, 63 },
  // 45: convert_float8, convert_float8_rte, convert_float8_rtn, convert_float8_rtp, convert_float8_rtz, 
  { 8, 2, 0, 1, 0, 64, 63 },
  { 20, 2, 0, 1, 0, 64, 63 },
  { 32, 2, 0, 1, 0, 64, 63 },
  { 44, 2, 0, 1, 0, 64, 63 },
  { 56, 2, 0, 1, 0, 64, 63 },
  { 68, 2, 0, 1, 0, 64, 63 },
  { 80, 2, 0, 1, 0, 64, 63 },
  { 92, 2, 0, 1, 0, 64, 63 },
  { 104, 2, 0, 1, 0, 64, 63 },
  { 116, 2, 0, 1, 0, 64, 63 },
  { 128, 2, 0, 1, 0, 64, 63 },
  // 56: convert_float16, convert_float16_rte, convert_float16_rtn, convert_float16_rtp, convert_float16_rtz, 
  { 10, 2, 0, 1, 0, 64, 63 },
  { 22, 2, 0, 1, 0, 64, 63 },
  { 34, 2, 0, 1, 0, 64, 63 },
  { 46, 2, 0, 1, 0, 64, 63 },
  { 58, 2, 0, 1, 0, 64, 63 },
  { 70, 2, 0, 1, 0, 64, 63 },
  { 82, 2, 0, 1, 0, 64, 63 },
  { 94, 2, 0, 1, 0, 64, 63 },
  { 106, 2, 0, 1, 0, 64, 63 },
  { 118, 2, 0, 1, 0, 64, 63 },
  { 130, 2, 0, 1, 0, 64, 63 },
  // 67: convert_double, convert_double_rte, convert_double_rtn, convert_double_rtp, convert_double_rtz, 
  { 132, 2, 0, 1, 0, 64, 63 },
  { 144, 2, 0, 1, 0, 64, 63 },
  { 156, 2, 0, 1, 0, 64, 63 },
  { 168, 2, 0, 1, 0, 64, 63 },
  { 180, 2, 0, 1, 0, 64, 63 },
  { 192, 2, 0, 1, 0, 64, 63 },
  { 204, 2, 0, 1, 0, 64, 63 },
  { 216, 2, 0, 1, 0, 64, 63 },
  { 228, 2, 0, 1, 0, 64, 63 },
  { 240, 2, 0, 1, 0, 64, 63 },
  { 252, 2, 0, 1, 0, 64, 63 },
  // 78: convert_double2, convert_double2_rte, convert_double2_rtn, convert_double2_rtp, convert_double2_rtz, 
  { 134, 2, 0, 1, 0, 64, 63 },
  { 146, 2, 0, 1, 0, 64, 63 },
  { 158, 2, 0, 1, 0, 64, 63 },
  { 170, 2, 0, 1, 0, 64, 63 },
  { 182, 2, 0, 1, 0, 64, 63 },
  { 194, 2, 0, 1, 0, 64, 63 },
  { 206, 2, 0, 1, 0, 64, 63 },
  { 218, 2, 0, 1, 0, 64, 63 },
  { 230, 2, 0, 1, 0, 64, 63 },
  { 242, 2, 0, 1, 0, 64, 63 },
  { 254, 2, 0, 1, 0, 64, 63 },
  // 89: convert_double3, convert_double3_rte, convert_double3_rtn, convert_double3_rtp, convert_double3_rtz, 
  { 136, 2, 0, 1, 0, 64, 63 },
  { 148, 2, 0, 1, 0, 64, 63 },
  { 160, 2, 0, 1, 0, 64, 63 },
  { 172, 2, 0, 1, 0, 64, 63 },
  { 184, 2, 0, 1, 0, 64, 63 },
  { 196, 2, 0, 1, 0, 64, 63 },
  { 208, 2, 0, 1, 0, 64, 63 },
  { 220, 2, 0, 1, 0, 64, 63 },
  { 232, 2, 0, 1, 0, 64, 63 },
  { 244, 2, 0, 1, 0, 64, 63 },
  { 256, 2, 0, 1, 0, 64, 63 },
  // 100: convert_double4, convert_double4_rte, convert_double4_rtn, convert_double4_rtp, convert_double4_rtz, 
  { 138, 2, 0, 1, 0, 64, 63 },
  { 150, 2, 0, 1, 0, 64, 63 },
  { 162, 2, 0, 1, 0, 64, 63 },
  { 174, 2, 0, 1, 0, 64, 63 },
  { 186, 2, 0, 1, 0, 64, 63 },
  { 198, 2, 0, 1, 0, 64, 63 },
  { 210, 2, 0, 1, 0, 64, 63 },
  { 222, 2, 0, 1, 0, 64, 63 },
  { 234, 2, 0, 1, 0, 64, 63 },
  { 246, 2, 0, 1, 0, 64, 63 },
  { 258, 2, 0, 1, 0, 64, 63 },
  // 111: convert_double8, convert_double8_rte, convert_double8_rtn, convert_double8_rtp, convert_double8_rtz, 
  { 140, 2, 0, 1, 0, 64, 63 },
  { 152, 2, 0, 1, 0, 64, 63 },
  { 164, 2, 0, 1, 0, 64, 63 },
  { 176, 2, 0, 1, 0, 64, 63 },
  { 188, 2, 0, 1, 0, 64, 63 },
  { 200, 2, 0, 1, 0, 64, 63 },
  { 212, 2, 0, 1, 0, 64, 63 },
  { 224, 2, 0, 1, 0, 64, 63 },
  { 236, 2, 0, 1, 0, 64, 63 },
  { 248, 2, 0, 1, 0, 64, 63 },
  { 260, 2, 0, 1, 0, 64, 63 },
  // 122: convert_double16, convert_double16_rte, convert_double16_rtn, convert_double16_rtp, convert_double16_rtz, 
  { 142, 2, 0, 1, 0, 64, 63 },
  { 154, 2, 0, 1, 0, 64, 63 },
  { 166, 2, 0, 1, 0, 64, 63 },
  { 178, 2, 0, 1, 0, 64, 63 },
  { 190, 2, 0, 1, 0, 64, 63 },
  { 202, 2, 0, 1, 0, 64, 63 },
  { 214, 2, 0, 1, 0, 64, 63 },
  { 226, 2, 0, 1, 0, 64, 63 },
  { 238, 2, 0, 1, 0, 64, 63 },
  { 250, 2, 0, 1, 0, 64, 63 },
  { 262, 2, 0, 1, 0, 64, 63 },
  // 133: convert_half, convert_half_rte, convert_half_rtn, convert_half_rtp, convert_half_rtz, 
  { 264, 2, 0, 1, 0, 64, 63 },
  { 276, 2, 0, 1, 0, 64, 63 },
  { 288, 2, 0, 1, 0, 64, 63 },
  { 300, 2, 0, 1, 0, 64, 63 },
  { 312, 2, 0, 1, 0, 64, 63 },
  { 324, 2, 0, 1, 0, 64, 63 },
  { 336, 2, 0, 1, 0, 64, 63 },
  { 348, 2, 0, 1, 0, 64, 63 },
  { 360, 2, 0, 1, 0, 64, 63 },
  { 372, 2, 0, 1, 0, 64, 63 },
  { 384, 2, 0, 1, 0, 64, 63 },
  // 144: convert_half2, convert_half2_rte, convert_half2_rtn, convert_half2_rtp, convert_half2_rtz, 
  { 266, 2, 0, 1, 0, 64, 63 },
  { 278, 2, 0, 1, 0, 64, 63 },
  { 290, 2, 0, 1, 0, 64, 63 },
  { 302, 2, 0, 1, 0, 64, 63 },
  { 314, 2, 0, 1, 0, 64, 63 },
  { 326, 2, 0, 1, 0, 64, 63 },
  { 338, 2, 0, 1, 0, 64, 63 },
  { 350, 2, 0, 1, 0, 64, 63 },
  { 362, 2, 0, 1, 0, 64, 63 },
  { 374, 2, 0, 1, 0, 64, 63 },
  { 386, 2, 0, 1, 0, 64, 63 },
  // 155: convert_half3, convert_half3_rte, convert_half3_rtn, convert_half3_rtp, convert_half3_rtz, 
  { 268, 2, 0, 1, 0, 64, 63 },
  { 280, 2, 0, 1, 0, 64, 63 },
  { 292, 2, 0, 1, 0, 64, 63 },
  { 304, 2, 0, 1, 0, 64, 63 },
  { 316, 2, 0, 1, 0, 64, 63 },
  { 328, 2, 0, 1, 0, 64, 63 },
  { 340, 2, 0, 1, 0, 64, 63 },
  { 352, 2, 0, 1, 0, 64, 63 },
  { 364, 2, 0, 1, 0, 64, 63 },
  { 376, 2, 0, 1, 0, 64, 63 },
  { 388, 2, 0, 1, 0, 64, 63 },
  // 166: convert_half4, convert_half4_rte, convert_half4_rtn, convert_half4_rtp, convert_half4_rtz, 
  { 270, 2, 0, 1, 0, 64, 63 },
  { 282, 2, 0, 1, 0, 64, 63 },
  { 294, 2, 0, 1, 0, 64, 63 },
  { 306, 2, 0, 1, 0, 64, 63 },
  { 318, 2, 0, 1, 0, 64, 63 },
  { 330, 2, 0, 1, 0, 64, 63 },
  { 342, 2, 0, 1, 0, 64, 63 },
  { 354, 2, 0, 1, 0, 64, 63 },
  { 366, 2, 0, 1, 0, 64, 63 },
  { 378, 2, 0, 1, 0, 64, 63 },
  { 390, 2, 0, 1, 0, 64, 63 },
  // 177: convert_half8, convert_half8_rte, convert_half8_rtn, convert_half8_rtp, convert_half8_rtz, 
  { 272, 2, 0, 1, 0, 64, 63 },
  { 284, 2, 0, 1, 0, 64, 63 },
  { 296, 2, 0, 1, 0, 64, 63 },
  { 308, 2, 0, 1, 0, 64, 63 },
  { 320, 2, 0, 1, 0, 64, 63 },
  { 332, 2, 0, 1, 0, 64, 63 },
  { 344, 2, 0, 1, 0, 64, 63 },
  { 356, 2, 0, 1, 0, 64, 63 },
  { 368, 2, 0, 1, 0, 64, 63 },
  { 380, 2, 0, 1, 0, 64, 63 },
  { 392, 2, 0, 1, 0, 64, 63 },
  // 188: convert_half16, convert_half16_rte, convert_half16_rtn, convert_half16_rtp, convert_half16_rtz, 
  { 274, 2, 0, 1, 0, 64, 63 },
  { 286, 2, 0, 1, 0, 64, 63 },
  { 298, 2, 0, 1, 0, 64, 63 },
  { 310, 2, 0, 1, 0, 64, 63 },
  { 322, 2, 0, 1, 0, 64, 63 },
  { 334, 2, 0, 1, 0, 64, 63 },
  { 346, 2, 0, 1, 0, 64, 63 },
  { 358, 2, 0, 1, 0, 64, 63 },
  { 370, 2, 0, 1, 0, 64, 63 },
  { 382, 2, 0, 1, 0, 64, 63 },
  { 394, 2, 0, 1, 0, 64, 63 },
  // 199: convert_char, convert_char_rte, convert_char_rtn, convert_char_rtp, convert_char_rtz, convert_char_sat, convert_char_sat_rte, convert_char_sat_rtn, convert_char_sat_rtp, convert_char_sat_rtz, 
  { 396, 2, 0, 1, 0, 64, 63 },
  { 408, 2, 0, 1, 0, 64, 63 },
  { 420, 2, 0, 1, 0, 64, 63 },
  { 432, 2, 0, 1, 0, 64, 63 },
  { 444, 2, 0, 1, 0, 64, 63 },
  { 456, 2, 0, 1, 0, 64, 63 },
  { 468, 2, 0, 1, 0, 64, 63 },
  { 480, 2, 0, 1, 0, 64, 63 },
  { 492, 2, 0, 1, 0, 64, 63 },
  { 504, 2, 0, 1, 0, 64, 63 },
  { 516, 2, 0, 1, 0, 64, 63 },
  // 210: convert_char2, convert_char2_rte, convert_char2_rtn, convert_char2_rtp, convert_char2_rtz, convert_char2_sat, convert_char2_sat_rte, convert_char2_sat_rtn, convert_char2_sat_rtp, convert_char2_sat_rtz, 
  { 398, 2, 0, 1, 0, 64, 63 },
  { 410, 2, 0, 1, 0, 64, 63 },
  { 422, 2, 0, 1, 0, 64, 63 },
  { 434, 2, 0, 1, 0, 64, 63 },
  { 446, 2, 0, 1, 0, 64, 63 },
  { 458, 2, 0, 1, 0, 64, 63 },
  { 470, 2, 0, 1, 0, 64, 63 },
  { 482, 2, 0, 1, 0, 64, 63 },
  { 494, 2, 0, 1, 0, 64, 63 },
  { 506, 2, 0, 1, 0, 64, 63 },
  { 518, 2, 0, 1, 0, 64, 63 },
  // 221: convert_char3, convert_char3_rte, convert_char3_rtn, convert_char3_rtp, convert_char3_rtz, convert_char3_sat, convert_char3_sat_rte, convert_char3_sat_rtn, convert_char3_sat_rtp, convert_char3_sat_rtz, 
  { 400, 2, 0, 1, 0, 64, 63 },
  { 412, 2, 0, 1, 0, 64, 63 },
  { 424, 2, 0, 1, 0, 64, 63 },
  { 436, 2, 0, 1, 0, 64, 63 },
  { 448, 2, 0, 1, 0, 64, 63 },
  { 460, 2, 0, 1, 0, 64, 63 },
  { 472, 2, 0, 1, 0, 64, 63 },
  { 484, 2, 0, 1, 0, 64, 63 },
  { 496, 2, 0, 1, 0, 64, 63 },
  { 508, 2, 0, 1, 0, 64, 63 },
  { 520, 2, 0, 1, 0, 64, 63 },
  // 232: convert_char4, convert_char4_rte, convert_char4_rtn, convert_char4_rtp, convert_char4_rtz, convert_char4_sat, convert_char4_sat_rte, convert_char4_sat_rtn, convert_char4_sat_rtp, convert_char4_sat_rtz, 
  { 402, 2, 0, 1, 0, 64, 63 },
  { 414, 2, 0, 1, 0, 64, 63 },
  { 426, 2, 0, 1, 0, 64, 63 },
  { 438, 2, 0, 1, 0, 64, 63 },
  { 450, 2, 0, 1, 0, 64, 63 },
  { 462, 2, 0, 1, 0, 64, 63 },
  { 474, 2, 0, 1, 0, 64, 63 },
  { 486, 2, 0, 1, 0, 64, 63 },
  { 498, 2, 0, 1, 0, 64, 63 },
  { 510, 2, 0, 1, 0, 64, 63 },
  { 522, 2, 0, 1, 0, 64, 63 },
  // 243: convert_char8, convert_char8_rte, convert_char8_rtn, convert_char8_rtp, convert_char8_rtz, convert_char8_sat, convert_char8_sat_rte, convert_char8_sat_rtn, convert_char8_sat_rtp, convert_char8_sat_rtz, 
  { 404, 2, 0, 1, 0, 64, 63 },
  { 416, 2, 0, 1, 0, 64, 63 },
  { 428, 2, 0, 1, 0, 64, 63 },
  { 440, 2, 0, 1, 0, 64, 63 },
  { 452, 2, 0, 1, 0, 64, 63 },
  { 464, 2, 0, 1, 0, 64, 63 },
  { 476, 2, 0, 1, 0, 64, 63 },
  { 488, 2, 0, 1, 0, 64, 63 },
  { 500, 2, 0, 1, 0, 64, 63 },
  { 512, 2, 0, 1, 0, 64, 63 },
  { 524, 2, 0, 1, 0, 64, 63 },
  // 254: convert_char16, convert_char16_rte, convert_char16_rtn, convert_char16_rtp, convert_char16_rtz, convert_char16_sat, convert_char16_sat_rte, convert_char16_sat_rtn, convert_char16_sat_rtp, convert_char16_sat_rtz, 
  { 406, 2, 0, 1, 0, 64, 63 },
  { 418, 2, 0, 1, 0, 64, 63 },
  { 430, 2, 0, 1, 0, 64, 63 },
  { 442, 2, 0, 1, 0, 64, 63 },
  { 454, 2, 0, 1, 0, 64, 63 },
  { 466, 2, 0, 1, 0, 64, 63 },
  { 478, 2, 0, 1, 0, 64, 63 },
  { 490, 2, 0, 1, 0, 64, 63 },
  { 502, 2, 0, 1, 0, 64, 63 },
  { 514, 2, 0, 1, 0, 64, 63 },
  { 526, 2, 0, 1, 0, 64, 63 },
  // 265: convert_uchar, convert_uchar_rte, convert_uchar_rtn, convert_uchar_rtp, convert_uchar_rtz, convert_uchar_sat, convert_uchar_sat_rte, convert_uchar_sat_rtn, convert_uchar_sat_rtp, convert_uchar_sat_rtz, 
  { 528, 2, 0, 1, 0, 64, 63 },
  { 540, 2, 0, 1, 0, 64, 63 },
  { 552, 2, 0, 1, 0, 64, 63 },
  { 564, 2, 0, 1, 0, 64, 63 },
  { 576, 2, 0, 1, 0, 64, 63 },
  { 588, 2, 0, 1, 0, 64, 63 },
  { 600, 2, 0, 1, 0, 64, 63 },
  { 612, 2, 0, 1, 0, 64, 63 },
  { 624, 2, 0, 1, 0, 64, 63 },
  { 636, 2, 0, 1, 0, 64, 63 },
  { 648, 2, 0, 1, 0, 64, 63 },
  // 276: convert_uchar2, convert_uchar2_rte, convert_uchar2_rtn, convert_uchar2_rtp, convert_uchar2_rtz, convert_uchar2_sat, convert_uchar2_sat_rte, convert_uchar2_sat_rtn, convert_uchar2_sat_rtp, convert_uchar2_sat_rtz, 
  { 530, 2, 0, 1, 0, 64, 63 },
  { 542, 2, 0, 1, 0, 64, 63 },
  { 554, 2, 0, 1, 0, 64, 63 },
  { 566, 2, 0, 1, 0, 64, 63 },
  { 578, 2, 0, 1, 0, 64, 63 },
  { 590, 2, 0, 1, 0, 64, 63 },
  { 602, 2, 0, 1, 0, 64, 63 },
  { 614, 2, 0, 1, 0, 64, 63 },
  { 626, 2, 0, 1, 0, 64, 63 },
  { 638, 2, 0, 1, 0, 64, 63 },
  { 650, 2, 0, 1, 0, 64, 63 },
  // 287: convert_uchar3, convert_uchar3_rte, convert_uchar3_rtn, convert_uchar3_rtp, convert_uchar3_rtz, convert_uchar3_sat, convert_uchar3_sat_rte, convert_uchar3_sat_rtn, convert_uchar3_sat_rtp, convert_uchar3_sat_rtz, 
  { 532, 2, 0, 1, 0, 64, 63 },
  { 544, 2, 0, 1, 0, 64, 63 },
  { 556, 2, 0, 1, 0, 64, 63 },
  { 568, 2, 0, 1, 0, 64, 63 },
  { 580, 2, 0, 1, 0, 64, 63 },
  { 592, 2, 0, 1, 0, 64, 63 },
  { 604, 2, 0, 1, 0, 64, 63 },
  { 616, 2, 0, 1, 0, 64, 63 },
  { 628, 2, 0, 1, 0, 64, 63 },
  { 640, 2, 0, 1, 0, 64, 63 },
  { 652, 2, 0, 1, 0, 64, 63 },
  // 298: convert_uchar4, convert_uchar4_rte, convert_uchar4_rtn, convert_uchar4_rtp, convert_uchar4_rtz, convert_uchar4_sat, convert_uchar4_sat_rte, convert_uchar4_sat_rtn, convert_uchar4_sat_rtp, convert_uchar4_sat_rtz, 
  { 534, 2, 0, 1, 0, 64, 63 },
  { 546, 2, 0, 1, 0, 64, 63 },
  { 558, 2, 0, 1, 0, 64, 63 },
  { 570, 2, 0, 1, 0, 64, 63 },
  { 582, 2, 0, 1, 0, 64, 63 },
  { 594, 2, 0, 1, 0, 64, 63 },
  { 606, 2, 0, 1, 0, 64, 63 },
  { 618, 2, 0, 1, 0, 64, 63 },
  { 630, 2, 0, 1, 0, 64, 63 },
  { 642, 2, 0, 1, 0, 64, 63 },
  { 654, 2, 0, 1, 0, 64, 63 },
  // 309: convert_uchar8, convert_uchar8_rte, convert_uchar8_rtn, convert_uchar8_rtp, convert_uchar8_rtz, convert_uchar8_sat, convert_uchar8_sat_rte, convert_uchar8_sat_rtn, convert_uchar8_sat_rtp, convert_uchar8_sat_rtz, 
  { 536, 2, 0, 1, 0, 64, 63 },
  { 548, 2, 0, 1, 0, 64, 63 },
  { 560, 2, 0, 1, 0, 64, 63 },
  { 572, 2, 0, 1, 0, 64, 63 },
  { 584, 2, 0, 1, 0, 64, 63 },
  { 596, 2, 0, 1, 0, 64, 63 },
  { 608, 2, 0, 1, 0, 64, 63 },
  { 620, 2, 0, 1, 0, 64, 63 },
  { 632, 2, 0, 1, 0, 64, 63 },
  { 644, 2, 0, 1, 0, 64, 63 },
  { 656, 2, 0, 1, 0, 64, 63 },
  // 320: convert_uchar16, convert_uchar16_rte, convert_uchar16_rtn, convert_uchar16_rtp, convert_uchar16_rtz, convert_uchar16_sat, convert_uchar16_sat_rte, convert_uchar16_sat_rtn, convert_uchar16_sat_rtp, convert_uchar16_sat_rtz, 
  { 538, 2, 0, 1, 0, 64, 63 },
  { 550, 2, 0, 1, 0, 64, 63 },
  { 562, 2, 0, 1, 0, 64, 63 },
  { 574, 2, 0, 1, 0, 64, 63 },
  { 586, 2, 0, 1, 0, 64, 63 },
  { 598, 2, 0, 1, 0, 64, 63 },
  { 610, 2, 0, 1, 0, 64, 63 },
  { 622, 2, 0, 1, 0, 64, 63 },
  { 634, 2, 0, 1, 0, 64, 63 },
  { 646, 2, 0, 1, 0, 64, 63 },
  { 658, 2, 0, 1, 0, 64, 63 },
  // 331: convert_short, convert_short_rte, convert_short_rtn, convert_short_rtp, convert_short_rtz, convert_short_sat, convert_short_sat_rte, convert_short_sat_rtn, convert_short_sat_rtp, convert_short_sat_rtz, 
  { 660, 2, 0, 1, 0, 64, 63 },
  { 672, 2, 0, 1, 0, 64, 63 },
  { 684, 2, 0, 1, 0, 64, 63 },
  { 696, 2, 0, 1, 0, 64, 63 },
  { 708, 2, 0, 1, 0, 64, 63 },
  { 720, 2, 0, 1, 0, 64, 63 },
  { 732, 2, 0, 1, 0, 64, 63 },
  { 744, 2, 0, 1, 0, 64, 63 },
  { 756, 2, 0, 1, 0, 64, 63 },
  { 768, 2, 0, 1, 0, 64, 63 },
  { 780, 2, 0, 1, 0, 64, 63 },
  // 342: convert_short2, convert_short2_rte, convert_short2_rtn, convert_short2_rtp, convert_short2_rtz, convert_short2_sat, convert_short2_sat_rte, convert_short2_sat_rtn, convert_short2_sat_rtp, convert_short2_sat_rtz, 
  { 662, 2, 0, 1, 0, 64, 63 },
  { 674, 2, 0, 1, 0, 64, 63 },
  { 686, 2, 0, 1, 0, 64, 63 },
  { 698, 2, 0, 1, 0, 64, 63 },
  { 710, 2, 0, 1, 0, 64, 63 },
  { 722, 2, 0, 1, 0, 64, 63 },
  { 734, 2, 0, 1, 0, 64, 63 },
  { 746, 2, 0, 1, 0, 64, 63 },
  { 758, 2, 0, 1, 0, 64, 63 },
  { 770, 2, 0, 1, 0, 64, 63 },
  { 782, 2, 0, 1, 0, 64, 63 },
  // 353: convert_short3, convert_short3_rte, convert_short3_rtn, convert_short3_rtp, convert_short3_rtz, convert_short3_sat, convert_short3_sat_rte, convert_short3_sat_rtn, convert_short3_sat_rtp, convert_short3_sat_rtz, 
  { 664, 2, 0, 1, 0, 64, 63 },
  { 676, 2, 0, 1, 0, 64, 63 },
  { 688, 2, 0, 1, 0, 64, 63 },
  { 700, 2, 0, 1, 0, 64, 63 },
  { 712, 2, 0, 1, 0, 64, 63 },
  { 724, 2, 0, 1, 0, 64, 63 },
  { 736, 2, 0, 1, 0, 64, 63 },
  { 748, 2, 0, 1, 0, 64, 63 },
  { 760, 2, 0, 1, 0, 64, 63 },
  { 772, 2, 0, 1, 0, 64, 63 },
  { 784, 2, 0, 1, 0, 64, 63 },
  // 364: convert_short4, convert_short4_rte, convert_short4_rtn, convert_short4_rtp, convert_short4_rtz, convert_short4_sat, convert_short4_sat_rte, convert_short4_sat_rtn, convert_short4_sat_rtp, convert_short4_sat_rtz, 
  { 666, 2, 0, 1, 0, 64, 63 },
  { 678, 2, 0, 1, 0, 64, 63 },
  { 690, 2, 0, 1, 0, 64, 63 },
  { 702, 2, 0, 1, 0, 64, 63 },
  { 714, 2, 0, 1, 0, 64, 63 },
  { 726, 2, 0, 1, 0, 64, 63 },
  { 738, 2, 0, 1, 0, 64, 63 },
  { 750, 2, 0, 1, 0, 64, 63 },
  { 762, 2, 0, 1, 0, 64, 63 },
  { 774, 2, 0, 1, 0, 64, 63 },
  { 786, 2, 0, 1, 0, 64, 63 },
  // 375: convert_short8, convert_short8_rte, convert_short8_rtn, convert_short8_rtp, convert_short8_rtz, convert_short8_sat, convert_short8_sat_rte, convert_short8_sat_rtn, convert_short8_sat_rtp, convert_short8_sat_rtz, 
  { 668, 2, 0, 1, 0, 64, 63 },
  { 680, 2, 0, 1, 0, 64, 63 },
  { 692, 2, 0, 1, 0, 64, 63 },
  { 704, 2, 0, 1, 0, 64, 63 },
  { 716, 2, 0, 1, 0, 64, 63 },
  { 728, 2, 0, 1, 0, 64, 63 },
  { 740, 2, 0, 1, 0, 64, 63 },
  { 752, 2, 0, 1, 0, 64, 63 },
  { 764, 2, 0, 1, 0, 64, 63 },
  { 776, 2, 0, 1, 0, 64, 63 },
  { 788, 2, 0, 1, 0, 64, 63 },
  // 386: convert_short16, convert_short16_rte, convert_short16_rtn, convert_short16_rtp, convert_short16_rtz, convert_short16_sat, convert_short16_sat_rte, convert_short16_sat_rtn, convert_short16_sat_rtp, convert_short16_sat_rtz, 
  { 670, 2, 0, 1, 0, 64, 63 },
  { 682, 2, 0, 1, 0, 64, 63 },
  { 694, 2, 0, 1, 0, 64, 63 },
  { 706, 2, 0, 1, 0, 64, 63 },
  { 718, 2, 0, 1, 0, 64, 63 },
  { 730, 2, 0, 1, 0, 64, 63 },
  { 742, 2, 0, 1, 0, 64, 63 },
  { 754, 2, 0, 1, 0, 64, 63 },
  { 766, 2, 0, 1, 0, 64, 63 },
  { 778, 2, 0, 1, 0, 64, 63 },
  { 790, 2, 0, 1, 0, 64, 63 },
  // 397: convert_ushort, convert_ushort_rte, convert_ushort_rtn, convert_ushort_rtp, convert_ushort_rtz, convert_ushort_sat, convert_ushort_sat_rte, convert_ushort_sat_rtn, convert_ushort_sat_rtp, convert_ushort_sat_rtz, 
  { 792, 2, 0, 1, 0, 64, 63 },
  { 804, 2, 0, 1, 0, 64, 63 },
  { 816, 2, 0, 1, 0, 64, 63 },
  { 828, 2, 0, 1, 0, 64, 63 },
  { 840, 2, 0, 1, 0, 64, 63 },
  { 852, 2, 0, 1, 0, 64, 63 },
  { 864, 2, 0, 1, 0, 64, 63 },
  { 876, 2, 0, 1, 0, 64, 63 },
  { 888, 2, 0, 1, 0, 64, 63 },
  { 900, 2, 0, 1, 0, 64, 63 },
  { 912, 2, 0, 1, 0, 64, 63 },
  // 408: convert_ushort2, convert_ushort2_rte, convert_ushort2_rtn, convert_ushort2_rtp, convert_ushort2_rtz, convert_ushort2_sat, convert_ushort2_sat_rte, convert_ushort2_sat_rtn, convert_ushort2_sat_rtp, convert_ushort2_sat_rtz, 
  { 794, 2, 0, 1, 0, 64, 63 },
  { 806, 2, 0, 1, 0, 64, 63 },
  { 818, 2, 0, 1, 0, 64, 63 },
  { 830, 2, 0, 1, 0, 64, 63 },
  { 842, 2, 0, 1, 0, 64, 63 },
  { 854, 2, 0, 1, 0, 64, 63 },
  { 866, 2, 0, 1, 0, 64, 63 },
  { 878, 2, 0, 1, 0, 64, 63 },
  { 890, 2, 0, 1, 0, 64, 63 },
  { 902, 2, 0, 1, 0, 64, 63 },
  { 914, 2, 0, 1, 0, 64, 63 },
  // 419: convert_ushort3, convert_ushort3_rte, convert_ushort3_rtn, convert_ushort3_rtp, convert_ushort3_rtz, convert_ushort3_sat, convert_ushort3_sat_rte, convert_ushort3_sat_rtn, convert_ushort3_sat_rtp, convert_ushort3_sat_rtz, 
  { 796, 2, 0, 1, 0, 64, 63 },
  { 808, 2, 0, 1, 0, 64, 63 },
  { 820, 2, 0, 1, 0, 64, 63 },
  { 832, 2, 0, 1, 0, 64, 63 },
  { 844, 2, 0, 1, 0, 64, 63 },
  { 856, 2, 0, 1, 0, 64, 63 },
  { 868, 2, 0, 1, 0, 64, 63 },
  { 880, 2, 0, 1, 0, 64, 63 },
  { 892, 2, 0, 1, 0, 64, 63 },
  { 904, 2, 0, 1, 0, 64, 63 },
  { 916, 2, 0, 1, 0, 64, 63 },
  // 430: convert_ushort4, convert_ushort4_rte, convert_ushort4_rtn, convert_ushort4_rtp, convert_ushort4_rtz, convert_ushort4_sat, convert_ushort4_sat_rte, convert_ushort4_sat_rtn, convert_ushort4_sat_rtp, convert_ushort4_sat_rtz, 
  { 798, 2, 0, 1, 0, 64, 63 },
  { 810, 2, 0, 1, 0, 64, 63 },
  { 822, 2, 0, 1, 0, 64, 63 },
  { 834, 2, 0, 1, 0, 64, 63 },
  { 846, 2, 0, 1, 0, 64, 63 },
  { 858, 2, 0, 1, 0, 64, 63 },
  { 870, 2, 0, 1, 0, 64, 63 },
  { 882, 2, 0, 1, 0, 64, 63 },
  { 894, 2, 0, 1, 0, 64, 63 },
  { 906, 2, 0, 1, 0, 64, 63 },
  { 918, 2, 0, 1, 0, 64, 63 },
  // 441: convert_ushort8, convert_ushort8_rte, convert_ushort8_rtn, convert_ushort8_rtp, convert_ushort8_rtz, convert_ushort8_sat, convert_ushort8_sat_rte, convert_ushort8_sat_rtn, convert_ushort8_sat_rtp, convert_ushort8_sat_rtz, 
  { 800, 2, 0, 1, 0, 64, 63 },
  { 812, 2, 0, 1, 0, 64, 63 },
  { 824, 2, 0, 1, 0, 64, 63 },
  { 836, 2, 0, 1, 0, 64, 63 },
  { 848, 2, 0, 1, 0, 64, 63 },
  { 860, 2, 0, 1, 0, 64, 63 },
  { 872, 2, 0, 1, 0, 64, 63 },
  { 884, 2, 0, 1, 0, 64, 63 },
  { 896, 2, 0, 1, 0, 64, 63 },
  { 908, 2, 0, 1, 0, 64, 63 },
  { 920, 2, 0, 1, 0, 64, 63 },
  // 452: convert_ushort16, convert_ushort16_rte, convert_ushort16_rtn, convert_ushort16_rtp, convert_ushort16_rtz, convert_ushort16_sat, convert_ushort16_sat_rte, convert_ushort16_sat_rtn, convert_ushort16_sat_rtp, convert_ushort16_sat_rtz, 
  { 802, 2, 0, 1, 0, 64, 63 },
  { 814, 2, 0, 1, 0, 64, 63 },
  { 826, 2, 0, 1, 0, 64, 63 },
  { 838, 2, 0, 1, 0, 64, 63 },
  { 850, 2, 0, 1, 0, 64, 63 },
  { 862, 2, 0, 1, 0, 64, 63 },
  { 874, 2, 0, 1, 0, 64, 63 },
  { 886, 2, 0, 1, 0, 64, 63 },
  { 898, 2, 0, 1, 0, 64, 63 },
  { 910, 2, 0, 1, 0, 64, 63 },
  { 922, 2, 0, 1, 0, 64, 63 },
  // 463: convert_int, convert_int_rte, convert_int_rtn, convert_int_rtp, convert_int_rtz, convert_int_sat, convert_int_sat_rte, convert_int_sat_rtn, convert_int_sat_rtp, convert_int_sat_rtz, 
  { 924, 2, 0, 1, 0, 64, 63 },
  { 936, 2, 0, 1, 0, 64, 63 },
  { 948, 2, 0, 1, 0, 64, 63 },
  { 960, 2, 0, 1, 0, 64, 63 },
  { 972, 2, 0, 1, 0, 64, 63 },
  { 984, 2, 0, 1, 0, 64, 63 },
  { 996, 2, 0, 1, 0, 64, 63 },
  { 1008, 2, 0, 1, 0, 64, 63 },
  { 1020, 2, 0, 1, 0, 64, 63 },
  { 1032, 2, 0, 1, 0, 64, 63 },
  { 1044, 2, 0, 1, 0, 64, 63 },
  // 474: convert_int2, convert_int2_rte, convert_int2_rtn, convert_int2_rtp, convert_int2_rtz, convert_int2_sat, convert_int2_sat_rte, convert_int2_sat_rtn, convert_int2_sat_rtp, convert_int2_sat_rtz, 
  { 926, 2, 0, 1, 0, 64, 63 },
  { 938, 2, 0, 1, 0, 64, 63 },
  { 950, 2, 0, 1, 0, 64, 63 },
  { 962, 2, 0, 1, 0, 64, 63 },
  { 974, 2, 0, 1, 0, 64, 63 },
  { 986, 2, 0, 1, 0, 64, 63 },
  { 998, 2, 0, 1, 0, 64, 63 },
  { 1010, 2, 0, 1, 0, 64, 63 },
  { 1022, 2, 0, 1, 0, 64, 63 },
  { 1034, 2, 0, 1, 0, 64, 63 },
  { 1046, 2, 0, 1, 0, 64, 63 },
  // 485: convert_int3, convert_int3_rte, convert_int3_rtn, convert_int3_rtp, convert_int3_rtz, convert_int3_sat, convert_int3_sat_rte, convert_int3_sat_rtn, convert_int3_sat_rtp, convert_int3_sat_rtz, 
  { 928, 2, 0, 1, 0, 64, 63 },
  { 940, 2, 0, 1, 0, 64, 63 },
  { 952, 2, 0, 1, 0, 64, 63 },
  { 964, 2, 0, 1, 0, 64, 63 },
  { 976, 2, 0, 1, 0, 64, 63 },
  { 988, 2, 0, 1, 0, 64, 63 },
  { 1000, 2, 0, 1, 0, 64, 63 },
  { 1012, 2, 0, 1, 0, 64, 63 },
  { 1024, 2, 0, 1, 0, 64, 63 },
  { 1036, 2, 0, 1, 0, 64, 63 },
  { 1048, 2, 0, 1, 0, 64, 63 },
  // 496: convert_int4, convert_int4_rte, convert_int4_rtn, convert_int4_rtp, convert_int4_rtz, convert_int4_sat, convert_int4_sat_rte, convert_int4_sat_rtn, convert_int4_sat_rtp, convert_int4_sat_rtz, 
  { 930, 2, 0, 1, 0, 64, 63 },
  { 942, 2, 0, 1, 0, 64, 63 },
  { 954, 2, 0, 1, 0, 64, 63 },
  { 966, 2, 0, 1, 0, 64, 63 },
  { 978, 2, 0, 1, 0, 64, 63 },
  { 990, 2, 0, 1, 0, 64, 63 },
  { 1002, 2, 0, 1, 0, 64, 63 },
  { 1014, 2, 0, 1, 0, 64, 63 },
  { 1026, 2, 0, 1, 0, 64, 63 },
  { 1038, 2, 0, 1, 0, 64, 63 },
  { 1050, 2, 0, 1, 0, 64, 63 },
  // 507: convert_int8, convert_int8_rte, convert_int8_rtn, convert_int8_rtp, convert_int8_rtz, convert_int8_sat, convert_int8_sat_rte, convert_int8_sat_rtn, convert_int8_sat_rtp, convert_int8_sat_rtz, 
  { 932, 2, 0, 1, 0, 64, 63 },
  { 944, 2, 0, 1, 0, 64, 63 },
  { 956, 2, 0, 1, 0, 64, 63 },
  { 968, 2, 0, 1, 0, 64, 63 },
  { 980, 2, 0, 1, 0, 64, 63 },
  { 992, 2, 0, 1, 0, 64, 63 },
  { 1004, 2, 0, 1, 0, 64, 63 },
  { 1016, 2, 0, 1, 0, 64, 63 },
  { 1028, 2, 0, 1, 0, 64, 63 },
  { 1040, 2, 0, 1, 0, 64, 63 },
  { 1052, 2, 0, 1, 0, 64, 63 },
  // 518: convert_int16, convert_int16_rte, convert_int16_rtn, convert_int16_rtp, convert_int16_rtz, convert_int16_sat, convert_int16_sat_rte, convert_int16_sat_rtn, convert_int16_sat_rtp, convert_int16_sat_rtz, 
  { 934, 2, 0, 1, 0, 64, 63 },
  { 946, 2, 0, 1, 0, 64, 63 },
  { 958, 2, 0, 1, 0, 64, 63 },
  { 970, 2, 0, 1, 0, 64, 63 },
  { 982, 2, 0, 1, 0, 64, 63 },
  { 994, 2, 0, 1, 0, 64, 63 },
  { 1006, 2, 0, 1, 0, 64, 63 },
  { 1018, 2, 0, 1, 0, 64, 63 },
  { 1030, 2, 0, 1, 0, 64, 63 },
  { 1042, 2, 0, 1, 0, 64, 63 },
  { 1054, 2, 0, 1, 0, 64, 63 },
  // 529: convert_uint, convert_uint_rte, convert_uint_rtn, convert_uint_rtp, convert_uint_rtz, convert_uint_sat, convert_uint_sat_rte, convert_uint_sat_rtn, convert_uint_sat_rtp, convert_uint_sat_rtz, 
  { 1056, 2, 0, 1, 0, 64, 63 },
  { 1068, 2, 0, 1, 0, 64, 63 },
  { 1080, 2, 0, 1, 0, 64, 63 },
  { 1092, 2, 0, 1, 0, 64, 63 },
  { 1104, 2, 0, 1, 0, 64, 63 },
  { 1116, 2, 0, 1, 0, 64, 63 },
  { 1128, 2, 0, 1, 0, 64, 63 },
  { 1140, 2, 0, 1, 0, 64, 63 },
  { 1152, 2, 0, 1, 0, 64, 63 },
  { 1164, 2, 0, 1, 0, 64, 63 },
  { 1176, 2, 0, 1, 0, 64, 63 },
  // 540: convert_uint2, convert_uint2_rte, convert_uint2_rtn, convert_uint2_rtp, convert_uint2_rtz, convert_uint2_sat, convert_uint2_sat_rte, convert_uint2_sat_rtn, convert_uint2_sat_rtp, convert_uint2_sat_rtz, 
  { 1058, 2, 0, 1, 0, 64, 63 },
  { 1070, 2, 0, 1, 0, 64, 63 },
  { 1082, 2, 0, 1, 0, 64, 63 },
  { 1094, 2, 0, 1, 0, 64, 63 },
  { 1106, 2, 0, 1, 0, 64, 63 },
  { 1118, 2, 0, 1, 0, 64, 63 },
  { 1130, 2, 0, 1, 0, 64, 63 },
  { 1142, 2, 0, 1, 0, 64, 63 },
  { 1154, 2, 0, 1, 0, 64, 63 },
  { 1166, 2, 0, 1, 0, 64, 63 },
  { 1178, 2, 0, 1, 0, 64, 63 },
  // 551: convert_uint3, convert_uint3_rte, convert_uint3_rtn, convert_uint3_rtp, convert_uint3_rtz, convert_uint3_sat, convert_uint3_sat_rte, convert_uint3_sat_rtn, convert_uint3_sat_rtp, convert_uint3_sat_rtz, 
  { 1060, 2, 0, 1, 0, 64, 63 },
  { 1072, 2, 0, 1, 0, 64, 63 },
  { 1084, 2, 0, 1, 0, 64, 63 },
  { 1096, 2, 0, 1, 0, 64, 63 },
  { 1108, 2, 0, 1, 0, 64, 63 },
  { 1120, 2, 0, 1, 0, 64, 63 },
  { 1132, 2, 0, 1, 0, 64, 63 },
  { 1144, 2, 0, 1, 0, 64, 63 },
  { 1156, 2, 0, 1, 0, 64, 63 },
  { 1168, 2, 0, 1, 0, 64, 63 },
  { 1180, 2, 0, 1, 0, 64, 63 },
  // 562: convert_uint4, convert_uint4_rte, convert_uint4_rtn, convert_uint4_rtp, convert_uint4_rtz, convert_uint4_sat, convert_uint4_sat_rte, convert_uint4_sat_rtn, convert_uint4_sat_rtp, convert_uint4_sat_rtz, 
  { 1062, 2, 0, 1, 0, 64, 63 },
  { 1074, 2, 0, 1, 0, 64, 63 },
  { 1086, 2, 0, 1, 0, 64, 63 },
  { 1098, 2, 0, 1, 0, 64, 63 },
  { 1110, 2, 0, 1, 0, 64, 63 },
  { 1122, 2, 0, 1, 0, 64, 63 },
  { 1134, 2, 0, 1, 0, 64, 63 },
  { 1146, 2, 0, 1, 0, 64, 63 },
  { 1158, 2, 0, 1, 0, 64, 63 },
  { 1170, 2, 0, 1, 0, 64, 63 },
  { 1182, 2, 0, 1, 0, 64, 63 },
  // 573: convert_uint8, convert_uint8_rte, convert_uint8_rtn, convert_uint8_rtp, convert_uint8_rtz, convert_uint8_sat, convert_uint8_sat_rte, convert_uint8_sat_rtn, convert_uint8_sat_rtp, convert_uint8_sat_rtz, 
  { 1064, 2, 0, 1, 0, 64, 63 },
  { 1076, 2, 0, 1, 0, 64, 63 },
  { 1088, 2, 0, 1, 0, 64, 63 },
  { 1100, 2, 0, 1, 0, 64, 63 },
  { 1112, 2, 0, 1, 0, 64, 63 },
  { 1124, 2, 0, 1, 0, 64, 63 },
  { 1136, 2, 0, 1, 0, 64, 63 },
  { 1148, 2, 0, 1, 0, 64, 63 },
  { 1160, 2, 0, 1, 0, 64, 63 },
  { 1172, 2, 0, 1, 0, 64, 63 },
  { 1184, 2, 0, 1, 0, 64, 63 },
  // 584: convert_uint16, convert_uint16_rte, convert_uint16_rtn, convert_uint16_rtp, convert_uint16_rtz, convert_uint16_sat, convert_uint16_sat_rte, convert_uint16_sat_rtn, convert_uint16_sat_rtp, convert_uint16_sat_rtz, 
  { 1066, 2, 0, 1, 0, 64, 63 },
  { 1078, 2, 0, 1, 0, 64, 63 },
  { 1090, 2, 0, 1, 0, 64, 63 },
  { 1102, 2, 0, 1, 0, 64, 63 },
  { 1114, 2, 0, 1, 0, 64, 63 },
  { 1126, 2, 0, 1, 0, 64, 63 },
  { 1138, 2, 0, 1, 0, 64, 63 },
  { 1150, 2, 0, 1, 0, 64, 63 },
  { 1162, 2, 0, 1, 0, 64, 63 },
  { 1174, 2, 0, 1, 0, 64, 63 },
  { 1186, 2, 0, 1, 0, 64, 63 },
  // 595: convert_long, convert_long_rte, convert_long_rtn, convert_long_rtp, convert_long_rtz, convert_long_sat, convert_long_sat_rte, convert_long_sat_rtn, convert_long_sat_rtp, convert_long_sat_rtz, 
  { 1188, 2, 0, 1, 0, 64, 63 },
  { 1200, 2, 0, 1, 0, 64, 63 },
  { 1212, 2, 0, 1, 0, 64, 63 },
  { 1224, 2, 0, 1, 0, 64, 63 },
  { 1236, 2, 0, 1, 0, 64, 63 },
  { 1248, 2, 0, 1, 0, 64, 63 },
  { 1260, 2, 0, 1, 0, 64, 63 },
  { 1272, 2, 0, 1, 0, 64, 63 },
  { 1284, 2, 0, 1, 0, 64, 63 },
  { 1296, 2, 0, 1, 0, 64, 63 },
  { 1308, 2, 0, 1, 0, 64, 63 },
  // 606: convert_long2, convert_long2_rte, convert_long2_rtn, convert_long2_rtp, convert_long2_rtz, convert_long2_sat, convert_long2_sat_rte, convert_long2_sat_rtn, convert_long2_sat_rtp, convert_long2_sat_rtz, 
  { 1190, 2, 0, 1, 0, 64, 63 },
  { 1202, 2, 0, 1, 0, 64, 63 },
  { 1214, 2, 0, 1, 0, 64, 63 },
  { 1226, 2, 0, 1, 0, 64, 63 },
  { 1238, 2, 0, 1, 0, 64, 63 },
  { 1250, 2, 0, 1, 0, 64, 63 },
  { 1262, 2, 0, 1, 0, 64, 63 },
  { 1274, 2, 0, 1, 0, 64, 63 },
  { 1286, 2, 0, 1, 0, 64, 63 },
  { 1298, 2, 0, 1, 0, 64, 63 },
  { 1310, 2, 0, 1, 0, 64, 63 },
  // 617: convert_long3, convert_long3_rte, convert_long3_rtn, convert_long3_rtp, convert_long3_rtz, convert_long3_sat, convert_long3_sat_rte, convert_long3_sat_rtn, convert_long3_sat_rtp, convert_long3_sat_rtz, 
  { 1192, 2, 0, 1, 0, 64, 63 },
  { 1204, 2, 0, 1, 0, 64, 63 },
  { 1216, 2, 0, 1, 0, 64, 63 },
  { 1228, 2, 0, 1, 0, 64, 63 },
  { 1240, 2, 0, 1, 0, 64, 63 },
  { 1252, 2, 0, 1, 0, 64, 63 },
  { 1264, 2, 0, 1, 0, 64, 63 },
  { 1276, 2, 0, 1, 0, 64, 63 },
  { 1288, 2, 0, 1, 0, 64, 63 },
  { 1300, 2, 0, 1, 0, 64, 63 },
  { 1312, 2, 0, 1, 0, 64, 63 },
  // 628: convert_long4, convert_long4_rte, convert_long4_rtn, convert_long4_rtp, convert_long4_rtz, convert_long4_sat, convert_long4_sat_rte, convert_long4_sat_rtn, convert_long4_sat_rtp, convert_long4_sat_rtz, 
  { 1194, 2, 0, 1, 0, 64, 63 },
  { 1206, 2, 0, 1, 0, 64, 63 },
  { 1218, 2, 0, 1, 0, 64, 63 },
  { 1230, 2, 0, 1, 0, 64, 63 },
  { 1242, 2, 0, 1, 0, 64, 63 },
  { 1254, 2, 0, 1, 0, 64, 63 },
  { 1266, 2, 0, 1, 0, 64, 63 },
  { 1278, 2, 0, 1, 0, 64, 63 },
  { 1290, 2, 0, 1, 0, 64, 63 },
  { 1302, 2, 0, 1, 0, 64, 63 },
  { 1314, 2, 0, 1, 0, 64, 63 },
  // 639: convert_long8, convert_long8_rte, convert_long8_rtn, convert_long8_rtp, convert_long8_rtz, convert_long8_sat, convert_long8_sat_rte, convert_long8_sat_rtn, convert_long8_sat_rtp, convert_long8_sat_rtz, 
  { 1196, 2, 0, 1, 0, 64, 63 },
  { 1208, 2, 0, 1, 0, 64, 63 },
  { 1220, 2, 0, 1, 0, 64, 63 },
  { 1232, 2, 0, 1, 0, 64, 63 },
  { 1244, 2, 0, 1, 0, 64, 63 },
  { 1256, 2, 0, 1, 0, 64, 63 },
  { 1268, 2, 0, 1, 0, 64, 63 },
  { 1280, 2, 0, 1, 0, 64, 63 },
  { 1292, 2, 0, 1, 0, 64, 63 },
  { 1304, 2, 0, 1, 0, 64, 63 },
  { 1316, 2, 0, 1, 0, 64, 63 },
  // 650: convert_long16, convert_long16_rte, convert_long16_rtn, convert_long16_rtp, convert_long16_rtz, convert_long16_sat, convert_long16_sat_rte, convert_long16_sat_rtn, convert_long16_sat_rtp, convert_long16_sat_rtz, 
  { 1198, 2, 0, 1, 0, 64, 63 },
  { 1210, 2, 0, 1, 0, 64, 63 },
  { 1222, 2, 0, 1, 0, 64, 63 },
  { 1234, 2, 0, 1, 0, 64, 63 },
  { 1246, 2, 0, 1, 0, 64, 63 },
  { 1258, 2, 0, 1, 0, 64, 63 },
  { 1270, 2, 0, 1, 0, 64, 63 },
  { 1282, 2, 0, 1, 0, 64, 63 },
  { 1294, 2, 0, 1, 0, 64, 63 },
  { 1306, 2, 0, 1, 0, 64, 63 },
  { 1318, 2, 0, 1, 0, 64, 63 },
  // 661: convert_ulong, convert_ulong_rte, convert_ulong_rtn, convert_ulong_rtp, convert_ulong_rtz, convert_ulong_sat, convert_ulong_sat_rte, convert_ulong_sat_rtn, convert_ulong_sat_rtp, convert_ulong_sat_rtz, 
  { 1320, 2, 0, 1, 0, 64, 63 },
  { 1332, 2, 0, 1, 0, 64, 63 },
  { 1344, 2, 0, 1, 0, 64, 63 },
  { 1356, 2, 0, 1, 0, 64, 63 },
  { 1368, 2, 0, 1, 0, 64, 63 },
  { 1380, 2, 0, 1, 0, 64, 63 },
  { 1392, 2, 0, 1, 0, 64, 63 },
  { 1404, 2, 0, 1, 0, 64, 63 },
  { 1416, 2, 0, 1, 0, 64, 63 },
  { 1428, 2, 0, 1, 0, 64, 63 },
  { 1440, 2, 0, 1, 0, 64, 63 },
  // 672: convert_ulong2, convert_ulong2_rte, convert_ulong2_rtn, convert_ulong2_rtp, convert_ulong2_rtz, convert_ulong2_sat, convert_ulong2_sat_rte, convert_ulong2_sat_rtn, convert_ulong2_sat_rtp, convert_ulong2_sat_rtz, 
  { 1322, 2, 0, 1, 0, 64, 63 },
  { 1334, 2, 0, 1, 0, 64, 63 },
  { 1346, 2, 0, 1, 0, 64, 63 },
  { 1358, 2, 0, 1, 0, 64, 63 },
  { 1370, 2, 0, 1, 0, 64, 63 },
  { 1382, 2, 0, 1, 0, 64, 63 },
  { 1394, 2, 0, 1, 0, 64, 63 },
  { 1406, 2, 0, 1, 0, 64, 63 },
  { 1418, 2, 0, 1, 0, 64, 63 },
  { 1430, 2, 0, 1, 0, 64, 63 },
  { 1442, 2, 0, 1, 0, 64, 63 },
  // 683: convert_ulong3, convert_ulong3_rte, convert_ulong3_rtn, convert_ulong3_rtp, convert_ulong3_rtz, convert_ulong3_sat, convert_ulong3_sat_rte, convert_ulong3_sat_rtn, convert_ulong3_sat_rtp, convert_ulong3_sat_rtz, 
  { 1324, 2, 0, 1, 0, 64, 63 },
  { 1336, 2, 0, 1, 0, 64, 63 },
  { 1348, 2, 0, 1, 0, 64, 63 },
  { 1360, 2, 0, 1, 0, 64, 63 },
  { 1372, 2, 0, 1, 0, 64, 63 },
  { 1384, 2, 0, 1, 0, 64, 63 },
  { 1396, 2, 0, 1, 0, 64, 63 },
  { 1408, 2, 0, 1, 0, 64, 63 },
  { 1420, 2, 0, 1, 0, 64, 63 },
  { 1432, 2, 0, 1, 0, 64, 63 },
  { 1444, 2, 0, 1, 0, 64, 63 },
  // 694: convert_ulong4, convert_ulong4_rte, convert_ulong4_rtn, convert_ulong4_rtp, convert_ulong4_rtz, convert_ulong4_sat, convert_ulong4_sat_rte, convert_ulong4_sat_rtn, convert_ulong4_sat_rtp, convert_ulong4_sat_rtz, 
  { 1326, 2, 0, 1, 0, 64, 63 },
  { 1338, 2, 0, 1, 0, 64, 63 },
  { 1350, 2, 0, 1, 0, 64, 63 },
  { 1362, 2, 0, 1, 0, 64, 63 },
  { 1374, 2, 0, 1, 0, 64, 63 },
  { 1386, 2, 0, 1, 0, 64, 63 },
  { 1398, 2, 0, 1, 0, 64, 63 },
  { 1410, 2, 0, 1, 0, 64, 63 },
  { 1422, 2, 0, 1, 0, 64, 63 },
  { 1434, 2, 0, 1, 0, 64, 63 },
  { 1446, 2, 0, 1, 0, 64, 63 },
  // 705: convert_ulong8, convert_ulong8_rte, convert_ulong8_rtn, convert_ulong8_rtp, convert_ulong8_rtz, convert_ulong8_sat, convert_ulong8_sat_rte, convert_ulong8_sat_rtn, convert_ulong8_sat_rtp, convert_ulong8_sat_rtz, 
  { 1328, 2, 0, 1, 0, 64, 63 },
  { 1340, 2, 0, 1, 0, 64, 63 },
  { 1352, 2, 0, 1, 0, 64, 63 },
  { 1364, 2, 0, 1, 0, 64, 63 },
  { 1376, 2, 0, 1, 0, 64, 63 },
  { 1388, 2, 0, 1, 0, 64, 63 },
  { 1400, 2, 0, 1, 0, 64, 63 },
  { 1412, 2, 0, 1, 0, 64, 63 },
  { 1424, 2, 0, 1, 0, 64, 63 },
  { 1436, 2, 0, 1, 0, 64, 63 },
  { 1448, 2, 0, 1, 0, 64, 63 },
  // 716: convert_ulong16, convert_ulong16_rte, convert_ulong16_rtn, convert_ulong16_rtp, convert_ulong16_rtz, convert_ulong16_sat, convert_ulong16_sat_rte, convert_ulong16_sat_rtn, convert_ulong16_sat_rtp, convert_ulong16_sat_rtz, 
  { 1330, 2, 0, 1, 0, 64, 63 },
  { 1342, 2, 0, 1, 0, 64, 63 },
  { 1354, 2, 0, 1, 0, 64, 63 },
  { 1366, 2, 0, 1, 0, 64, 63 },
  { 1378, 2, 0, 1, 0, 64, 63 },
  { 1390, 2, 0, 1, 0, 64, 63 },
  { 1402, 2, 0, 1, 0, 64, 63 },
  { 1414, 2, 0, 1, 0, 64, 63 },
  { 1426, 2, 0, 1, 0, 64, 63 },
  { 1438, 2, 0, 1, 0, 64, 63 },
  { 1450, 2, 0, 1, 0, 64, 63 },
  // 727: get_work_dim, 
  { 1452, 1, 0, 1, 0, 64, 63 },
  // 728: get_global_size, get_global_id, get_local_size, get_local_id, get_num_groups, get_group_id, get_global_offset, 
  { 1453, 2, 0, 1, 0, 64, 63 },
  // 729: get_enqueued_local_size, 
  { 1453, 2, 0, 0, 0, 64, 56 },
  // 730: get_global_linear_id, get_local_linear_id, 
  { 1455, 1, 0, 0, 0, 64, 56 },
  // 731: acos, acosh, acospi, asin, asinh, asinpi, atan, atanh, atanpi, cbrt, ceil, cos, cosh, cospi, erfc, erf, exp, exp2, exp10, expm1, fabs, floor, log, log2, log10, log1p, logb, rint, round, rsqrt, sin, sinh, sinpi, sqrt, tan, tanh, tanpi, tgamma, trunc, lgamma, degrees, radians, sign, 
  { 1456, 2, 0, 1, 0, 64, 63 },
  // 732: nan, 
  { 1458, 2, 0, 1, 0, 64, 63 },
  { 1460, 2, 0, 1, 0, 64, 63 },
  { 1462, 2, 0, 1, 0, 64, 63 },
  // 735: atan2, atan2pi, copysign, fdim, fmod, hypot, maxmag, minmag, nextafter, pow, powr, remainder, 
  { 1464, 3, 0, 1, 0, 64, 63 },
  // 736: fmax, fmin, 
  { 1464, 3, 0, 1, 0, 64, 63 },
  { 1467, 3, 0, 1, 0, 64, 63 },
  { 1470, 3, 0, 1, 0, 64, 63 },
  { 1473, 3, 0, 1, 0, 64, 63 },
  // 740: ilogb, 
  { 1476, 2, 0, 1, 0, 64, 63 },
  { 1478, 2, 0, 1, 0, 64, 63 },
  { 1480, 2, 0, 1, 0, 64, 63 },
  // 743: ldexp, 
  { 1482, 3, 0, 1, 0, 64, 63 },
  { 1485, 3, 0, 1, 0, 64, 63 },
  { 1488, 3, 0, 1, 0, 64, 63 },
  { 1491, 3, 0, 1, 0, 64, 63 },
  { 1494, 3, 0, 1, 0, 64, 63 },
  { 1497, 3, 0, 1, 0, 64, 63 },
  // 749: pown, rootn, 
  { 1482, 3, 0, 1, 0, 64, 63 },
  { 1488, 3, 0, 1, 0, 64, 63 },
  { 1494, 3, 0, 1, 0, 64, 63 },
  // 752: fma, mad, 
  { 1500, 4, 0, 1, 0, 64, 63 },
  // 753: fract, modf, sincos, 
  { 1504, 3, 0, 0, 0, 67, 63 },
  { 1528, 3, 0, 0, 0, 67, 63 },
  { 1552, 3, 0, 0, 0, 67, 63 },
  { 1576, 3, 0, 0, 0, 66, 63 },
  // 757: frexp, lgamma_r, 
  { 1507, 3, 0, 0, 0, 67, 63 },
  { 1514, 3, 0, 0, 0, 67, 63 },
  { 1517, 3, 0, 0, 0, 67, 63 },
  { 1531, 3, 0, 0, 0, 67, 63 },
  { 1534, 3, 0, 0, 0, 67, 63 },
  { 1537, 3, 0, 0, 0, 67, 63 },
  { 1555, 3, 0, 0, 0, 67, 63 },
  { 1558, 3, 0, 0, 0, 67, 63 },
  { 1561, 3, 0, 0, 0, 67, 63 },
  { 1579, 3, 0, 0, 0, 66, 63 },
  { 1582, 3, 0, 0, 0, 66, 63 },
  { 1585, 3, 0, 0, 0, 66, 63 },
  // 769: remquo, 
  { 1510, 4, 0, 0, 0, 67, 63 },
  { 1520, 4, 0, 0, 0, 67, 63 },
  { 1524, 4, 0, 0, 0, 67, 63 },
  { 1540, 4, 0, 0, 0, 67, 63 },
  { 1544, 4, 0, 0, 0, 67, 63 },
  { 1548, 4, 0, 0, 0, 67, 63 },
  { 1564, 4, 0, 0, 0, 67, 63 },
  { 1568, 4, 0, 0, 0, 67, 63 },
  { 1572, 4, 0, 0, 0, 67, 63 },
  { 1588, 4, 0, 0, 0, 66, 63 },
  { 1592, 4, 0, 0, 0, 66, 63 },
  { 1596, 4, 0, 0, 0, 66, 63 },
  // 781: half_cos, half_exp, half_exp2, half_exp10, half_log, half_log2, half_log10, half_recip, half_rsqrt, half_sin, half_sqrt, half_tan, native_cos, native_exp, native_exp2, native_exp10, native_log, native_log2, native_log10, native_recip, native_rsqrt, native_sin, native_sqrt, native_tan, 
  { 1600, 2, 0, 1, 0, 64, 63 },
  // 782: half_divide, half_powr, native_divide, native_powr, 
  { 1602, 3, 0, 1, 0, 64, 63 },
  // 783: abs, 
  { 1605, 2, 0, 1, 0, 64, 63 },
  // 784: clz, 
  { 1607, 2, 0, 1, 0, 64, 63 },
  // 785: popcount, 
  { 1607, 2, 0, 1, 0, 64, 60 },
  // 786: ctz, 
  { 1607, 2, 0, 1, 0, 64, 56 },
  // 787: abs_diff, 
  { 1609, 3, 0, 1, 0, 64, 63 },
  // 788: add_sat, hadd, rhadd, mul_hi, rotate, sub_sat, 
  { 1612, 3, 0, 1, 0, 64, 63 },
  // 789: max, min, 
  { 1612, 3, 0, 1, 0, 64, 63 },
  { 1615, 3, 0, 1, 0, 64, 63 },
  { 1464, 3, 0, 1, 0, 64, 63 },
  { 1467, 3, 0, 1, 0, 64, 63 },
  { 1470, 3, 0, 1, 0, 64, 63 },
  { 1473, 3, 0, 1, 0, 64, 63 },
  // 795: upsample, 
  { 1618, 3, 0, 1, 0, 64, 63 },
  { 1621, 3, 0, 1, 0, 64, 63 },
  { 1624, 3, 0, 1, 0, 64, 63 },
  { 1627, 3, 0, 1, 0, 64, 63 },
  { 1630, 3, 0, 1, 0, 64, 63 },
  { 1633, 3, 0, 1, 0, 64, 63 },
  // 801: clamp, 
  { 1636, 4, 0, 1, 0, 64, 63 },
  { 1640, 4, 0, 1, 0, 64, 63 },
  { 1500, 4, 0, 1, 0, 64, 63 },
  { 1667, 4, 0, 1, 0, 64, 63 },
  { 1671, 4, 0, 1, 0, 64, 63 },
  { 1675, 4, 0, 1, 0, 64, 63 },
  // 807: mad_hi, mad_sat, 
  { 1636, 4, 0, 1, 0, 64, 63 },
  // 808: mad24, 
  { 1644, 4, 0, 1, 0, 64, 63 },
  { 1648, 4, 0, 1, 0, 64, 63 },
  // 810: mul24, 
  { 1652, 3, 0, 1, 0, 64, 63 },
  { 1655, 3, 0, 1, 0, 64, 63 },
  // 812: step, 
  { 1464, 3, 0, 1, 0, 64, 63 },
  { 1658, 3, 0, 1, 0, 64, 63 },
  { 1661, 3, 0, 1, 0, 64, 63 },
  { 1664, 3, 0, 1, 0, 64, 63 },
  // 816: mix, 
  { 1500, 4, 0, 1, 0, 64, 63 },
  { 1679, 4, 0, 1, 0, 64, 63 },
  { 1683, 4, 0, 1, 0, 64, 63 },
  { 1687, 4, 0, 1, 0, 64, 63 },
  // 820: smoothstep, 
  { 1500, 4, 0, 1, 0, 64, 63 },
  { 1691, 4, 0, 1, 0, 64, 63 },
  { 1695, 4, 0, 1, 0, 64, 63 },
  { 1699, 4, 0, 1, 0, 64, 63 },
  // 824: length, 
  { 1703, 2, 0, 1, 0, 64, 63 },
  { 1705, 2, 0, 1, 0, 64, 63 },
  { 1707, 2, 0, 1, 0, 64, 63 },
  // 827: normalize, 
  { 1709, 2, 0, 1, 0, 64, 63 },
  { 1711, 2, 0, 1, 0, 64, 63 },
  { 1713, 2, 0, 1, 0, 64, 63 },
  // 830: fast_length, 
  { 1703, 2, 0, 1, 0, 64, 63 },
  // 831: fast_normalize, 
  { 1709, 2, 0, 1, 0, 64, 63 },
  // 832: cross, 
  { 1715, 3, 0, 1, 0, 64, 63 },
  { 1718, 3, 0, 1, 0, 64, 63 },
  { 1721, 3, 0, 1, 0, 64, 63 },
  { 1724, 3, 0, 1, 0, 64, 63 },
  { 1727, 3, 0, 1, 0, 64, 63 },
  { 1730, 3, 0, 1, 0, 64, 63 },
  // 838: dot, 
  { 1733, 3, 0, 1, 0, 64, 63 },
  { 1736, 3, 0, 1, 0, 64, 63 },
  { 1739, 3, 0, 1, 0, 64, 63 },
  { 8080, 3, 0, 1, 0, 118, 63 },
  { 8083, 3, 0, 1, 0, 118, 63 },
  { 8086, 3, 0, 1, 0, 118, 63 },
  { 8089, 3, 0, 1, 0, 118, 63 },
  // 845: distance, 
  { 1733, 3, 0, 1, 0, 64, 63 },
  { 1736, 3, 0, 1, 0, 64, 63 },
  { 1739, 3, 0, 1, 0, 64, 63 },
  // 848: fast_distance, 
  { 1733, 3, 0, 1, 0, 64, 63 },
  // 849: isfinite, isinf, isnan, isnormal, signbit, 
  { 1476, 2, 0, 1, 0, 64, 63 },
  { 936, 2, 0, 1, 0, 64, 63 },
  { 1742, 2, 0, 1, 0, 64, 63 },
  { 948, 2, 0, 1, 0, 64, 63 },
  { 1744, 2, 0, 1, 0, 64, 63 },
  // 854: any, all, 
  { 1746, 2, 0, 1, 0, 64, 63 },
  // 855: isequal, isnotequal, isgreater, isgreaterequal, isless, islessequal, islessgreater, isordered, isunordered, 
  { 1748, 3, 0, 1, 0, 64, 63 },
  { 1751, 3, 0, 1, 0, 64, 63 },
  { 1754, 3, 0, 1, 0, 64, 63 },
  { 1757, 3, 0, 1, 0, 64, 63 },
  { 1760, 3, 0, 1, 0, 64, 63 },
  // 860: bitselect, 
  { 1763, 4, 0, 1, 0, 64, 63 },
  // 861: select, 
  { 1767, 4, 0, 1, 0, 64, 63 },
  { 1771, 4, 0, 1, 0, 64, 63 },
  { 1775, 4, 0, 1, 0, 64, 63 },
  { 1779, 4, 0, 1, 0, 64, 63 },
  { 1783, 4, 0, 1, 0, 64, 63 },
  { 1787, 4, 0, 1, 0, 64, 63 },
  { 1791, 4, 0, 1, 0, 64, 63 },
  { 1795, 4, 0, 1, 0, 64, 63 },
  { 1799, 4, 0, 1, 0, 64, 63 },
  { 1803, 4, 0, 1, 0, 64, 63 },
  // 871: vload2, 
  { 1807, 3, 1, 0, 0, 67, 63 },
  { 1810, 3, 1, 0, 0, 67, 63 },
  { 1813, 3, 1, 0, 0, 67, 63 },
  { 1816, 3, 1, 0, 0, 67, 63 },
  { 1819, 3, 1, 0, 0, 67, 63 },
  { 1822, 3, 1, 0, 0, 67, 63 },
  { 1825, 3, 1, 0, 0, 67, 63 },
  { 1828, 3, 1, 0, 0, 67, 63 },
  { 1831, 3, 1, 0, 0, 67, 63 },
  { 1834, 3, 1, 0, 0, 67, 63 },
  { 1837, 3, 1, 0, 0, 67, 63 },
  { 2192, 3, 1, 0, 0, 67, 63 },
  { 2195, 3, 1, 0, 0, 67, 63 },
  { 2198, 3, 1, 0, 0, 67, 63 },
  { 2201, 3, 1, 0, 0, 67, 63 },
  { 2204, 3, 1, 0, 0, 67, 63 },
  { 2207, 3, 1, 0, 0, 67, 63 },
  { 2210, 3, 1, 0, 0, 67, 63 },
  { 2213, 3, 1, 0, 0, 67, 63 },
  { 2216, 3, 1, 0, 0, 67, 63 },
  { 2219, 3, 1, 0, 0, 67, 63 },
  { 2222, 3, 1, 0, 0, 67, 63 },
  { 2577, 3, 1, 0, 0, 67, 63 },
  { 2580, 3, 1, 0, 0, 67, 63 },
  { 2583, 3, 1, 0, 0, 67, 63 },
  { 2586, 3, 1, 0, 0, 67, 63 },
  { 2589, 3, 1, 0, 0, 67, 63 },
  { 2592, 3, 1, 0, 0, 67, 63 },
  { 2595, 3, 1, 0, 0, 67, 63 },
  { 2598, 3, 1, 0, 0, 67, 63 },
  { 2601, 3, 1, 0, 0, 67, 63 },
  { 2604, 3, 1, 0, 0, 67, 63 },
  { 2607, 3, 1, 0, 0, 67, 63 },
  { 2962, 3, 1, 0, 0, 66, 63 },
  { 2965, 3, 1, 0, 0, 66, 63 },
  { 2968, 3, 1, 0, 0, 66, 63 },
  { 2971, 3, 1, 0, 0, 66, 63 },
  { 2974, 3, 1, 0, 0, 66, 63 },
  { 2977, 3, 1, 0, 0, 66, 63 },
  { 2980, 3, 1, 0, 0, 66, 63 },
  { 2983, 3, 1, 0, 0, 66, 63 },
  { 2986, 3, 1, 0, 0, 66, 63 },
  { 2989, 3, 1, 0, 0, 66, 63 },
  { 2992, 3, 1, 0, 0, 66, 63 },
  { 3347, 3, 1, 0, 0, 64, 63 },
  { 3350, 3, 1, 0, 0, 64, 63 },
  { 3353, 3, 1, 0, 0, 64, 63 },
  { 3356, 3, 1, 0, 0, 64, 63 },
  { 3359, 3, 1, 0, 0, 64, 63 },
  { 3362, 3, 1, 0, 0, 64, 63 },
  { 3365, 3, 1, 0, 0, 64, 63 },
  { 3368, 3, 1, 0, 0, 64, 63 },
  { 3371, 3, 1, 0, 0, 64, 63 },
  { 3374, 3, 1, 0, 0, 64, 63 },
  { 3377, 3, 1, 0, 0, 64, 63 },
  // 926: vstore2, 
  { 1840, 4, 0, 0, 0, 67, 63 },
  { 1844, 4, 0, 0, 0, 67, 63 },
  { 1848, 4, 0, 0, 0, 67, 63 },
  { 1852, 4, 0, 0, 0, 67, 63 },
  { 1856, 4, 0, 0, 0, 67, 63 },
  { 1860, 4, 0, 0, 0, 67, 63 },
  { 1864, 4, 0, 0, 0, 67, 63 },
  { 1868, 4, 0, 0, 0, 67, 63 },
  { 1872, 4, 0, 0, 0, 67, 63 },
  { 1876, 4, 0, 0, 0, 67, 63 },
  { 1880, 4, 0, 0, 0, 67, 63 },
  { 2225, 4, 0, 0, 0, 67, 63 },
  { 2229, 4, 0, 0, 0, 67, 63 },
  { 2233, 4, 0, 0, 0, 67, 63 },
  { 2237, 4, 0, 0, 0, 67, 63 },
  { 2241, 4, 0, 0, 0, 67, 63 },
  { 2245, 4, 0, 0, 0, 67, 63 },
  { 2249, 4, 0, 0, 0, 67, 63 },
  { 2253, 4, 0, 0, 0, 67, 63 },
  { 2257, 4, 0, 0, 0, 67, 63 },
  { 2261, 4, 0, 0, 0, 67, 63 },
  { 2265, 4, 0, 0, 0, 67, 63 },
  { 2610, 4, 0, 0, 0, 67, 63 },
  { 2614, 4, 0, 0, 0, 67, 63 },
  { 2618, 4, 0, 0, 0, 67, 63 },
  { 2622, 4, 0, 0, 0, 67, 63 },
  { 2626, 4, 0, 0, 0, 67, 63 },
  { 2630, 4, 0, 0, 0, 67, 63 },
  { 2634, 4, 0, 0, 0, 67, 63 },
  { 2638, 4, 0, 0, 0, 67, 63 },
  { 2642, 4, 0, 0, 0, 67, 63 },
  { 2646, 4, 0, 0, 0, 67, 63 },
  { 2650, 4, 0, 0, 0, 67, 63 },
  { 2995, 4, 0, 0, 0, 66, 63 },
  { 2999, 4, 0, 0, 0, 66, 63 },
  { 3003, 4, 0, 0, 0, 66, 63 },
  { 3007, 4, 0, 0, 0, 66, 63 },
  { 3011, 4, 0, 0, 0, 66, 63 },
  { 3015, 4, 0, 0, 0, 66, 63 },
  { 3019, 4, 0, 0, 0, 66, 63 },
  { 3023, 4, 0, 0, 0, 66, 63 },
  { 3027, 4, 0, 0, 0, 66, 63 },
  { 3031, 4, 0, 0, 0, 66, 63 },
  { 3035, 4, 0, 0, 0, 66, 63 },
  // 970: vload3, 
  { 1884, 3, 1, 0, 0, 67, 63 },
  { 1887, 3, 1, 0, 0, 67, 63 },
  { 1890, 3, 1, 0, 0, 67, 63 },
  { 1893, 3, 1, 0, 0, 67, 63 },
  { 1896, 3, 1, 0, 0, 67, 63 },
  { 1899, 3, 1, 0, 0, 67, 63 },
  { 1902, 3, 1, 0, 0, 67, 63 },
  { 1905, 3, 1, 0, 0, 67, 63 },
  { 1908, 3, 1, 0, 0, 67, 63 },
  { 1911, 3, 1, 0, 0, 67, 63 },
  { 1914, 3, 1, 0, 0, 67, 63 },
  { 2269, 3, 1, 0, 0, 67, 63 },
  { 2272, 3, 1, 0, 0, 67, 63 },
  { 2275, 3, 1, 0, 0, 67, 63 },
  { 2278, 3, 1, 0, 0, 67, 63 },
  { 2281, 3, 1, 0, 0, 67, 63 },
  { 2284, 3, 1, 0, 0, 67, 63 },
  { 2287, 3, 1, 0, 0, 67, 63 },
  { 2290, 3, 1, 0, 0, 67, 63 },
  { 2293, 3, 1, 0, 0, 67, 63 },
  { 2296, 3, 1, 0, 0, 67, 63 },
  { 2299, 3, 1, 0, 0, 67, 63 },
  { 2654, 3, 1, 0, 0, 67, 63 },
  { 2657, 3, 1, 0, 0, 67, 63 },
  { 2660, 3, 1, 0, 0, 67, 63 },
  { 2663, 3, 1, 0, 0, 67, 63 },
  { 2666, 3, 1, 0, 0, 67, 63 },
  { 2669, 3, 1, 0, 0, 67, 63 },
  { 2672, 3, 1, 0, 0, 67, 63 },
  { 2675, 3, 1, 0, 0, 67, 63 },
  { 2678, 3, 1, 0, 0, 67, 63 },
  { 2681, 3, 1, 0, 0, 67, 63 },
  { 2684, 3, 1, 0, 0, 67, 63 },
  { 3039, 3, 1, 0, 0, 66, 63 },
  { 3042, 3, 1, 0, 0, 66, 63 },
  { 3045, 3, 1, 0, 0, 66, 63 },
  { 3048, 3, 1, 0, 0, 66, 63 },
  { 3051, 3, 1, 0, 0, 66, 63 },
  { 3054, 3, 1, 0, 0, 66, 63 },
  { 3057, 3, 1, 0, 0, 66, 63 },
  { 3060, 3, 1, 0, 0, 66, 63 },
  { 3063, 3, 1, 0, 0, 66, 63 },
  { 3066, 3, 1, 0, 0, 66, 63 },
  { 3069, 3, 1, 0, 0, 66, 63 },
  { 3380, 3, 1, 0, 0, 64, 63 },
  { 3383, 3, 1, 0, 0, 64, 63 },
  { 3386, 3, 1, 0, 0, 64, 63 },
  { 3389, 3, 1, 0, 0, 64, 63 },
  { 3392, 3, 1, 0, 0, 64, 63 },
  { 3395, 3, 1, 0, 0, 64, 63 },
  { 3398, 3, 1, 0, 0, 64, 63 },
  { 3401, 3, 1, 0, 0, 64, 63 },
  { 3404, 3, 1, 0, 0, 64, 63 },
  { 3407, 3, 1, 0, 0, 64, 63 },
  { 3410, 3, 1, 0, 0, 64, 63 },
  // 1025: vstore3, 
  { 1917, 4, 0, 0, 0, 67, 63 },
  { 1921, 4, 0, 0, 0, 67, 63 },
  { 1925, 4, 0, 0, 0, 67, 63 },
  { 1929, 4, 0, 0, 0, 67, 63 },
  { 1933, 4, 0, 0, 0, 67, 63 },
  { 1937, 4, 0, 0, 0, 67, 63 },
  { 1941, 4, 0, 0, 0, 67, 63 },
  { 1945, 4, 0, 0, 0, 67, 63 },
  { 1949, 4, 0, 0, 0, 67, 63 },
  { 1953, 4, 0, 0, 0, 67, 63 },
  { 1957, 4, 0, 0, 0, 67, 63 },
  { 2302, 4, 0, 0, 0, 67, 63 },
  { 2306, 4, 0, 0, 0, 67, 63 },
  { 2310, 4, 0, 0, 0, 67, 63 },
  { 2314, 4, 0, 0, 0, 67, 63 },
  { 2318, 4, 0, 0, 0, 67, 63 },
  { 2322, 4, 0, 0, 0, 67, 63 },
  { 2326, 4, 0, 0, 0, 67, 63 },
  { 2330, 4, 0, 0, 0, 67, 63 },
  { 2334, 4, 0, 0, 0, 67, 63 },
  { 2338, 4, 0, 0, 0, 67, 63 },
  { 2342, 4, 0, 0, 0, 67, 63 },
  { 2687, 4, 0, 0, 0, 67, 63 },
  { 2691, 4, 0, 0, 0, 67, 63 },
  { 2695, 4, 0, 0, 0, 67, 63 },
  { 2699, 4, 0, 0, 0, 67, 63 },
  { 2703, 4, 0, 0, 0, 67, 63 },
  { 2707, 4, 0, 0, 0, 67, 63 },
  { 2711, 4, 0, 0, 0, 67, 63 },
  { 2715, 4, 0, 0, 0, 67, 63 },
  { 2719, 4, 0, 0, 0, 67, 63 },
  { 2723, 4, 0, 0, 0, 67, 63 },
  { 2727, 4, 0, 0, 0, 67, 63 },
  { 3072, 4, 0, 0, 0, 66, 63 },
  { 3076, 4, 0, 0, 0, 66, 63 },
  { 3080, 4, 0, 0, 0, 66, 63 },
  { 3084, 4, 0, 0, 0, 66, 63 },
  { 3088, 4, 0, 0, 0, 66, 63 },
  { 3092, 4, 0, 0, 0, 66, 63 },
  { 3096, 4, 0, 0, 0, 66, 63 },
  { 3100, 4, 0, 0, 0, 66, 63 },
  { 3104, 4, 0, 0, 0, 66, 63 },
  { 3108, 4, 0, 0, 0, 66, 63 },
  { 3112, 4, 0, 0, 0, 66, 63 },
  // 1069: vload4, 
  { 1961, 3, 1, 0, 0, 67, 63 },
  { 1964, 3, 1, 0, 0, 67, 63 },
  { 1967, 3, 1, 0, 0, 67, 63 },
  { 1970, 3, 1, 0, 0, 67, 63 },
  { 1973, 3, 1, 0, 0, 67, 63 },
  { 1976, 3, 1, 0, 0, 67, 63 },
  { 1979, 3, 1, 0, 0, 67, 63 },
  { 1982, 3, 1, 0, 0, 67, 63 },
  { 1985, 3, 1, 0, 0, 67, 63 },
  { 1988, 3, 1, 0, 0, 67, 63 },
  { 1991, 3, 1, 0, 0, 67, 63 },
  { 2346, 3, 1, 0, 0, 67, 63 },
  { 2349, 3, 1, 0, 0, 67, 63 },
  { 2352, 3, 1, 0, 0, 67, 63 },
  { 2355, 3, 1, 0, 0, 67, 63 },
  { 2358, 3, 1, 0, 0, 67, 63 },
  { 2361, 3, 1, 0, 0, 67, 63 },
  { 2364, 3, 1, 0, 0, 67, 63 },
  { 2367, 3, 1, 0, 0, 67, 63 },
  { 2370, 3, 1, 0, 0, 67, 63 },
  { 2373, 3, 1, 0, 0, 67, 63 },
  { 2376, 3, 1, 0, 0, 67, 63 },
  { 2731, 3, 1, 0, 0, 67, 63 },
  { 2734, 3, 1, 0, 0, 67, 63 },
  { 2737, 3, 1, 0, 0, 67, 63 },
  { 2740, 3, 1, 0, 0, 67, 63 },
  { 2743, 3, 1, 0, 0, 67, 63 },
  { 2746, 3, 1, 0, 0, 67, 63 },
  { 2749, 3, 1, 0, 0, 67, 63 },
  { 2752, 3, 1, 0, 0, 67, 63 },
  { 2755, 3, 1, 0, 0, 67, 63 },
  { 2758, 3, 1, 0, 0, 67, 63 },
  { 2761, 3, 1, 0, 0, 67, 63 },
  { 3116, 3, 1, 0, 0, 66, 63 },
  { 3119, 3, 1, 0, 0, 66, 63 },
  { 3122, 3, 1, 0, 0, 66, 63 },
  { 3125, 3, 1, 0, 0, 66, 63 },
  { 3128, 3, 1, 0, 0, 66, 63 },
  { 3131, 3, 1, 0, 0, 66, 63 },
  { 3134, 3, 1, 0, 0, 66, 63 },
  { 3137, 3, 1, 0, 0, 66, 63 },
  { 3140, 3, 1, 0, 0, 66, 63 },
  { 3143, 3, 1, 0, 0, 66, 63 },
  { 3146, 3, 1, 0, 0, 66, 63 },
  { 3413, 3, 1, 0, 0, 64, 63 },
  { 3416, 3, 1, 0, 0, 64, 63 },
  { 3419, 3, 1, 0, 0, 64, 63 },
  { 3422, 3, 1, 0, 0, 64, 63 },
  { 3425, 3, 1, 0, 0, 64, 63 },
  { 3428, 3, 1, 0, 0, 64, 63 },
  { 3431, 3, 1, 0, 0, 64, 63 },
  { 3434, 3, 1, 0, 0, 64, 63 },
  { 3437, 3, 1, 0, 0, 64, 63 },
  { 3440, 3, 1, 0, 0, 64, 63 },
  { 3443, 3, 1, 0, 0, 64, 63 },
  // 1124: vstore4, 
  { 1994, 4, 0, 0, 0, 67, 63 },
  { 1998, 4, 0, 0, 0, 67, 63 },
  { 2002, 4, 0, 0, 0, 67, 63 },
  { 2006, 4, 0, 0, 0, 67, 63 },
  { 2010, 4, 0, 0, 0, 67, 63 },
  { 2014, 4, 0, 0, 0, 67, 63 },
  { 2018, 4, 0, 0, 0, 67, 63 },
  { 2022, 4, 0, 0, 0, 67, 63 },
  { 2026, 4, 0, 0, 0, 67, 63 },
  { 2030, 4, 0, 0, 0, 67, 63 },
  { 2034, 4, 0, 0, 0, 67, 63 },
  { 2379, 4, 0, 0, 0, 67, 63 },
  { 2383, 4, 0, 0, 0, 67, 63 },
  { 2387, 4, 0, 0, 0, 67, 63 },
  { 2391, 4, 0, 0, 0, 67, 63 },
  { 2395, 4, 0, 0, 0, 67, 63 },
  { 2399, 4, 0, 0, 0, 67, 63 },
  { 2403, 4, 0, 0, 0, 67, 63 },
  { 2407, 4, 0, 0, 0, 67, 63 },
  { 2411, 4, 0, 0, 0, 67, 63 },
  { 2415, 4, 0, 0, 0, 67, 63 },
  { 2419, 4, 0, 0, 0, 67, 63 },
  { 2764, 4, 0, 0, 0, 67, 63 },
  { 2768, 4, 0, 0, 0, 67, 63 },
  { 2772, 4, 0, 0, 0, 67, 63 },
  { 2776, 4, 0, 0, 0, 67, 63 },
  { 2780, 4, 0, 0, 0, 67, 63 },
  { 2784, 4, 0, 0, 0, 67, 63 },
  { 2788, 4, 0, 0, 0, 67, 63 },
  { 2792, 4, 0, 0, 0, 67, 63 },
  { 2796, 4, 0, 0, 0, 67, 63 },
  { 2800, 4, 0, 0, 0, 67, 63 },
  { 2804, 4, 0, 0, 0, 67, 63 },
  { 3149, 4, 0, 0, 0, 66, 63 },
  { 3153, 4, 0, 0, 0, 66, 63 },
  { 3157, 4, 0, 0, 0, 66, 63 },
  { 3161, 4, 0, 0, 0, 66, 63 },
  { 3165, 4, 0, 0, 0, 66, 63 },
  { 3169, 4, 0, 0, 0, 66, 63 },
  { 3173, 4, 0, 0, 0, 66, 63 },
  { 3177, 4, 0, 0, 0, 66, 63 },
  { 3181, 4, 0, 0, 0, 66, 63 },
  { 3185, 4, 0, 0, 0, 66, 63 },
  { 3189, 4, 0, 0, 0, 66, 63 },
  // 1168: vload8, 
  { 2038, 3, 1, 0, 0, 67, 63 },
  { 2041, 3, 1, 0, 0, 67, 63 },
  { 2044, 3, 1, 0, 0, 67, 63 },
  { 2047, 3, 1, 0, 0, 67, 63 },
  { 2050, 3, 1, 0, 0, 67, 63 },
  { 2053, 3, 1, 0, 0, 67, 63 },
  { 2056, 3, 1, 0, 0, 67, 63 },
  { 2059, 3, 1, 0, 0, 67, 63 },
  { 2062, 3, 1, 0, 0, 67, 63 },
  { 2065, 3, 1, 0, 0, 67, 63 },
  { 2068, 3, 1, 0, 0, 67, 63 },
  { 2423, 3, 1, 0, 0, 67, 63 },
  { 2426, 3, 1, 0, 0, 67, 63 },
  { 2429, 3, 1, 0, 0, 67, 63 },
  { 2432, 3, 1, 0, 0, 67, 63 },
  { 2435, 3, 1, 0, 0, 67, 63 },
  { 2438, 3, 1, 0, 0, 67, 63 },
  { 2441, 3, 1, 0, 0, 67, 63 },
  { 2444, 3, 1, 0, 0, 67, 63 },
  { 2447, 3, 1, 0, 0, 67, 63 },
  { 2450, 3, 1, 0, 0, 67, 63 },
  { 2453, 3, 1, 0, 0, 67, 63 },
  { 2808, 3, 1, 0, 0, 67, 63 },
  { 2811, 3, 1, 0, 0, 67, 63 },
  { 2814, 3, 1, 0, 0, 67, 63 },
  { 2817, 3, 1, 0, 0, 67, 63 },
  { 2820, 3, 1, 0, 0, 67, 63 },
  { 2823, 3, 1, 0, 0, 67, 63 },
  { 2826, 3, 1, 0, 0, 67, 63 },
  { 2829, 3, 1, 0, 0, 67, 63 },
  { 2832, 3, 1, 0, 0, 67, 63 },
  { 2835, 3, 1, 0, 0, 67, 63 },
  { 2838, 3, 1, 0, 0, 67, 63 },
  { 3193, 3, 1, 0, 0, 66, 63 },
  { 3196, 3, 1, 0, 0, 66, 63 },
  { 3199, 3, 1, 0, 0, 66, 63 },
  { 3202, 3, 1, 0, 0, 66, 63 },
  { 3205, 3, 1, 0, 0, 66, 63 },
  { 3208, 3, 1, 0, 0, 66, 63 },
  { 3211, 3, 1, 0, 0, 66, 63 },
  { 3214, 3, 1, 0, 0, 66, 63 },
  { 3217, 3, 1, 0, 0, 66, 63 },
  { 3220, 3, 1, 0, 0, 66, 63 },
  { 3223, 3, 1, 0, 0, 66, 63 },
  { 3446, 3, 1, 0, 0, 64, 63 },
  { 3449, 3, 1, 0, 0, 64, 63 },
  { 3452, 3, 1, 0, 0, 64, 63 },
  { 3455, 3, 1, 0, 0, 64, 63 },
  { 3458, 3, 1, 0, 0, 64, 63 },
  { 3461, 3, 1, 0, 0, 64, 63 },
  { 3464, 3, 1, 0, 0, 64, 63 },
  { 3467, 3, 1, 0, 0, 64, 63 },
  { 3470, 3, 1, 0, 0, 64, 63 },
  { 3473, 3, 1, 0, 0, 64, 63 },
  { 3476, 3, 1, 0, 0, 64, 63 },
  // 1223: vstore8, 
  { 2071, 4, 0, 0, 0, 67, 63 },
  { 2075, 4, 0, 0, 0, 67, 63 },
  { 2079, 4, 0, 0, 0, 67, 63 },
  { 2083, 4, 0, 0, 0, 67, 63 },
  { 2087, 4, 0, 0, 0, 67, 63 },
  { 2091, 4, 0, 0, 0, 67, 63 },
  { 2095, 4, 0, 0, 0, 67, 63 },
  { 2099, 4, 0, 0, 0, 67, 63 },
  { 2103, 4, 0, 0, 0, 67, 63 },
  { 2107, 4, 0, 0, 0, 67, 63 },
  { 2111, 4, 0, 0, 0, 67, 63 },
  { 2456, 4, 0, 0, 0, 67, 63 },
  { 2460, 4, 0, 0, 0, 67, 63 },
  { 2464, 4, 0, 0, 0, 67, 63 },
  { 2468, 4, 0, 0, 0, 67, 63 },
  { 2472, 4, 0, 0, 0, 67, 63 },
  { 2476, 4, 0, 0, 0, 67, 63 },
  { 2480, 4, 0, 0, 0, 67, 63 },
  { 2484, 4, 0, 0, 0, 67, 63 },
  { 2488, 4, 0, 0, 0, 67, 63 },
  { 2492, 4, 0, 0, 0, 67, 63 },
  { 2496, 4, 0, 0, 0, 67, 63 },
  { 2841, 4, 0, 0, 0, 67, 63 },
  { 2845, 4, 0, 0, 0, 67, 63 },
  { 2849, 4, 0, 0, 0, 67, 63 },
  { 2853, 4, 0, 0, 0, 67, 63 },
  { 2857, 4, 0, 0, 0, 67, 63 },
  { 2861, 4, 0, 0, 0, 67, 63 },
  { 2865, 4, 0, 0, 0, 67, 63 },
  { 2869, 4, 0, 0, 0, 67, 63 },
  { 2873, 4, 0, 0, 0, 67, 63 },
  { 2877, 4, 0, 0, 0, 67, 63 },
  { 2881, 4, 0, 0, 0, 67, 63 },
  { 3226, 4, 0, 0, 0, 66, 63 },
  { 3230, 4, 0, 0, 0, 66, 63 },
  { 3234, 4, 0, 0, 0, 66, 63 },
  { 3238, 4, 0, 0, 0, 66, 63 },
  { 3242, 4, 0, 0, 0, 66, 63 },
  { 3246, 4, 0, 0, 0, 66, 63 },
  { 3250, 4, 0, 0, 0, 66, 63 },
  { 3254, 4, 0, 0, 0, 66, 63 },
  { 3258, 4, 0, 0, 0, 66, 63 },
  { 3262, 4, 0, 0, 0, 66, 63 },
  { 3266, 4, 0, 0, 0, 66, 63 },
  // 1267: vload16, 
  { 2115, 3, 1, 0, 0, 67, 63 },
  { 2118, 3, 1, 0, 0, 67, 63 },
  { 2121, 3, 1, 0, 0, 67, 63 },
  { 2124, 3, 1, 0, 0, 67, 63 },
  { 2127, 3, 1, 0, 0, 67, 63 },
  { 2130, 3, 1, 0, 0, 67, 63 },
  { 2133, 3, 1, 0, 0, 67, 63 },
  { 2136, 3, 1, 0, 0, 67, 63 },
  { 2139, 3, 1, 0, 0, 67, 63 },
  { 2142, 3, 1, 0, 0, 67, 63 },
  { 2145, 3, 1, 0, 0, 67, 63 },
  { 2500, 3, 1, 0, 0, 67, 63 },
  { 2503, 3, 1, 0, 0, 67, 63 },
  { 2506, 3, 1, 0, 0, 67, 63 },
  { 2509, 3, 1, 0, 0, 67, 63 },
  { 2512, 3, 1, 0, 0, 67, 63 },
  { 2515, 3, 1, 0, 0, 67, 63 },
  { 2518, 3, 1, 0, 0, 67, 63 },
  { 2521, 3, 1, 0, 0, 67, 63 },
  { 2524, 3, 1, 0, 0, 67, 63 },
  { 2527, 3, 1, 0, 0, 67, 63 },
  { 2530, 3, 1, 0, 0, 67, 63 },
  { 2885, 3, 1, 0, 0, 67, 63 },
  { 2888, 3, 1, 0, 0, 67, 63 },
  { 2891, 3, 1, 0, 0, 67, 63 },
  { 2894, 3, 1, 0, 0, 67, 63 },
  { 2897, 3, 1, 0, 0, 67, 63 },
  { 2900, 3, 1, 0, 0, 67, 63 },
  { 2903, 3, 1, 0, 0, 67, 63 },
  { 2906, 3, 1, 0, 0, 67, 63 },
  { 2909, 3, 1, 0, 0, 67, 63 },
  { 2912, 3, 1, 0, 0, 67, 63 },
  { 2915, 3, 1, 0, 0, 67, 63 },
  { 3270, 3, 1, 0, 0, 66, 63 },
  { 3273, 3, 1, 0, 0, 66, 63 },
  { 3276, 3, 1, 0, 0, 66, 63 },
  { 3279, 3, 1, 0, 0, 66, 63 },
  { 3282, 3, 1, 0, 0, 66, 63 },
  { 3285, 3, 1, 0, 0, 66, 63 },
  { 3288, 3, 1, 0, 0, 66, 63 },
  { 3291, 3, 1, 0, 0, 66, 63 },
  { 3294, 3, 1, 0, 0, 66, 63 },
  { 3297, 3, 1, 0, 0, 66, 63 },
  { 3300, 3, 1, 0, 0, 66, 63 },
  { 3479, 3, 1, 0, 0, 64, 63 },
  { 3482, 3, 1, 0, 0, 64, 63 },
  { 3485, 3, 1, 0, 0, 64, 63 },
  { 3488, 3, 1, 0, 0, 64, 63 },
  { 3491, 3, 1, 0, 0, 64, 63 },
  { 3494, 3, 1, 0, 0, 64, 63 },
  { 3497, 3, 1, 0, 0, 64, 63 },
  { 3500, 3, 1, 0, 0, 64, 63 },
  { 3503, 3, 1, 0, 0, 64, 63 },
  { 3506, 3, 1, 0, 0, 64, 63 },
  { 3509, 3, 1, 0, 0, 64, 63 },
  // 1322: vstore16, 
  { 2148, 4, 0, 0, 0, 67, 63 },
  { 2152, 4, 0, 0, 0, 67, 63 },
  { 2156, 4, 0, 0, 0, 67, 63 },
  { 2160, 4, 0, 0, 0, 67, 63 },
  { 2164, 4, 0, 0, 0, 67, 63 },
  { 2168, 4, 0, 0, 0, 67, 63 },
  { 2172, 4, 0, 0, 0, 67, 63 },
  { 2176, 4, 0, 0, 0, 67, 63 },
  { 2180, 4, 0, 0, 0, 67, 63 },
  { 2184, 4, 0, 0, 0, 67, 63 },
  { 2188, 4, 0, 0, 0, 67, 63 },
  { 2533, 4, 0, 0, 0, 67, 63 },
  { 2537, 4, 0, 0, 0, 67, 63 },
  { 2541, 4, 0, 0, 0, 67, 63 },
  { 2545, 4, 0, 0, 0, 67, 63 },
  { 2549, 4, 0, 0, 0, 67, 63 },
  { 2553, 4, 0, 0, 0, 67, 63 },
  { 2557, 4, 0, 0, 0, 67, 63 },
  { 2561, 4, 0, 0, 0, 67, 63 },
  { 2565, 4, 0, 0, 0, 67, 63 },
  { 2569, 4, 0, 0, 0, 67, 63 },
  { 2573, 4, 0, 0, 0, 67, 63 },
  { 2918, 4, 0, 0, 0, 67, 63 },
  { 2922, 4, 0, 0, 0, 67, 63 },
  { 2926, 4, 0, 0, 0, 67, 63 },
  { 2930, 4, 0, 0, 0, 67, 63 },
  { 2934, 4, 0, 0, 0, 67, 63 },
  { 2938, 4, 0, 0, 0, 67, 63 },
  { 2942, 4, 0, 0, 0, 67, 63 },
  { 2946, 4, 0, 0, 0, 67, 63 },
  { 2950, 4, 0, 0, 0, 67, 63 },
  { 2954, 4, 0, 0, 0, 67, 63 },
  { 2958, 4, 0, 0, 0, 67, 63 },
  { 3303, 4, 0, 0, 0, 66, 63 },
  { 3307, 4, 0, 0, 0, 66, 63 },
  { 3311, 4, 0, 0, 0, 66, 63 },
  { 3315, 4, 0, 0, 0, 66, 63 },
  { 3319, 4, 0, 0, 0, 66, 63 },
  { 3323, 4, 0, 0, 0, 66, 63 },
  { 3327, 4, 0, 0, 0, 66, 63 },
  { 3331, 4, 0, 0, 0, 66, 63 },
  { 3335, 4, 0, 0, 0, 66, 63 },
  { 3339, 4, 0, 0, 0, 66, 63 },
  { 3343, 4, 0, 0, 0, 66, 63 },
  // 1366: vload_half, 
  { 3512, 3, 1, 0, 0, 67, 63 },
  { 3578, 3, 1, 0, 0, 67, 63 },
  { 3644, 3, 1, 0, 0, 67, 63 },
  { 3710, 3, 1, 0, 0, 66, 63 },
  { 3776, 3, 1, 0, 0, 64, 63 },
  // 1371: vload_half2, vloada_half2, 
  { 3515, 3, 1, 0, 0, 67, 63 },
  { 3581, 3, 1, 0, 0, 67, 63 },
  { 3647, 3, 1, 0, 0, 67, 63 },
  { 3713, 3, 1, 0, 0, 66, 63 },
  { 3779, 3, 1, 0, 0, 64, 63 },
  // 1376: vstore_half, vstore_half_rte, vstore_half_rtz, vstore_half_rtp, vstore_half_rtn, 
  { 3518, 4, 0, 0, 0, 67, 63 },
  { 3522, 4, 0, 0, 0, 67, 63 },
  { 3596, 4, 0, 0, 0, 67, 63 },
  { 3600, 4, 0, 0, 0, 67, 63 },
  { 3662, 4, 0, 0, 0, 67, 63 },
  { 3666, 4, 0, 0, 0, 67, 63 },
  { 3728, 4, 0, 0, 0, 66, 63 },
  { 3732, 4, 0, 0, 0, 66, 63 },
  // 1384: vstore_half2, vstorea_half2, vstore_half2_rte, vstorea_half2_rte, vstore_half2_rtz, vstorea_half2_rtz, vstore_half2_rtp, vstorea_half2_rtp, vstore_half2_rtn, vstorea_half2_rtn, 
  { 3526, 4, 0, 0, 0, 67, 63 },
  { 3530, 4, 0, 0, 0, 67, 63 },
  { 3604, 4, 0, 0, 0, 67, 63 },
  { 3608, 4, 0, 0, 0, 67, 63 },
  { 3670, 4, 0, 0, 0, 67, 63 },
  { 3674, 4, 0, 0, 0, 67, 63 },
  { 3736, 4, 0, 0, 0, 66, 63 },
  { 3740, 4, 0, 0, 0, 66, 63 },
  // 1392: vload_half3, vloada_half3, 
  { 3534, 3, 1, 0, 0, 67, 63 },
  { 3584, 3, 1, 0, 0, 67, 63 },
  { 3650, 3, 1, 0, 0, 67, 63 },
  { 3716, 3, 1, 0, 0, 66, 63 },
  { 3782, 3, 1, 0, 0, 64, 63 },
  // 1397: vload_half4, vloada_half4, 
  { 3537, 3, 1, 0, 0, 67, 63 },
  { 3587, 3, 1, 0, 0, 67, 63 },
  { 3653, 3, 1, 0, 0, 67, 63 },
  { 3719, 3, 1, 0, 0, 66, 63 },
  { 3785, 3, 1, 0, 0, 64, 63 },
  // 1402: vload_half8, vloada_half8, 
  { 3540, 3, 1, 0, 0, 67, 63 },
  { 3590, 3, 1, 0, 0, 67, 63 },
  { 3656, 3, 1, 0, 0, 67, 63 },
  { 3722, 3, 1, 0, 0, 66, 63 },
  { 3788, 3, 1, 0, 0, 64, 63 },
  // 1407: vload_half16, vloada_half16, 
  { 3543, 3, 1, 0, 0, 67, 63 },
  { 3593, 3, 1, 0, 0, 67, 63 },
  { 3659, 3, 1, 0, 0, 67, 63 },
  { 3725, 3, 1, 0, 0, 66, 63 },
  { 3791, 3, 1, 0, 0, 64, 63 },
  // 1412: vstore_half3, vstorea_half3, vstore_half3_rte, vstorea_half3_rte, vstore_half3_rtz, vstorea_half3_rtz, vstore_half3_rtp, vstorea_half3_rtp, vstore_half3_rtn, vstorea_half3_rtn, 
  { 3546, 4, 0, 0, 0, 67, 63 },
  { 3550, 4, 0, 0, 0, 67, 63 },
  { 3612, 4, 0, 0, 0, 67, 63 },
  { 3616, 4, 0, 0, 0, 67, 63 },
  { 3678, 4, 0, 0, 0, 67, 63 },
  { 3682, 4, 0, 0, 0, 67, 63 },
  { 3744, 4, 0, 0, 0, 66, 63 },
  { 3748, 4, 0, 0, 0, 66, 63 },
  // 1420: vstore_half4, vstorea_half4, vstore_half4_rte, vstorea_half4_rte, vstore_half4_rtz, vstorea_half4_rtz, vstore_half4_rtp, vstorea_half4_rtp, vstore_half4_rtn, vstorea_half4_rtn, 
  { 3554, 4, 0, 0, 0, 67, 63 },
  { 3558, 4, 0, 0, 0, 67, 63 },
  { 3620, 4, 0, 0, 0, 67, 63 },
  { 3624, 4, 0, 0, 0, 67, 63 },
  { 3686, 4, 0, 0, 0, 67, 63 },
  { 3690, 4, 0, 0, 0, 67, 63 },
  { 3752, 4, 0, 0, 0, 66, 63 },
  { 3756, 4, 0, 0, 0, 66, 63 },
  // 1428: vstore_half8, vstorea_half8, vstore_half8_rte, vstorea_half8_rte, vstore_half8_rtz, vstorea_half8_rtz, vstore_half8_rtp, vstorea_half8_rtp, vstore_half8_rtn, vstorea_half8_rtn, 
  { 3562, 4, 0, 0, 0, 67, 63 },
  { 3566, 4, 0, 0, 0, 67, 63 },
  { 3628, 4, 0, 0, 0, 67, 63 },
  { 3632, 4, 0, 0, 0, 67, 63 },
  { 3694, 4, 0, 0, 0, 67, 63 },
  { 3698, 4, 0, 0, 0, 67, 63 },
  { 3760, 4, 0, 0, 0, 66, 63 },
  { 3764, 4, 0, 0, 0, 66, 63 },
  // 1436: vstore_half16, vstorea_half16, vstore_half16_rte, vstorea_half16_rte, vstore_half16_rtz, vstorea_half16_rtz, vstore_half16_rtp, vstorea_half16_rtp, vstore_half16_rtn, vstorea_half16_rtn, 
  { 3570, 4, 0, 0, 0, 67, 63 },
  { 3574, 4, 0, 0, 0, 67, 63 },
  { 3636, 4, 0, 0, 0, 67, 63 },
  { 3640, 4, 0, 0, 0, 67, 63 },
  { 3702, 4, 0, 0, 0, 67, 63 },
  { 3706, 4, 0, 0, 0, 67, 63 },
  { 3768, 4, 0, 0, 0, 66, 63 },
  { 3772, 4, 0, 0, 0, 66, 63 },
  // 1444: barrier, 
  { 3794, 2, 0, 0, 1, 64, 63 },
  // 1445: work_group_barrier, 
  { 3794, 2, 0, 0, 1, 64, 56 },
  { 3796, 3, 0, 0, 1, 64, 56 },
  // 1447: mem_fence, read_mem_fence, write_mem_fence, 
  { 3794, 2, 0, 0, 0, 64, 63 },
  // 1448: get_fence, 
  { 3799, 2, 0, 0, 0, 66, 63 },
  { 3801, 2, 0, 0, 0, 66, 63 },
  // 1450: async_work_group_copy, 
  { 3803, 5, 0, 0, 0, 64, 63 },
  { 3808, 5, 0, 0, 0, 64, 63 },
  // 1452: async_work_group_strided_copy, 
  { 3813, 6, 0, 0, 0, 64, 63 },
  { 3819, 6, 0, 0, 0, 64, 63 },
  // 1454: prefetch, 
  { 3825, 3, 0, 0, 0, 64, 63 },
  // 1455: wait_group_events, 
  { 3828, 3, 0, 0, 0, 67, 63 },
  { 3831, 3, 0, 0, 0, 66, 63 },
  // 1457: atom_add, atom_sub, atom_xchg, 
  { 3834, 3, 0, 0, 0, 48, 63 },
  { 3843, 3, 0, 0, 0, 48, 63 },
  { 3852, 3, 0, 0, 0, 52, 63 },
  { 3861, 3, 0, 0, 0, 52, 63 },
  { 3870, 3, 0, 0, 0, 50, 63 },
  { 3879, 3, 0, 0, 0, 50, 63 },
  { 3888, 3, 0, 0, 0, 50, 63 },
  { 3897, 3, 0, 0, 0, 50, 63 },
  // 1465: atom_inc, atom_dec, 
  { 3837, 2, 0, 0, 0, 48, 63 },
  { 3846, 2, 0, 0, 0, 48, 63 },
  { 3855, 2, 0, 0, 0, 52, 63 },
  { 3864, 2, 0, 0, 0, 52, 63 },
  { 3873, 2, 0, 0, 0, 50, 63 },
  { 3882, 2, 0, 0, 0, 50, 63 },
  { 3891, 2, 0, 0, 0, 50, 63 },
  { 3900, 2, 0, 0, 0, 50, 63 },
  // 1473: atom_cmpxchg, 
  { 3839, 4, 0, 0, 0, 48, 63 },
  { 3848, 4, 0, 0, 0, 48, 63 },
  { 3857, 4, 0, 0, 0, 52, 63 },
  { 3866, 4, 0, 0, 0, 52, 63 },
  { 3875, 4, 0, 0, 0, 50, 63 },
  { 3884, 4, 0, 0, 0, 50, 63 },
  { 3893, 4, 0, 0, 0, 50, 63 },
  { 3902, 4, 0, 0, 0, 50, 63 },
  // 1481: atom_min, atom_max, atom_and, atom_or, atom_xor, 
  { 3834, 3, 0, 0, 0, 49, 63 },
  { 3843, 3, 0, 0, 0, 49, 63 },
  { 3852, 3, 0, 0, 0, 53, 63 },
  { 3861, 3, 0, 0, 0, 53, 63 },
  { 3870, 3, 0, 0, 0, 51, 63 },
  { 3879, 3, 0, 0, 0, 51, 63 },
  { 3888, 3, 0, 0, 0, 51, 63 },
  { 3897, 3, 0, 0, 0, 51, 63 },
  // 1489: atomic_xchg, 
  { 3906, 3, 0, 0, 0, 64, 63 },
  { 3834, 3, 0, 0, 0, 64, 63 },
  { 3843, 3, 0, 0, 0, 64, 63 },
  { 3909, 3, 0, 0, 0, 64, 63 },
  { 3852, 3, 0, 0, 0, 64, 63 },
  { 3861, 3, 0, 0, 0, 64, 63 },
  { 3912, 3, 0, 0, 0, 71, 63 },
  { 3921, 3, 0, 0, 0, 71, 63 },
  // 1497: atomic_add, atomic_sub, atomic_min, atomic_max, atomic_and, atomic_or, atomic_xor, 
  { 3834, 3, 0, 0, 0, 64, 63 },
  { 3843, 3, 0, 0, 0, 64, 63 },
  { 3852, 3, 0, 0, 0, 64, 63 },
  { 3861, 3, 0, 0, 0, 64, 63 },
  { 3912, 3, 0, 0, 0, 71, 63 },
  { 3921, 3, 0, 0, 0, 71, 63 },
  // 1503: atomic_inc, atomic_dec, 
  { 3837, 2, 0, 0, 0, 64, 63 },
  { 3846, 2, 0, 0, 0, 64, 63 },
  { 3855, 2, 0, 0, 0, 64, 63 },
  { 3864, 2, 0, 0, 0, 64, 63 },
  { 3915, 2, 0, 0, 0, 71, 63 },
  { 3924, 2, 0, 0, 0, 71, 63 },
  // 1509: atomic_cmpxchg, 
  { 3839, 4, 0, 0, 0, 64, 63 },
  { 3848, 4, 0, 0, 0, 64, 63 },
  { 3857, 4, 0, 0, 0, 64, 63 },
  { 3866, 4, 0, 0, 0, 64, 63 },
  { 3917, 4, 0, 0, 0, 71, 63 },
  { 3926, 4, 0, 0, 0, 71, 63 },
  // 1515: atomic_store, 
  { 3930, 3, 0, 0, 0, 72, 56 },
  { 3984, 3, 0, 0, 0, 72, 56 },
  { 4034, 3, 0, 0, 0, 72, 56 },
  { 4084, 3, 0, 0, 0, 72, 56 },
  { 4134, 3, 0, 0, 0, 72, 56 },
  { 4184, 3, 0, 0, 0, 72, 56 },
  { 4264, 3, 0, 0, 0, 74, 56 },
  { 4348, 3, 0, 0, 0, 74, 56 },
  { 4432, 3, 0, 0, 0, 74, 56 },
  { 4516, 3, 0, 0, 0, 74, 56 },
  { 4600, 3, 0, 0, 0, 74, 56 },
  { 4684, 3, 0, 0, 0, 74, 56 },
  { 4798, 3, 0, 0, 0, 74, 56 },
  { 4882, 3, 0, 0, 0, 74, 56 },
  { 4966, 3, 0, 0, 0, 74, 56 },
  { 5050, 3, 0, 0, 0, 74, 56 },
  { 5134, 3, 0, 0, 0, 74, 56 },
  { 5218, 3, 0, 0, 0, 74, 56 },
  { 5332, 3, 0, 0, 0, 76, 56 },
  { 5365, 3, 0, 0, 0, 90, 56 },
  { 5398, 3, 0, 0, 0, 104, 56 },
  // 1536: atomic_store_explicit, 
  { 3933, 4, 0, 0, 0, 73, 56 },
  { 3937, 5, 0, 0, 0, 66, 56 },
  { 3987, 4, 0, 0, 0, 73, 56 },
  { 3991, 5, 0, 0, 0, 66, 56 },
  { 4037, 4, 0, 0, 0, 73, 56 },
  { 4041, 5, 0, 0, 0, 66, 56 },
  { 4087, 4, 0, 0, 0, 73, 56 },
  { 4091, 5, 0, 0, 0, 66, 56 },
  { 4137, 4, 0, 0, 0, 73, 56 },
  { 4141, 5, 0, 0, 0, 66, 56 },
  { 4187, 4, 0, 0, 0, 73, 56 },
  { 4191, 5, 0, 0, 0, 66, 56 },
  { 4267, 4, 0, 0, 0, 75, 56 },
  { 4271, 5, 0, 0, 0, 67, 56 },
  { 4351, 4, 0, 0, 0, 75, 56 },
  { 4355, 5, 0, 0, 0, 67, 56 },
  { 4435, 4, 0, 0, 0, 75, 56 },
  { 4439, 5, 0, 0, 0, 67, 56 },
  { 4519, 4, 0, 0, 0, 75, 56 },
  { 4523, 5, 0, 0, 0, 67, 56 },
  { 4603, 4, 0, 0, 0, 75, 56 },
  { 4607, 5, 0, 0, 0, 67, 56 },
  { 4687, 4, 0, 0, 0, 75, 56 },
  { 4691, 5, 0, 0, 0, 67, 56 },
  { 4801, 4, 0, 0, 0, 75, 56 },
  { 4805, 5, 0, 0, 0, 67, 56 },
  { 4885, 4, 0, 0, 0, 75, 56 },
  { 4889, 5, 0, 0, 0, 67, 56 },
  { 4969, 4, 0, 0, 0, 75, 56 },
  { 4973, 5, 0, 0, 0, 67, 56 },
  { 5053, 4, 0, 0, 0, 75, 56 },
  { 5057, 5, 0, 0, 0, 67, 56 },
  { 5137, 4, 0, 0, 0, 75, 56 },
  { 5141, 5, 0, 0, 0, 67, 56 },
  { 5221, 4, 0, 0, 0, 75, 56 },
  { 5225, 5, 0, 0, 0, 67, 56 },
  { 5335, 4, 0, 0, 0, 77, 56 },
  { 5339, 5, 0, 0, 0, 10, 56 },
  { 5368, 4, 0, 0, 0, 91, 56 },
  { 5372, 5, 0, 0, 0, 13, 56 },
  { 5401, 4, 0, 0, 0, 105, 56 },
  { 5405, 5, 0, 0, 0, 7, 56 },
  // 1578: atomic_init, 
  { 3930, 3, 0, 0, 0, 66, 56 },
  { 3984, 3, 0, 0, 0, 66, 56 },
  { 4034, 3, 0, 0, 0, 66, 56 },
  { 4084, 3, 0, 0, 0, 66, 56 },
  { 4134, 3, 0, 0, 0, 66, 56 },
  { 4184, 3, 0, 0, 0, 66, 56 },
  { 4264, 3, 0, 0, 0, 67, 56 },
  { 4348, 3, 0, 0, 0, 67, 56 },
  { 4432, 3, 0, 0, 0, 67, 56 },
  { 4516, 3, 0, 0, 0, 67, 56 },
  { 4600, 3, 0, 0, 0, 67, 56 },
  { 4684, 3, 0, 0, 0, 67, 56 },
  { 4798, 3, 0, 0, 0, 67, 56 },
  { 4882, 3, 0, 0, 0, 67, 56 },
  { 4966, 3, 0, 0, 0, 67, 56 },
  { 5050, 3, 0, 0, 0, 67, 56 },
  { 5134, 3, 0, 0, 0, 67, 56 },
  { 5218, 3, 0, 0, 0, 67, 56 },
  // 1596: atomic_compare_exchange_weak, atomic_compare_exchange_strong, 
  { 3942, 4, 0, 0, 0, 72, 56 },
  { 4017, 4, 0, 0, 0, 72, 56 },
  { 4067, 4, 0, 0, 0, 72, 56 },
  { 4117, 4, 0, 0, 0, 72, 56 },
  { 4167, 4, 0, 0, 0, 72, 56 },
  { 4217, 4, 0, 0, 0, 72, 56 },
  { 4297, 4, 0, 0, 0, 74, 56 },
  { 4314, 4, 0, 0, 0, 74, 56 },
  { 4331, 4, 0, 0, 0, 74, 56 },
  { 4381, 4, 0, 0, 0, 74, 56 },
  { 4398, 4, 0, 0, 0, 74, 56 },
  { 4415, 4, 0, 0, 0, 74, 56 },
  { 4465, 4, 0, 0, 0, 74, 56 },
  { 4482, 4, 0, 0, 0, 74, 56 },
  { 4499, 4, 0, 0, 0, 74, 56 },
  { 4549, 4, 0, 0, 0, 74, 56 },
  { 4566, 4, 0, 0, 0, 74, 56 },
  { 4583, 4, 0, 0, 0, 74, 56 },
  { 4633, 4, 0, 0, 0, 74, 56 },
  { 4650, 4, 0, 0, 0, 74, 56 },
  { 4667, 4, 0, 0, 0, 74, 56 },
  { 4717, 4, 0, 0, 0, 74, 56 },
  { 4734, 4, 0, 0, 0, 74, 56 },
  { 4751, 4, 0, 0, 0, 74, 56 },
  { 4831, 4, 0, 0, 0, 74, 56 },
  { 4848, 4, 0, 0, 0, 74, 56 },
  { 4865, 4, 0, 0, 0, 74, 56 },
  { 4915, 4, 0, 0, 0, 74, 56 },
  { 4932, 4, 0, 0, 0, 74, 56 },
  { 4949, 4, 0, 0, 0, 74, 56 },
  { 4999, 4, 0, 0, 0, 74, 56 },
  { 5016, 4, 0, 0, 0, 74, 56 },
  { 5033, 4, 0, 0, 0, 74, 56 },
  { 5083, 4, 0, 0, 0, 74, 56 },
  { 5100, 4, 0, 0, 0, 74, 56 },
  { 5117, 4, 0, 0, 0, 74, 56 },
  { 5167, 4, 0, 0, 0, 74, 56 },
  { 5184, 4, 0, 0, 0, 74, 56 },
  { 5201, 4, 0, 0, 0, 74, 56 },
  { 5251, 4, 0, 0, 0, 74, 56 },
  { 5268, 4, 0, 0, 0, 74, 56 },
  { 5285, 4, 0, 0, 0, 74, 56 },
  // 1638: atomic_compare_exchange_weak_explicit, atomic_compare_exchange_strong_explicit, 
  { 3946, 6, 0, 0, 0, 73, 56 },
  { 3952, 7, 0, 0, 0, 66, 56 },
  { 4021, 6, 0, 0, 0, 73, 56 },
  { 4027, 7, 0, 0, 0, 66, 56 },
  { 4071, 6, 0, 0, 0, 73, 56 },
  { 4077, 7, 0, 0, 0, 66, 56 },
  { 4121, 6, 0, 0, 0, 73, 56 },
  { 4127, 7, 0, 0, 0, 66, 56 },
  { 4171, 6, 0, 0, 0, 73, 56 },
  { 4177, 7, 0, 0, 0, 66, 56 },
  { 4221, 6, 0, 0, 0, 73, 56 },
  { 4227, 7, 0, 0, 0, 66, 56 },
  { 4301, 6, 0, 0, 0, 75, 56 },
  { 4307, 7, 0, 0, 0, 67, 56 },
  { 4318, 6, 0, 0, 0, 75, 56 },
  { 4324, 7, 0, 0, 0, 67, 56 },
  { 4335, 6, 0, 0, 0, 75, 56 },
  { 4341, 7, 0, 0, 0, 67, 56 },
  { 4385, 6, 0, 0, 0, 75, 56 },
  { 4391, 7, 0, 0, 0, 67, 56 },
  { 4402, 6, 0, 0, 0, 75, 56 },
  { 4408, 7, 0, 0, 0, 67, 56 },
  { 4419, 6, 0, 0, 0, 75, 56 },
  { 4425, 7, 0, 0, 0, 67, 56 },
  { 4469, 6, 0, 0, 0, 75, 56 },
  { 4475, 7, 0, 0, 0, 67, 56 },
  { 4486, 6, 0, 0, 0, 75, 56 },
  { 4492, 7, 0, 0, 0, 67, 56 },
  { 4503, 6, 0, 0, 0, 75, 56 },
  { 4509, 7, 0, 0, 0, 67, 56 },
  { 4553, 6, 0, 0, 0, 75, 56 },
  { 4559, 7, 0, 0, 0, 67, 56 },
  { 4570, 6, 0, 0, 0, 75, 56 },
  { 4576, 7, 0, 0, 0, 67, 56 },
  { 4587, 6, 0, 0, 0, 75, 56 },
  { 4593, 7, 0, 0, 0, 67, 56 },
  { 4637, 6, 0, 0, 0, 75, 56 },
  { 4643, 7, 0, 0, 0, 67, 56 },
  { 4654, 6, 0, 0, 0, 75, 56 },
  { 4660, 7, 0, 0, 0, 67, 56 },
  { 4671, 6, 0, 0, 0, 75, 56 },
  { 4677, 7, 0, 0, 0, 67, 56 },
  { 4721, 6, 0, 0, 0, 75, 56 },
  { 4727, 7, 0, 0, 0, 67, 56 },
  { 4738, 6, 0, 0, 0, 75, 56 },
  { 4744, 7, 0, 0, 0, 67, 56 },
  { 4755, 6, 0, 0, 0, 75, 56 },
  { 4761, 7, 0, 0, 0, 67, 56 },
  { 4835, 6, 0, 0, 0, 75, 56 },
  { 4841, 7, 0, 0, 0, 67, 56 },
  { 4852, 6, 0, 0, 0, 75, 56 },
  { 4858, 7, 0, 0, 0, 67, 56 },
  { 4869, 6, 0, 0, 0, 75, 56 },
  { 4875, 7, 0, 0, 0, 67, 56 },
  { 4919, 6, 0, 0, 0, 75, 56 },
  { 4925, 7, 0, 0, 0, 67, 56 },
  { 4936, 6, 0, 0, 0, 75, 56 },
  { 4942, 7, 0, 0, 0, 67, 56 },
  { 4953, 6, 0, 0, 0, 75, 56 },
  { 4959, 7, 0, 0, 0, 67, 56 },
  { 5003, 6, 0, 0, 0, 75, 56 },
  { 5009, 7, 0, 0, 0, 67, 56 },
  { 5020, 6, 0, 0, 0, 75, 56 },
  { 5026, 7, 0, 0, 0, 67, 56 },
  { 5037, 6, 0, 0, 0, 75, 56 },
  { 5043, 7, 0, 0, 0, 67, 56 },
  { 5087, 6, 0, 0, 0, 75, 56 },
  { 5093, 7, 0, 0, 0, 67, 56 },
  { 5104, 6, 0, 0, 0, 75, 56 },
  { 5110, 7, 0, 0, 0, 67, 56 },
  { 5121, 6, 0, 0, 0, 75, 56 },
  { 5127, 7, 0, 0, 0, 67, 56 },
  { 5171, 6, 0, 0, 0, 75, 56 },
  { 5177, 7, 0, 0, 0, 67, 56 },
  { 5188, 6, 0, 0, 0, 75, 56 },
  { 5194, 7, 0, 0, 0, 67, 56 },
  { 5205, 6, 0, 0, 0, 75, 56 },
  { 5211, 7, 0, 0, 0, 67, 56 },
  { 5255, 6, 0, 0, 0, 75, 56 },
  { 5261, 7, 0, 0, 0, 67, 56 },
  { 5272, 6, 0, 0, 0, 75, 56 },
  { 5278, 7, 0, 0, 0, 67, 56 },
  { 5289, 6, 0, 0, 0, 75, 56 },
  { 5295, 7, 0, 0, 0, 67, 56 },
  // 1722: atomic_work_item_fence, 
  { 3959, 4, 0, 0, 0, 64, 56 },
  // 1723: atomic_load, 
  { 3963, 2, 0, 0, 0, 72, 56 },
  { 3996, 2, 0, 0, 0, 72, 56 },
  { 4046, 2, 0, 0, 0, 72, 56 },
  { 4096, 2, 0, 0, 0, 72, 56 },
  { 4146, 2, 0, 0, 0, 72, 56 },
  { 4196, 2, 0, 0, 0, 72, 56 },
  { 4276, 2, 0, 0, 0, 74, 56 },
  { 4360, 2, 0, 0, 0, 74, 56 },
  { 4444, 2, 0, 0, 0, 74, 56 },
  { 4528, 2, 0, 0, 0, 74, 56 },
  { 4612, 2, 0, 0, 0, 74, 56 },
  { 4696, 2, 0, 0, 0, 74, 56 },
  { 4810, 2, 0, 0, 0, 74, 56 },
  { 4894, 2, 0, 0, 0, 74, 56 },
  { 4978, 2, 0, 0, 0, 74, 56 },
  { 5062, 2, 0, 0, 0, 74, 56 },
  { 5146, 2, 0, 0, 0, 74, 56 },
  { 5230, 2, 0, 0, 0, 74, 56 },
  { 5344, 2, 0, 0, 0, 76, 56 },
  { 5377, 2, 0, 0, 0, 90, 56 },
  { 5410, 2, 0, 0, 0, 104, 56 },
  // 1744: atomic_load_explicit, 
  { 3965, 3, 0, 0, 0, 73, 56 },
  { 3968, 4, 0, 0, 0, 66, 56 },
  { 3998, 3, 0, 0, 0, 73, 56 },
  { 4001, 4, 0, 0, 0, 66, 56 },
  { 4048, 3, 0, 0, 0, 73, 56 },
  { 4051, 4, 0, 0, 0, 66, 56 },
  { 4098, 3, 0, 0, 0, 73, 56 },
  { 4101, 4, 0, 0, 0, 66, 56 },
  { 4148, 3, 0, 0, 0, 73, 56 },
  { 4151, 4, 0, 0, 0, 66, 56 },
  { 4198, 3, 0, 0, 0, 73, 56 },
  { 4201, 4, 0, 0, 0, 66, 56 },
  { 4278, 3, 0, 0, 0, 75, 56 },
  { 4281, 4, 0, 0, 0, 67, 56 },
  { 4362, 3, 0, 0, 0, 75, 56 },
  { 4365, 4, 0, 0, 0, 67, 56 },
  { 4446, 3, 0, 0, 0, 75, 56 },
  { 4449, 4, 0, 0, 0, 67, 56 },
  { 4530, 3, 0, 0, 0, 75, 56 },
  { 4533, 4, 0, 0, 0, 67, 56 },
  { 4614, 3, 0, 0, 0, 75, 56 },
  { 4617, 4, 0, 0, 0, 67, 56 },
  { 4698, 3, 0, 0, 0, 75, 56 },
  { 4701, 4, 0, 0, 0, 67, 56 },
  { 4812, 3, 0, 0, 0, 75, 56 },
  { 4815, 4, 0, 0, 0, 67, 56 },
  { 4896, 3, 0, 0, 0, 75, 56 },
  { 4899, 4, 0, 0, 0, 67, 56 },
  { 4980, 3, 0, 0, 0, 75, 56 },
  { 4983, 4, 0, 0, 0, 67, 56 },
  { 5064, 3, 0, 0, 0, 75, 56 },
  { 5067, 4, 0, 0, 0, 67, 56 },
  { 5148, 3, 0, 0, 0, 75, 56 },
  { 5151, 4, 0, 0, 0, 67, 56 },
  { 5232, 3, 0, 0, 0, 75, 56 },
  { 5235, 4, 0, 0, 0, 67, 56 },
  { 5346, 3, 0, 0, 0, 77, 56 },
  { 5349, 4, 0, 0, 0, 10, 56 },
  { 5379, 3, 0, 0, 0, 91, 56 },
  { 5382, 4, 0, 0, 0, 13, 56 },
  { 5412, 3, 0, 0, 0, 105, 56 },
  { 5415, 4, 0, 0, 0, 7, 56 },
  // 1786: atomic_exchange, atomic_fetch_min, atomic_fetch_max, 
  { 3972, 3, 0, 0, 0, 72, 56 },
  { 4005, 3, 0, 0, 0, 72, 56 },
  { 4055, 3, 0, 0, 0, 72, 56 },
  { 4105, 3, 0, 0, 0, 72, 56 },
  { 4155, 3, 0, 0, 0, 72, 56 },
  { 4205, 3, 0, 0, 0, 72, 56 },
  { 4285, 3, 0, 0, 0, 74, 56 },
  { 4369, 3, 0, 0, 0, 74, 56 },
  { 4453, 3, 0, 0, 0, 74, 56 },
  { 4537, 3, 0, 0, 0, 74, 56 },
  { 4621, 3, 0, 0, 0, 74, 56 },
  { 4705, 3, 0, 0, 0, 74, 56 },
  { 4819, 3, 0, 0, 0, 74, 56 },
  { 4903, 3, 0, 0, 0, 74, 56 },
  { 4987, 3, 0, 0, 0, 74, 56 },
  { 5071, 3, 0, 0, 0, 74, 56 },
  { 5155, 3, 0, 0, 0, 74, 56 },
  { 5239, 3, 0, 0, 0, 74, 56 },
  { 5353, 3, 0, 0, 0, 76, 56 },
  { 5386, 3, 0, 0, 0, 90, 56 },
  { 5419, 3, 0, 0, 0, 104, 56 },
  // 1807: atomic_exchange_explicit, atomic_fetch_min_explicit, atomic_fetch_max_explicit, 
  { 3975, 4, 0, 0, 0, 73, 56 },
  { 3979, 5, 0, 0, 0, 66, 56 },
  { 4008, 4, 0, 0, 0, 73, 56 },
  { 4012, 5, 0, 0, 0, 66, 56 },
  { 4058, 4, 0, 0, 0, 73, 56 },
  { 4062, 5, 0, 0, 0, 66, 56 },
  { 4108, 4, 0, 0, 0, 73, 56 },
  { 4112, 5, 0, 0, 0, 66, 56 },
  { 4158, 4, 0, 0, 0, 73, 56 },
  { 4162, 5, 0, 0, 0, 66, 56 },
  { 4208, 4, 0, 0, 0, 73, 56 },
  { 4212, 5, 0, 0, 0, 66, 56 },
  { 4288, 4, 0, 0, 0, 75, 56 },
  { 4292, 5, 0, 0, 0, 67, 56 },
  { 4372, 4, 0, 0, 0, 75, 56 },
  { 4376, 5, 0, 0, 0, 67, 56 },
  { 4456, 4, 0, 0, 0, 75, 56 },
  { 4460, 5, 0, 0, 0, 67, 56 },
  { 4540, 4, 0, 0, 0, 75, 56 },
  { 4544, 5, 0, 0, 0, 67, 56 },
  { 4624, 4, 0, 0, 0, 75, 56 },
  { 4628, 5, 0, 0, 0, 67, 56 },
  { 4708, 4, 0, 0, 0, 75, 56 },
  { 4712, 5, 0, 0, 0, 67, 56 },
  { 4822, 4, 0, 0, 0, 75, 56 },
  { 4826, 5, 0, 0, 0, 67, 56 },
  { 4906, 4, 0, 0, 0, 75, 56 },
  { 4910, 5, 0, 0, 0, 67, 56 },
  { 4990, 4, 0, 0, 0, 75, 56 },
  { 4994, 5, 0, 0, 0, 67, 56 },
  { 5074, 4, 0, 0, 0, 75, 56 },
  { 5078, 5, 0, 0, 0, 67, 56 },
  { 5158, 4, 0, 0, 0, 75, 56 },
  { 5162, 5, 0, 0, 0, 67, 56 },
  { 5242, 4, 0, 0, 0, 75, 56 },
  { 5246, 5, 0, 0, 0, 67, 56 },
  { 5356, 4, 0, 0, 0, 77, 56 },
  { 5360, 5, 0, 0, 0, 10, 56 },
  { 5389, 4, 0, 0, 0, 91, 56 },
  { 5393, 5, 0, 0, 0, 13, 56 },
  { 5422, 4, 0, 0, 0, 105, 56 },
  { 5426, 5, 0, 0, 0, 7, 56 },
  // 1849: atomic_fetch_add, atomic_fetch_sub, 
  { 3972, 3, 0, 0, 0, 72, 56 },
  { 4005, 3, 0, 0, 0, 72, 56 },
  { 4055, 3, 0, 0, 0, 72, 56 },
  { 4105, 3, 0, 0, 0, 72, 56 },
  { 4234, 3, 0, 0, 0, 72, 56 },
  { 4285, 3, 0, 0, 0, 74, 56 },
  { 4369, 3, 0, 0, 0, 74, 56 },
  { 4453, 3, 0, 0, 0, 74, 56 },
  { 4537, 3, 0, 0, 0, 74, 56 },
  { 4768, 3, 0, 0, 0, 74, 56 },
  { 4819, 3, 0, 0, 0, 74, 56 },
  { 4903, 3, 0, 0, 0, 74, 56 },
  { 4987, 3, 0, 0, 0, 74, 56 },
  { 5071, 3, 0, 0, 0, 74, 56 },
  { 5302, 3, 0, 0, 0, 74, 56 },
  { 5353, 3, 0, 0, 0, 78, 56 },
  { 4621, 3, 0, 0, 0, 80, 56 },
  { 4705, 3, 0, 0, 0, 82, 56 },
  { 5386, 3, 0, 0, 0, 92, 56 },
  { 5155, 3, 0, 0, 0, 94, 56 },
  { 5239, 3, 0, 0, 0, 96, 56 },
  { 5419, 3, 0, 0, 0, 106, 56 },
  { 4155, 3, 0, 0, 0, 108, 56 },
  { 4205, 3, 0, 0, 0, 110, 56 },
  // 1873: atomic_fetch_add_explicit, atomic_fetch_sub_explicit, 
  { 3975, 4, 0, 0, 0, 73, 56 },
  { 3979, 5, 0, 0, 0, 66, 56 },
  { 4008, 4, 0, 0, 0, 73, 56 },
  { 4012, 5, 0, 0, 0, 66, 56 },
  { 4058, 4, 0, 0, 0, 73, 56 },
  { 4062, 5, 0, 0, 0, 66, 56 },
  { 4108, 4, 0, 0, 0, 73, 56 },
  { 4112, 5, 0, 0, 0, 66, 56 },
  { 4237, 4, 0, 0, 0, 73, 56 },
  { 4241, 5, 0, 0, 0, 66, 56 },
  { 4288, 4, 0, 0, 0, 75, 56 },
  { 4292, 5, 0, 0, 0, 67, 56 },
  { 4372, 4, 0, 0, 0, 75, 56 },
  { 4376, 5, 0, 0, 0, 67, 56 },
  { 4456, 4, 0, 0, 0, 75, 56 },
  { 4460, 5, 0, 0, 0, 67, 56 },
  { 4540, 4, 0, 0, 0, 75, 56 },
  { 4544, 5, 0, 0, 0, 67, 56 },
  { 4771, 4, 0, 0, 0, 75, 56 },
  { 4775, 5, 0, 0, 0, 67, 56 },
  { 4822, 4, 0, 0, 0, 75, 56 },
  { 4826, 5, 0, 0, 0, 67, 56 },
  { 4906, 4, 0, 0, 0, 75, 56 },
  { 4910, 5, 0, 0, 0, 67, 56 },
  { 4990, 4, 0, 0, 0, 75, 56 },
  { 4994, 5, 0, 0, 0, 67, 56 },
  { 5074, 4, 0, 0, 0, 75, 56 },
  { 5078, 5, 0, 0, 0, 67, 56 },
  { 5305, 4, 0, 0, 0, 75, 56 },
  { 5309, 5, 0, 0, 0, 67, 56 },
  { 5356, 4, 0, 0, 0, 79, 56 },
  { 5360, 5, 0, 0, 0, 9, 56 },
  { 4624, 4, 0, 0, 0, 81, 56 },
  { 4628, 5, 0, 0, 0, 17, 56 },
  { 4708, 4, 0, 0, 0, 83, 56 },
  { 4712, 5, 0, 0, 0, 23, 56 },
  { 5389, 4, 0, 0, 0, 93, 56 },
  { 5393, 5, 0, 0, 0, 12, 56 },
  { 5158, 4, 0, 0, 0, 95, 56 },
  { 5162, 5, 0, 0, 0, 19, 56 },
  { 5242, 4, 0, 0, 0, 97, 56 },
  { 5246, 5, 0, 0, 0, 25, 56 },
  { 5422, 4, 0, 0, 0, 107, 56 },
  { 5426, 5, 0, 0, 0, 6, 56 },
  { 4158, 4, 0, 0, 0, 109, 56 },
  { 4162, 5, 0, 0, 0, 15, 56 },
  { 4208, 4, 0, 0, 0, 111, 56 },
  { 4212, 5, 0, 0, 0, 21, 56 },
  // 1921: atomic_fetch_or, atomic_fetch_xor, atomic_fetch_and, 
  { 3972, 3, 0, 0, 0, 72, 56 },
  { 4005, 3, 0, 0, 0, 72, 56 },
  { 4055, 3, 0, 0, 0, 72, 56 },
  { 4105, 3, 0, 0, 0, 72, 56 },
  { 4285, 3, 0, 0, 0, 74, 56 },
  { 4369, 3, 0, 0, 0, 74, 56 },
  { 4453, 3, 0, 0, 0, 74, 56 },
  { 4537, 3, 0, 0, 0, 74, 56 },
  { 4819, 3, 0, 0, 0, 74, 56 },
  { 4903, 3, 0, 0, 0, 74, 56 },
  { 4987, 3, 0, 0, 0, 74, 56 },
  { 5071, 3, 0, 0, 0, 74, 56 },
  // 1933: atomic_fetch_or_explicit, atomic_fetch_xor_explicit, atomic_fetch_and_explicit, 
  { 3975, 4, 0, 0, 0, 73, 56 },
  { 3979, 5, 0, 0, 0, 66, 56 },
  { 4008, 4, 0, 0, 0, 73, 56 },
  { 4012, 5, 0, 0, 0, 66, 56 },
  { 4058, 4, 0, 0, 0, 73, 56 },
  { 4062, 5, 0, 0, 0, 66, 56 },
  { 4108, 4, 0, 0, 0, 73, 56 },
  { 4112, 5, 0, 0, 0, 66, 56 },
  { 4288, 4, 0, 0, 0, 75, 56 },
  { 4292, 5, 0, 0, 0, 67, 56 },
  { 4372, 4, 0, 0, 0, 75, 56 },
  { 4376, 5, 0, 0, 0, 67, 56 },
  { 4456, 4, 0, 0, 0, 75, 56 },
  { 4460, 5, 0, 0, 0, 67, 56 },
  { 4540, 4, 0, 0, 0, 75, 56 },
  { 4544, 5, 0, 0, 0, 67, 56 },
  { 4822, 4, 0, 0, 0, 75, 56 },
  { 4826, 5, 0, 0, 0, 67, 56 },
  { 4906, 4, 0, 0, 0, 75, 56 },
  { 4910, 5, 0, 0, 0, 67, 56 },
  { 4990, 4, 0, 0, 0, 75, 56 },
  { 4994, 5, 0, 0, 0, 67, 56 },
  { 5074, 4, 0, 0, 0, 75, 56 },
  { 5078, 5, 0, 0, 0, 67, 56 },
  // 1957: atomic_flag_clear, 
  { 4246, 2, 0, 0, 0, 72, 56 },
  { 4780, 2, 0, 0, 0, 74, 56 },
  { 5314, 2, 0, 0, 0, 74, 56 },
  // 1960: atomic_flag_clear_explicit, 
  { 4248, 3, 0, 0, 0, 73, 56 },
  { 4251, 4, 0, 0, 0, 66, 56 },
  { 4782, 3, 0, 0, 0, 75, 56 },
  { 4785, 4, 0, 0, 0, 67, 56 },
  { 5316, 3, 0, 0, 0, 75, 56 },
  { 5319, 4, 0, 0, 0, 67, 56 },
  // 1966: atomic_flag_test_and_set, 
  { 4255, 2, 0, 0, 0, 72, 56 },
  { 4789, 2, 0, 0, 0, 74, 56 },
  { 5323, 2, 0, 0, 0, 74, 56 },
  // 1969: atomic_flag_test_and_set_explicit, 
  { 4257, 3, 0, 0, 0, 73, 56 },
  { 4260, 4, 0, 0, 0, 66, 56 },
  { 4791, 3, 0, 0, 0, 75, 56 },
  { 4794, 4, 0, 0, 0, 67, 56 },
  { 5325, 3, 0, 0, 0, 75, 56 },
  { 5328, 4, 0, 0, 0, 67, 56 },
  // 1975: shuffle, 
  { 5431, 3, 0, 1, 0, 64, 63 },
  { 5434, 3, 0, 1, 0, 64, 63 },
  { 5437, 3, 0, 1, 0, 64, 63 },
  { 5440, 3, 0, 1, 0, 64, 63 },
  { 5443, 3, 0, 1, 0, 64, 63 },
  { 5446, 3, 0, 1, 0, 64, 63 },
  { 5449, 3, 0, 1, 0, 64, 63 },
  { 5452, 3, 0, 1, 0, 64, 63 },
  { 5455, 3, 0, 1, 0, 64, 63 },
  { 5458, 3, 0, 1, 0, 64, 63 },
  { 5461, 3, 0, 1, 0, 64, 63 },
  { 5464, 3, 0, 1, 0, 64, 63 },
  { 5467, 3, 0, 1, 0, 64, 63 },
  { 5470, 3, 0, 1, 0, 64, 63 },
  { 5473, 3, 0, 1, 0, 64, 63 },
  { 5476, 3, 0, 1, 0, 64, 63 },
  { 5479, 3, 0, 1, 0, 64, 63 },
  { 5482, 3, 0, 1, 0, 64, 63 },
  { 5485, 3, 0, 1, 0, 64, 63 },
  { 5488, 3, 0, 1, 0, 64, 63 },
  { 5491, 3, 0, 1, 0, 64, 63 },
  { 5494, 3, 0, 1, 0, 64, 63 },
  { 5497, 3, 0, 1, 0, 64, 63 },
  { 5500, 3, 0, 1, 0, 64, 63 },
  { 5503, 3, 0, 1, 0, 64, 63 },
  { 5506, 3, 0, 1, 0, 64, 63 },
  { 5509, 3, 0, 1, 0, 64, 63 },
  { 5512, 3, 0, 1, 0, 64, 63 },
  { 5515, 3, 0, 1, 0, 64, 63 },
  { 5518, 3, 0, 1, 0, 64, 63 },
  { 5521, 3, 0, 1, 0, 64, 63 },
  { 5524, 3, 0, 1, 0, 64, 63 },
  { 5527, 3, 0, 1, 0, 64, 63 },
  { 5530, 3, 0, 1, 0, 64, 63 },
  { 5533, 3, 0, 1, 0, 64, 63 },
  { 5536, 3, 0, 1, 0, 64, 63 },
  { 5539, 3, 0, 1, 0, 64, 63 },
  { 5542, 3, 0, 1, 0, 64, 63 },
  { 5545, 3, 0, 1, 0, 64, 63 },
  { 5548, 3, 0, 1, 0, 64, 63 },
  { 5551, 3, 0, 1, 0, 64, 63 },
  { 5554, 3, 0, 1, 0, 64, 63 },
  { 5557, 3, 0, 1, 0, 64, 63 },
  { 5560, 3, 0, 1, 0, 64, 63 },
  { 5563, 3, 0, 1, 0, 64, 63 },
  { 5566, 3, 0, 1, 0, 64, 63 },
  { 5569, 3, 0, 1, 0, 64, 63 },
  { 5572, 3, 0, 1, 0, 64, 63 },
  { 5575, 3, 0, 1, 0, 64, 63 },
  { 5578, 3, 0, 1, 0, 64, 63 },
  { 5581, 3, 0, 1, 0, 64, 63 },
  { 5584, 3, 0, 1, 0, 64, 63 },
  { 5587, 3, 0, 1, 0, 64, 63 },
  { 5590, 3, 0, 1, 0, 64, 63 },
  { 5593, 3, 0, 1, 0, 64, 63 },
  { 5596, 3, 0, 1, 0, 64, 63 },
  { 5599, 3, 0, 1, 0, 64, 63 },
  { 5602, 3, 0, 1, 0, 64, 63 },
  { 5605, 3, 0, 1, 0, 64, 63 },
  { 5608, 3, 0, 1, 0, 64, 63 },
  { 5611, 3, 0, 1, 0, 64, 63 },
  { 5614, 3, 0, 1, 0, 64, 63 },
  { 5617, 3, 0, 1, 0, 64, 63 },
  { 5620, 3, 0, 1, 0, 64, 63 },
  { 5623, 3, 0, 1, 0, 64, 63 },
  { 5626, 3, 0, 1, 0, 64, 63 },
  { 5629, 3, 0, 1, 0, 64, 63 },
  { 5632, 3, 0, 1, 0, 64, 63 },
  { 5635, 3, 0, 1, 0, 64, 63 },
  { 5638, 3, 0, 1, 0, 64, 63 },
  { 5641, 3, 0, 1, 0, 64, 63 },
  { 5644, 3, 0, 1, 0, 64, 63 },
  { 5647, 3, 0, 1, 0, 64, 63 },
  { 5650, 3, 0, 1, 0, 64, 63 },
  { 5653, 3, 0, 1, 0, 64, 63 },
  { 5656, 3, 0, 1, 0, 64, 63 },
  { 5659, 3, 0, 1, 0, 64, 63 },
  { 5662, 3, 0, 1, 0, 64, 63 },
  { 5665, 3, 0, 1, 0, 64, 63 },
  { 5668, 3, 0, 1, 0, 64, 63 },
  { 5671, 3, 0, 1, 0, 64, 63 },
  { 5674, 3, 0, 1, 0, 64, 63 },
  { 5677, 3, 0, 1, 0, 64, 63 },
  { 5680, 3, 0, 1, 0, 64, 63 },
  { 5683, 3, 0, 1, 0, 64, 63 },
  { 5686, 3, 0, 1, 0, 64, 63 },
  { 5689, 3, 0, 1, 0, 64, 63 },
  { 5692, 3, 0, 1, 0, 64, 63 },
  { 5695, 3, 0, 1, 0, 64, 63 },
  { 5698, 3, 0, 1, 0, 64, 63 },
  { 5701, 3, 0, 1, 0, 64, 63 },
  { 5704, 3, 0, 1, 0, 64, 63 },
  { 5707, 3, 0, 1, 0, 64, 63 },
  { 5710, 3, 0, 1, 0, 64, 63 },
  { 5713, 3, 0, 1, 0, 64, 63 },
  { 5716, 3, 0, 1, 0, 64, 63 },
  { 5719, 3, 0, 1, 0, 64, 63 },
  { 5722, 3, 0, 1, 0, 64, 63 },
  { 5725, 3, 0, 1, 0, 64, 63 },
  { 5728, 3, 0, 1, 0, 64, 63 },
  { 5731, 3, 0, 1, 0, 64, 63 },
  { 5734, 3, 0, 1, 0, 64, 63 },
  { 5737, 3, 0, 1, 0, 64, 63 },
  { 5740, 3, 0, 1, 0, 64, 63 },
  { 5743, 3, 0, 1, 0, 64, 63 },
  { 5746, 3, 0, 1, 0, 64, 63 },
  { 5749, 3, 0, 1, 0, 64, 63 },
  { 5752, 3, 0, 1, 0, 64, 63 },
  { 5755, 3, 0, 1, 0, 64, 63 },
  { 5758, 3, 0, 1, 0, 64, 63 },
  { 5761, 3, 0, 1, 0, 64, 63 },
  { 5764, 3, 0, 1, 0, 64, 63 },
  { 5767, 3, 0, 1, 0, 64, 63 },
  { 5770, 3, 0, 1, 0, 64, 63 },
  { 5773, 3, 0, 1, 0, 64, 63 },
  { 5776, 3, 0, 1, 0, 64, 63 },
  { 5779, 3, 0, 1, 0, 64, 63 },
  { 5782, 3, 0, 1, 0, 64, 63 },
  { 5785, 3, 0, 1, 0, 64, 63 },
  { 5788, 3, 0, 1, 0, 64, 63 },
  { 5791, 3, 0, 1, 0, 64, 63 },
  { 5794, 3, 0, 1, 0, 64, 63 },
  { 5797, 3, 0, 1, 0, 64, 63 },
  { 5800, 3, 0, 1, 0, 64, 63 },
  { 5803, 3, 0, 1, 0, 64, 63 },
  { 5806, 3, 0, 1, 0, 64, 63 },
  { 5809, 3, 0, 1, 0, 64, 63 },
  { 5812, 3, 0, 1, 0, 64, 63 },
  { 5815, 3, 0, 1, 0, 64, 63 },
  { 5818, 3, 0, 1, 0, 64, 63 },
  { 5821, 3, 0, 1, 0, 64, 63 },
  { 5824, 3, 0, 1, 0, 64, 63 },
  { 5827, 3, 0, 1, 0, 64, 63 },
  { 5830, 3, 0, 1, 0, 64, 63 },
  { 5833, 3, 0, 1, 0, 64, 63 },
  { 5836, 3, 0, 1, 0, 64, 63 },
  { 5839, 3, 0, 1, 0, 64, 63 },
  { 5842, 3, 0, 1, 0, 64, 63 },
  { 5845, 3, 0, 1, 0, 64, 63 },
  { 5848, 3, 0, 1, 0, 64, 63 },
  { 5851, 3, 0, 1, 0, 64, 63 },
  { 5854, 3, 0, 1, 0, 64, 63 },
  { 5857, 3, 0, 1, 0, 64, 63 },
  { 5860, 3, 0, 1, 0, 64, 63 },
  { 5863, 3, 0, 1, 0, 64, 63 },
  { 5866, 3, 0, 1, 0, 64, 63 },
  { 5869, 3, 0, 1, 0, 64, 63 },
  { 5872, 3, 0, 1, 0, 64, 63 },
  { 5875, 3, 0, 1, 0, 64, 63 },
  { 5878, 3, 0, 1, 0, 64, 63 },
  { 5881, 3, 0, 1, 0, 64, 63 },
  { 5884, 3, 0, 1, 0, 64, 63 },
  { 5887, 3, 0, 1, 0, 64, 63 },
  { 5890, 3, 0, 1, 0, 64, 63 },
  { 5893, 3, 0, 1, 0, 64, 63 },
  { 5896, 3, 0, 1, 0, 64, 63 },
  { 5899, 3, 0, 1, 0, 64, 63 },
  { 5902, 3, 0, 1, 0, 64, 63 },
  { 5905, 3, 0, 1, 0, 64, 63 },
  { 5908, 3, 0, 1, 0, 64, 63 },
  { 5911, 3, 0, 1, 0, 64, 63 },
  { 5914, 3, 0, 1, 0, 64, 63 },
  { 5917, 3, 0, 1, 0, 64, 63 },
  { 5920, 3, 0, 1, 0, 64, 63 },
  { 5923, 3, 0, 1, 0, 64, 63 },
  { 5926, 3, 0, 1, 0, 64, 63 },
  { 5929, 3, 0, 1, 0, 64, 63 },
  { 5932, 3, 0, 1, 0, 64, 63 },
  { 5935, 3, 0, 1, 0, 64, 63 },
  { 5938, 3, 0, 1, 0, 64, 63 },
  { 5941, 3, 0, 1, 0, 64, 63 },
  { 5944, 3, 0, 1, 0, 64, 63 },
  { 5947, 3, 0, 1, 0, 64, 63 },
  { 5950, 3, 0, 1, 0, 64, 63 },
  { 5953, 3, 0, 1, 0, 64, 63 },
  { 5956, 3, 0, 1, 0, 64, 63 },
  // 2151: shuffle2, 
  { 5959, 4, 0, 1, 0, 64, 63 },
  { 5963, 4, 0, 1, 0, 64, 63 },
  { 5967, 4, 0, 1, 0, 64, 63 },
  { 5971, 4, 0, 1, 0, 64, 63 },
  { 5975, 4, 0, 1, 0, 64, 63 },
  { 5979, 4, 0, 1, 0, 64, 63 },
  { 5983, 4, 0, 1, 0, 64, 63 },
  { 5987, 4, 0, 1, 0, 64, 63 },
  { 5991, 4, 0, 1, 0, 64, 63 },
  { 5995, 4, 0, 1, 0, 64, 63 },
  { 5999, 4, 0, 1, 0, 64, 63 },
  { 6003, 4, 0, 1, 0, 64, 63 },
  { 6007, 4, 0, 1, 0, 64, 63 },
  { 6011, 4, 0, 1, 0, 64, 63 },
  { 6015, 4, 0, 1, 0, 64, 63 },
  { 6019, 4, 0, 1, 0, 64, 63 },
  { 6023, 4, 0, 1, 0, 64, 63 },
  { 6027, 4, 0, 1, 0, 64, 63 },
  { 6031, 4, 0, 1, 0, 64, 63 },
  { 6035, 4, 0, 1, 0, 64, 63 },
  { 6039, 4, 0, 1, 0, 64, 63 },
  { 6043, 4, 0, 1, 0, 64, 63 },
  { 6047, 4, 0, 1, 0, 64, 63 },
  { 6051, 4, 0, 1, 0, 64, 63 },
  { 6055, 4, 0, 1, 0, 64, 63 },
  { 6059, 4, 0, 1, 0, 64, 63 },
  { 6063, 4, 0, 1, 0, 64, 63 },
  { 6067, 4, 0, 1, 0, 64, 63 },
  { 6071, 4, 0, 1, 0, 64, 63 },
  { 6075, 4, 0, 1, 0, 64, 63 },
  { 6079, 4, 0, 1, 0, 64, 63 },
  { 6083, 4, 0, 1, 0, 64, 63 },
  { 6087, 4, 0, 1, 0, 64, 63 },
  { 6091, 4, 0, 1, 0, 64, 63 },
  { 6095, 4, 0, 1, 0, 64, 63 },
  { 6099, 4, 0, 1, 0, 64, 63 },
  { 6103, 4, 0, 1, 0, 64, 63 },
  { 6107, 4, 0, 1, 0, 64, 63 },
  { 6111, 4, 0, 1, 0, 64, 63 },
  { 6115, 4, 0, 1, 0, 64, 63 },
  { 6119, 4, 0, 1, 0, 64, 63 },
  { 6123, 4, 0, 1, 0, 64, 63 },
  { 6127, 4, 0, 1, 0, 64, 63 },
  { 6131, 4, 0, 1, 0, 64, 63 },
  { 6135, 4, 0, 1, 0, 64, 63 },
  { 6139, 4, 0, 1, 0, 64, 63 },
  { 6143, 4, 0, 1, 0, 64, 63 },
  { 6147, 4, 0, 1, 0, 64, 63 },
  { 6151, 4, 0, 1, 0, 64, 63 },
  { 6155, 4, 0, 1, 0, 64, 63 },
  { 6159, 4, 0, 1, 0, 64, 63 },
  { 6163, 4, 0, 1, 0, 64, 63 },
  { 6167, 4, 0, 1, 0, 64, 63 },
  { 6171, 4, 0, 1, 0, 64, 63 },
  { 6175, 4, 0, 1, 0, 64, 63 },
  { 6179, 4, 0, 1, 0, 64, 63 },
  { 6183, 4, 0, 1, 0, 64, 63 },
  { 6187, 4, 0, 1, 0, 64, 63 },
  { 6191, 4, 0, 1, 0, 64, 63 },
  { 6195, 4, 0, 1, 0, 64, 63 },
  { 6199, 4, 0, 1, 0, 64, 63 },
  { 6203, 4, 0, 1, 0, 64, 63 },
  { 6207, 4, 0, 1, 0, 64, 63 },
  { 6211, 4, 0, 1, 0, 64, 63 },
  { 6215, 4, 0, 1, 0, 64, 63 },
  { 6219, 4, 0, 1, 0, 64, 63 },
  { 6223, 4, 0, 1, 0, 64, 63 },
  { 6227, 4, 0, 1, 0, 64, 63 },
  { 6231, 4, 0, 1, 0, 64, 63 },
  { 6235, 4, 0, 1, 0, 64, 63 },
  { 6239, 4, 0, 1, 0, 64, 63 },
  { 6243, 4, 0, 1, 0, 64, 63 },
  { 6247, 4, 0, 1, 0, 64, 63 },
  { 6251, 4, 0, 1, 0, 64, 63 },
  { 6255, 4, 0, 1, 0, 64, 63 },
  { 6259, 4, 0, 1, 0, 64, 63 },
  { 6263, 4, 0, 1, 0, 64, 63 },
  { 6267, 4, 0, 1, 0, 64, 63 },
  { 6271, 4, 0, 1, 0, 64, 63 },
  { 6275, 4, 0, 1, 0, 64, 63 },
  { 6279, 4, 0, 1, 0, 64, 63 },
  { 6283, 4, 0, 1, 0, 64, 63 },
  { 6287, 4, 0, 1, 0, 64, 63 },
  { 6291, 4, 0, 1, 0, 64, 63 },
  { 6295, 4, 0, 1, 0, 64, 63 },
  { 6299, 4, 0, 1, 0, 64, 63 },
  { 6303, 4, 0, 1, 0, 64, 63 },
  { 6307, 4, 0, 1, 0, 64, 63 },
  { 6311, 4, 0, 1, 0, 64, 63 },
  { 6315, 4, 0, 1, 0, 64, 63 },
  { 6319, 4, 0, 1, 0, 64, 63 },
  { 6323, 4, 0, 1, 0, 64, 63 },
  { 6327, 4, 0, 1, 0, 64, 63 },
  { 6331, 4, 0, 1, 0, 64, 63 },
  { 6335, 4, 0, 1, 0, 64, 63 },
  { 6339, 4, 0, 1, 0, 64, 63 },
  { 6343, 4, 0, 1, 0, 64, 63 },
  { 6347, 4, 0, 1, 0, 64, 63 },
  { 6351, 4, 0, 1, 0, 64, 63 },
  { 6355, 4, 0, 1, 0, 64, 63 },
  { 6359, 4, 0, 1, 0, 64, 63 },
  { 6363, 4, 0, 1, 0, 64, 63 },
  { 6367, 4, 0, 1, 0, 64, 63 },
  { 6371, 4, 0, 1, 0, 64, 63 },
  { 6375, 4, 0, 1, 0, 64, 63 },
  { 6379, 4, 0, 1, 0, 64, 63 },
  { 6383, 4, 0, 1, 0, 64, 63 },
  { 6387, 4, 0, 1, 0, 64, 63 },
  { 6391, 4, 0, 1, 0, 64, 63 },
  { 6395, 4, 0, 1, 0, 64, 63 },
  { 6399, 4, 0, 1, 0, 64, 63 },
  { 6403, 4, 0, 1, 0, 64, 63 },
  { 6407, 4, 0, 1, 0, 64, 63 },
  { 6411, 4, 0, 1, 0, 64, 63 },
  { 6415, 4, 0, 1, 0, 64, 63 },
  { 6419, 4, 0, 1, 0, 64, 63 },
  { 6423, 4, 0, 1, 0, 64, 63 },
  { 6427, 4, 0, 1, 0, 64, 63 },
  { 6431, 4, 0, 1, 0, 64, 63 },
  { 6435, 4, 0, 1, 0, 64, 63 },
  { 6439, 4, 0, 1, 0, 64, 63 },
  { 6443, 4, 0, 1, 0, 64, 63 },
  { 6447, 4, 0, 1, 0, 64, 63 },
  { 6451, 4, 0, 1, 0, 64, 63 },
  { 6455, 4, 0, 1, 0, 64, 63 },
  { 6459, 4, 0, 1, 0, 64, 63 },
  { 6463, 4, 0, 1, 0, 64, 63 },
  { 6467, 4, 0, 1, 0, 64, 63 },
  { 6471, 4, 0, 1, 0, 64, 63 },
  { 6475, 4, 0, 1, 0, 64, 63 },
  { 6479, 4, 0, 1, 0, 64, 63 },
  { 6483, 4, 0, 1, 0, 64, 63 },
  { 6487, 4, 0, 1, 0, 64, 63 },
  { 6491, 4, 0, 1, 0, 64, 63 },
  { 6495, 4, 0, 1, 0, 64, 63 },
  { 6499, 4, 0, 1, 0, 64, 63 },
  { 6503, 4, 0, 1, 0, 64, 63 },
  { 6507, 4, 0, 1, 0, 64, 63 },
  { 6511, 4, 0, 1, 0, 64, 63 },
  { 6515, 4, 0, 1, 0, 64, 63 },
  { 6519, 4, 0, 1, 0, 64, 63 },
  { 6523, 4, 0, 1, 0, 64, 63 },
  { 6527, 4, 0, 1, 0, 64, 63 },
  { 6531, 4, 0, 1, 0, 64, 63 },
  { 6535, 4, 0, 1, 0, 64, 63 },
  { 6539, 4, 0, 1, 0, 64, 63 },
  { 6543, 4, 0, 1, 0, 64, 63 },
  { 6547, 4, 0, 1, 0, 64, 63 },
  { 6551, 4, 0, 1, 0, 64, 63 },
  { 6555, 4, 0, 1, 0, 64, 63 },
  { 6559, 4, 0, 1, 0, 64, 63 },
  { 6563, 4, 0, 1, 0, 64, 63 },
  { 6567, 4, 0, 1, 0, 64, 63 },
  { 6571, 4, 0, 1, 0, 64, 63 },
  { 6575, 4, 0, 1, 0, 64, 63 },
  { 6579, 4, 0, 1, 0, 64, 63 },
  { 6583, 4, 0, 1, 0, 64, 63 },
  { 6587, 4, 0, 1, 0, 64, 63 },
  { 6591, 4, 0, 1, 0, 64, 63 },
  { 6595, 4, 0, 1, 0, 64, 63 },
  { 6599, 4, 0, 1, 0, 64, 63 },
  { 6603, 4, 0, 1, 0, 64, 63 },
  { 6607, 4, 0, 1, 0, 64, 63 },
  { 6611, 4, 0, 1, 0, 64, 63 },
  { 6615, 4, 0, 1, 0, 64, 63 },
  { 6619, 4, 0, 1, 0, 64, 63 },
  { 6623, 4, 0, 1, 0, 64, 63 },
  { 6627, 4, 0, 1, 0, 64, 63 },
  { 6631, 4, 0, 1, 0, 64, 63 },
  { 6635, 4, 0, 1, 0, 64, 63 },
  { 6639, 4, 0, 1, 0, 64, 63 },
  { 6643, 4, 0, 1, 0, 64, 63 },
  { 6647, 4, 0, 1, 0, 64, 63 },
  { 6651, 4, 0, 1, 0, 64, 63 },
  { 6655, 4, 0, 1, 0, 64, 63 },
  { 6659, 4, 0, 1, 0, 64, 63 },
  // 2327: read_imagef, 
  { 6663, 4, 1, 0, 0, 64, 63 },
  { 6675, 4, 1, 0, 0, 64, 63 },
  { 6687, 4, 1, 0, 0, 64, 63 },
  { 6699, 4, 1, 0, 0, 64, 63 },
  { 6711, 4, 1, 0, 0, 64, 63 },
  { 6723, 4, 1, 0, 0, 64, 63 },
  { 6735, 4, 1, 0, 0, 64, 63 },
  { 6747, 4, 1, 0, 0, 64, 63 },
  { 6759, 4, 1, 0, 0, 64, 63 },
  { 6771, 4, 1, 0, 0, 64, 63 },
  { 6783, 4, 1, 0, 0, 64, 63 },
  { 6787, 4, 1, 0, 0, 64, 63 },
  { 6791, 4, 1, 0, 0, 64, 63 },
  { 6795, 4, 1, 0, 0, 64, 63 },
  { 6799, 3, 1, 0, 0, 64, 60 },
  { 6808, 3, 1, 0, 0, 64, 60 },
  { 6817, 3, 1, 0, 0, 64, 60 },
  { 6826, 3, 1, 0, 0, 64, 60 },
  { 6829, 3, 1, 0, 0, 64, 60 },
  { 6832, 3, 1, 0, 0, 64, 60 },
  { 6841, 3, 1, 0, 0, 64, 60 },
  { 6850, 3, 1, 0, 0, 64, 60 },
  { 6859, 3, 1, 0, 0, 69, 60 },
  { 6868, 3, 1, 0, 0, 69, 60 },
  { 6877, 3, 1, 0, 0, 69, 60 },
  { 6886, 3, 1, 0, 0, 69, 60 },
  { 6895, 3, 1, 0, 0, 69, 60 },
  { 6904, 3, 1, 0, 0, 69, 60 },
  { 6913, 3, 1, 0, 0, 69, 60 },
  { 6916, 3, 1, 0, 0, 69, 60 },
  { 7351, 5, 1, 0, 0, 54, 63 },
  { 7356, 6, 1, 0, 0, 54, 63 },
  { 7384, 5, 1, 0, 0, 54, 63 },
  { 7389, 6, 1, 0, 0, 54, 63 },
  { 7395, 5, 1, 0, 0, 54, 63 },
  { 7400, 6, 1, 0, 0, 54, 63 },
  { 7428, 6, 1, 0, 0, 54, 63 },
  { 7434, 5, 1, 0, 0, 54, 63 },
  { 7461, 5, 1, 0, 0, 54, 63 },
  { 7466, 6, 1, 0, 0, 54, 63 },
  { 7494, 5, 1, 0, 0, 54, 63 },
  { 7499, 6, 1, 0, 0, 54, 63 },
  { 7527, 5, 1, 0, 0, 54, 63 },
  { 7532, 6, 1, 0, 0, 54, 63 },
  { 7538, 5, 1, 0, 0, 54, 63 },
  { 7543, 6, 1, 0, 0, 54, 63 },
  { 7571, 5, 1, 0, 0, 54, 63 },
  { 7576, 6, 1, 0, 0, 54, 63 },
  { 7582, 5, 1, 0, 0, 54, 63 },
  { 7587, 6, 1, 0, 0, 54, 63 },
  { 7615, 6, 1, 0, 0, 54, 63 },
  { 7621, 5, 1, 0, 0, 54, 63 },
  { 7648, 5, 1, 0, 0, 54, 63 },
  { 7653, 6, 1, 0, 0, 54, 63 },
  { 7681, 5, 1, 0, 0, 54, 63 },
  { 7686, 6, 1, 0, 0, 54, 63 },
  { 7714, 5, 1, 0, 0, 54, 63 },
  { 7719, 6, 1, 0, 0, 54, 63 },
  { 7895, 4, 1, 0, 0, 47, 63 },
  { 7907, 4, 1, 0, 0, 47, 63 },
  { 7919, 4, 1, 0, 0, 47, 63 },
  { 7923, 4, 1, 0, 0, 47, 63 },
  { 7967, 4, 1, 0, 0, 47, 63 },
  { 7979, 4, 1, 0, 0, 47, 63 },
  { 7991, 4, 1, 0, 0, 47, 63 },
  { 7995, 4, 1, 0, 0, 47, 63 },
  // 2393: read_imagei, 
  { 6667, 4, 1, 0, 0, 64, 63 },
  { 6679, 4, 1, 0, 0, 64, 63 },
  { 6691, 4, 1, 0, 0, 64, 63 },
  { 6703, 4, 1, 0, 0, 64, 63 },
  { 6715, 4, 1, 0, 0, 64, 63 },
  { 6727, 4, 1, 0, 0, 64, 63 },
  { 6739, 4, 1, 0, 0, 64, 63 },
  { 6751, 4, 1, 0, 0, 64, 63 },
  { 6763, 4, 1, 0, 0, 64, 63 },
  { 6775, 4, 1, 0, 0, 64, 63 },
  { 6802, 3, 1, 0, 0, 64, 60 },
  { 6811, 3, 1, 0, 0, 64, 60 },
  { 6820, 3, 1, 0, 0, 64, 60 },
  { 6835, 3, 1, 0, 0, 64, 60 },
  { 6844, 3, 1, 0, 0, 64, 60 },
  { 6853, 3, 1, 0, 0, 64, 60 },
  { 6862, 3, 1, 0, 0, 69, 60 },
  { 6871, 3, 1, 0, 0, 69, 60 },
  { 6880, 3, 1, 0, 0, 69, 60 },
  { 6889, 3, 1, 0, 0, 69, 60 },
  { 6898, 3, 1, 0, 0, 69, 60 },
  { 6907, 3, 1, 0, 0, 69, 60 },
  { 7362, 5, 1, 0, 0, 54, 63 },
  { 7367, 6, 1, 0, 0, 54, 63 },
  { 7406, 5, 1, 0, 0, 54, 63 },
  { 7411, 6, 1, 0, 0, 54, 63 },
  { 7439, 6, 1, 0, 0, 54, 63 },
  { 7445, 5, 1, 0, 0, 54, 63 },
  { 7472, 5, 1, 0, 0, 54, 63 },
  { 7477, 6, 1, 0, 0, 54, 63 },
  { 7505, 5, 1, 0, 0, 54, 63 },
  { 7510, 6, 1, 0, 0, 54, 63 },
  { 7549, 5, 1, 0, 0, 54, 63 },
  { 7554, 6, 1, 0, 0, 54, 63 },
  { 7593, 5, 1, 0, 0, 54, 63 },
  { 7598, 6, 1, 0, 0, 54, 63 },
  { 7626, 6, 1, 0, 0, 54, 63 },
  { 7632, 5, 1, 0, 0, 54, 63 },
  { 7659, 5, 1, 0, 0, 54, 63 },
  { 7664, 6, 1, 0, 0, 54, 63 },
  { 7692, 5, 1, 0, 0, 54, 63 },
  { 7697, 6, 1, 0, 0, 54, 63 },
  { 7899, 4, 1, 0, 0, 47, 63 },
  { 7911, 4, 1, 0, 0, 47, 63 },
  { 7971, 4, 1, 0, 0, 47, 63 },
  { 7983, 4, 1, 0, 0, 47, 63 },
  // 2439: read_imageui, 
  { 6671, 4, 1, 0, 0, 64, 63 },
  { 6683, 4, 1, 0, 0, 64, 63 },
  { 6695, 4, 1, 0, 0, 64, 63 },
  { 6707, 4, 1, 0, 0, 64, 63 },
  { 6719, 4, 1, 0, 0, 64, 63 },
  { 6731, 4, 1, 0, 0, 64, 63 },
  { 6743, 4, 1, 0, 0, 64, 63 },
  { 6755, 4, 1, 0, 0, 64, 63 },
  { 6767, 4, 1, 0, 0, 64, 63 },
  { 6779, 4, 1, 0, 0, 64, 63 },
  { 6805, 3, 1, 0, 0, 64, 60 },
  { 6814, 3, 1, 0, 0, 64, 60 },
  { 6823, 3, 1, 0, 0, 64, 60 },
  { 6838, 3, 1, 0, 0, 64, 60 },
  { 6847, 3, 1, 0, 0, 64, 60 },
  { 6856, 3, 1, 0, 0, 64, 60 },
  { 6865, 3, 1, 0, 0, 69, 60 },
  { 6874, 3, 1, 0, 0, 69, 60 },
  { 6883, 3, 1, 0, 0, 69, 60 },
  { 6892, 3, 1, 0, 0, 69, 60 },
  { 6901, 3, 1, 0, 0, 69, 60 },
  { 6910, 3, 1, 0, 0, 69, 60 },
  { 7373, 5, 1, 0, 0, 54, 63 },
  { 7378, 6, 1, 0, 0, 54, 63 },
  { 7417, 5, 1, 0, 0, 54, 63 },
  { 7422, 6, 1, 0, 0, 54, 63 },
  { 7450, 6, 1, 0, 0, 54, 63 },
  { 7456, 5, 1, 0, 0, 54, 63 },
  { 7483, 5, 1, 0, 0, 54, 63 },
  { 7488, 6, 1, 0, 0, 54, 63 },
  { 7516, 5, 1, 0, 0, 54, 63 },
  { 7521, 6, 1, 0, 0, 54, 63 },
  { 7560, 5, 1, 0, 0, 54, 63 },
  { 7565, 6, 1, 0, 0, 54, 63 },
  { 7604, 5, 1, 0, 0, 54, 63 },
  { 7609, 6, 1, 0, 0, 54, 63 },
  { 7637, 6, 1, 0, 0, 54, 63 },
  { 7643, 5, 1, 0, 0, 54, 63 },
  { 7670, 5, 1, 0, 0, 54, 63 },
  { 7675, 6, 1, 0, 0, 54, 63 },
  { 7703, 5, 1, 0, 0, 54, 63 },
  { 7708, 6, 1, 0, 0, 54, 63 },
  { 7903, 4, 1, 0, 0, 47, 63 },
  { 7915, 4, 1, 0, 0, 47, 63 },
  { 7975, 4, 1, 0, 0, 47, 63 },
  { 7987, 4, 1, 0, 0, 47, 63 },
  // 2485: write_imagef, 
  { 6919, 4, 0, 0, 0, 64, 63 },
  { 6931, 4, 0, 0, 0, 64, 63 },
  { 6943, 4, 0, 0, 0, 64, 63 },
  { 6955, 4, 0, 0, 0, 64, 63 },
  { 6967, 4, 0, 0, 0, 64, 63 },
  { 6979, 4, 0, 0, 0, 64, 63 },
  { 6983, 4, 0, 0, 0, 64, 63 },
  { 6987, 4, 0, 0, 0, 64, 63 },
  { 6999, 4, 0, 0, 0, 69, 63 },
  { 7011, 4, 0, 0, 0, 69, 63 },
  { 7023, 4, 0, 0, 0, 69, 63 },
  { 7035, 4, 0, 0, 0, 69, 63 },
  { 7047, 4, 0, 0, 0, 69, 63 },
  { 7059, 4, 0, 0, 0, 69, 63 },
  { 7071, 4, 0, 0, 0, 69, 63 },
  { 7075, 4, 0, 0, 0, 69, 63 },
  { 7725, 5, 0, 0, 0, 55, 63 },
  { 7740, 5, 0, 0, 0, 55, 63 },
  { 7745, 5, 0, 0, 0, 55, 63 },
  { 7760, 5, 0, 0, 0, 55, 63 },
  { 7775, 5, 0, 0, 0, 55, 63 },
  { 7790, 5, 0, 0, 0, 55, 63 },
  { 7795, 5, 0, 0, 0, 55, 63 },
  { 7810, 5, 0, 0, 0, 55, 63 },
  { 7825, 5, 0, 0, 0, 55, 63 },
  { 7830, 5, 0, 0, 0, 55, 63 },
  { 7845, 5, 0, 0, 0, 55, 63 },
  { 7860, 5, 0, 0, 0, 55, 63 },
  { 7875, 5, 0, 0, 0, 55, 63 },
  { 7880, 5, 0, 0, 0, 55, 63 },
  // 2515: write_imagei, 
  { 6923, 4, 0, 0, 0, 64, 63 },
  { 6935, 4, 0, 0, 0, 64, 63 },
  { 6947, 4, 0, 0, 0, 64, 63 },
  { 6959, 4, 0, 0, 0, 64, 63 },
  { 6971, 4, 0, 0, 0, 64, 63 },
  { 6991, 4, 0, 0, 0, 64, 63 },
  { 7003, 4, 0, 0, 0, 69, 63 },
  { 7015, 4, 0, 0, 0, 69, 63 },
  { 7027, 4, 0, 0, 0, 69, 63 },
  { 7039, 4, 0, 0, 0, 69, 63 },
  { 7051, 4, 0, 0, 0, 69, 63 },
  { 7063, 4, 0, 0, 0, 69, 63 },
  { 7730, 5, 0, 0, 0, 55, 63 },
  { 7750, 5, 0, 0, 0, 55, 63 },
  { 7765, 5, 0, 0, 0, 55, 63 },
  { 7780, 5, 0, 0, 0, 55, 63 },
  { 7800, 5, 0, 0, 0, 55, 63 },
  { 7815, 5, 0, 0, 0, 55, 63 },
  { 7835, 5, 0, 0, 0, 55, 63 },
  { 7850, 5, 0, 0, 0, 55, 63 },
  { 7865, 5, 0, 0, 0, 55, 63 },
  { 7885, 5, 0, 0, 0, 55, 63 },
  // 2537: write_imageui, 
  { 6927, 4, 0, 0, 0, 64, 63 },
  { 6939, 4, 0, 0, 0, 64, 63 },
  { 6951, 4, 0, 0, 0, 64, 63 },
  { 6963, 4, 0, 0, 0, 64, 63 },
  { 6975, 4, 0, 0, 0, 64, 63 },
  { 6995, 4, 0, 0, 0, 64, 63 },
  { 7007, 4, 0, 0, 0, 69, 63 },
  { 7019, 4, 0, 0, 0, 69, 63 },
  { 7031, 4, 0, 0, 0, 69, 63 },
  { 7043, 4, 0, 0, 0, 69, 63 },
  { 7055, 4, 0, 0, 0, 69, 63 },
  { 7067, 4, 0, 0, 0, 69, 63 },
  { 7735, 5, 0, 0, 0, 55, 63 },
  { 7755, 5, 0, 0, 0, 55, 63 },
  { 7770, 5, 0, 0, 0, 55, 63 },
  { 7785, 5, 0, 0, 0, 55, 63 },
  { 7805, 5, 0, 0, 0, 55, 63 },
  { 7820, 5, 0, 0, 0, 55, 63 },
  { 7840, 5, 0, 0, 0, 55, 63 },
  { 7855, 5, 0, 0, 0, 55, 63 },
  { 7870, 5, 0, 0, 0, 55, 63 },
  { 7890, 5, 0, 0, 0, 55, 63 },
  // 2559: get_image_width, get_image_channel_data_type, get_image_channel_order, 
  { 7079, 2, 0, 1, 0, 64, 63 },
  { 7091, 2, 0, 1, 0, 64, 63 },
  { 7081, 2, 0, 1, 0, 64, 63 },
  { 7083, 2, 0, 1, 0, 64, 63 },
  { 7093, 2, 0, 1, 0, 64, 63 },
  { 7095, 2, 0, 1, 0, 64, 63 },
  { 7097, 2, 0, 1, 0, 64, 63 },
  { 7099, 2, 0, 1, 0, 64, 63 },
  { 7111, 2, 0, 1, 0, 64, 63 },
  { 7113, 2, 0, 1, 0, 64, 63 },
  { 7115, 2, 0, 1, 0, 64, 63 },
  { 7117, 2, 0, 1, 0, 64, 63 },
  { 7119, 2, 0, 1, 0, 64, 63 },
  { 7121, 2, 0, 1, 0, 64, 63 },
  { 7123, 2, 0, 1, 0, 64, 63 },
  { 7125, 2, 0, 1, 0, 64, 63 },
  { 7143, 2, 0, 1, 0, 69, 63 },
  { 7145, 2, 0, 1, 0, 69, 63 },
  { 7147, 2, 0, 1, 0, 69, 63 },
  { 7149, 2, 0, 1, 0, 69, 63 },
  { 7151, 2, 0, 1, 0, 69, 63 },
  { 7153, 2, 0, 1, 0, 69, 63 },
  { 7155, 2, 0, 1, 0, 69, 63 },
  { 7157, 2, 0, 1, 0, 69, 63 },
  { 7927, 2, 0, 1, 0, 47, 63 },
  { 7933, 2, 0, 1, 0, 47, 63 },
  { 7937, 2, 0, 1, 0, 47, 63 },
  { 7941, 2, 0, 1, 0, 47, 63 },
  { 7947, 2, 0, 1, 0, 47, 63 },
  { 7951, 2, 0, 1, 0, 47, 63 },
  { 7955, 2, 0, 1, 0, 47, 63 },
  { 7959, 2, 0, 1, 0, 47, 63 },
  { 7999, 2, 0, 1, 0, 47, 63 },
  { 8003, 2, 0, 1, 0, 47, 63 },
  { 8007, 2, 0, 1, 0, 47, 63 },
  { 8011, 2, 0, 1, 0, 47, 63 },
  // 2595: get_image_height, 
  { 7081, 2, 0, 1, 0, 64, 63 },
  { 7083, 2, 0, 1, 0, 64, 63 },
  { 7095, 2, 0, 1, 0, 64, 63 },
  { 7097, 2, 0, 1, 0, 64, 63 },
  { 7099, 2, 0, 1, 0, 64, 63 },
  { 7115, 2, 0, 1, 0, 64, 63 },
  { 7117, 2, 0, 1, 0, 64, 63 },
  { 7121, 2, 0, 1, 0, 64, 63 },
  { 7123, 2, 0, 1, 0, 64, 63 },
  { 7125, 2, 0, 1, 0, 64, 63 },
  { 7147, 2, 0, 1, 0, 69, 63 },
  { 7149, 2, 0, 1, 0, 69, 63 },
  { 7153, 2, 0, 1, 0, 69, 63 },
  { 7155, 2, 0, 1, 0, 69, 63 },
  { 7157, 2, 0, 1, 0, 69, 63 },
  { 7927, 2, 0, 1, 0, 47, 63 },
  { 7933, 2, 0, 1, 0, 47, 63 },
  { 7937, 2, 0, 1, 0, 47, 63 },
  { 7941, 2, 0, 1, 0, 47, 63 },
  { 7947, 2, 0, 1, 0, 47, 63 },
  { 7951, 2, 0, 1, 0, 47, 63 },
  { 7955, 2, 0, 1, 0, 47, 63 },
  { 7959, 2, 0, 1, 0, 47, 63 },
  { 7999, 2, 0, 1, 0, 47, 63 },
  { 8003, 2, 0, 1, 0, 47, 63 },
  { 8007, 2, 0, 1, 0, 47, 63 },
  { 8011, 2, 0, 1, 0, 47, 63 },
  // 2622: get_image_depth, 
  { 7083, 2, 0, 1, 0, 64, 63 },
  { 7117, 2, 0, 1, 0, 64, 63 },
  { 7149, 2, 0, 1, 0, 69, 63 },
  // 2625: get_image_dim, 
  { 7085, 2, 0, 1, 0, 64, 63 },
  { 7087, 2, 0, 1, 0, 64, 63 },
  { 7101, 2, 0, 1, 0, 64, 63 },
  { 7103, 2, 0, 1, 0, 64, 63 },
  { 7105, 2, 0, 1, 0, 64, 63 },
  { 7127, 2, 0, 1, 0, 64, 63 },
  { 7129, 2, 0, 1, 0, 64, 63 },
  { 7131, 2, 0, 1, 0, 64, 63 },
  { 7133, 2, 0, 1, 0, 64, 63 },
  { 7135, 2, 0, 1, 0, 64, 63 },
  { 7159, 2, 0, 1, 0, 69, 63 },
  { 7161, 2, 0, 1, 0, 69, 63 },
  { 7163, 2, 0, 1, 0, 69, 63 },
  { 7165, 2, 0, 1, 0, 69, 63 },
  { 7167, 2, 0, 1, 0, 69, 63 },
  { 7929, 2, 0, 1, 0, 47, 63 },
  { 7935, 2, 0, 1, 0, 47, 63 },
  { 7939, 2, 0, 1, 0, 47, 63 },
  { 7943, 2, 0, 1, 0, 47, 63 },
  { 7949, 2, 0, 1, 0, 47, 63 },
  { 7953, 2, 0, 1, 0, 47, 63 },
  { 7957, 2, 0, 1, 0, 47, 63 },
  { 7961, 2, 0, 1, 0, 47, 63 },
  { 8001, 2, 0, 1, 0, 47, 63 },
  { 8005, 2, 0, 1, 0, 47, 63 },
  { 8009, 2, 0, 1, 0, 47, 63 },
  { 8013, 2, 0, 1, 0, 47, 63 },
  // 2652: get_image_array_size, 
  { 7089, 2, 0, 1, 0, 64, 63 },
  { 7107, 2, 0, 1, 0, 64, 63 },
  { 7109, 2, 0, 1, 0, 64, 63 },
  { 7137, 2, 0, 1, 0, 64, 63 },
  { 7139, 2, 0, 1, 0, 64, 63 },
  { 7141, 2, 0, 1, 0, 64, 63 },
  { 7169, 2, 0, 1, 0, 69, 63 },
  { 7171, 2, 0, 1, 0, 69, 63 },
  { 7173, 2, 0, 1, 0, 69, 63 },
  { 7931, 2, 0, 1, 0, 47, 63 },
  { 7945, 2, 0, 1, 0, 47, 63 },
  { 7963, 2, 0, 1, 0, 47, 63 },
  { 7965, 2, 0, 1, 0, 47, 63 },
  { 8015, 2, 0, 1, 0, 47, 63 },
  { 8017, 2, 0, 1, 0, 47, 63 },
  // 2667: read_imageh, 
  { 7175, 4, 1, 0, 0, 64, 63 },
  { 7179, 4, 1, 0, 0, 64, 63 },
  { 7183, 4, 1, 0, 0, 64, 63 },
  { 7187, 4, 1, 0, 0, 64, 63 },
  { 7191, 4, 1, 0, 0, 64, 63 },
  { 7195, 4, 1, 0, 0, 64, 63 },
  { 7199, 4, 1, 0, 0, 64, 63 },
  { 7203, 4, 1, 0, 0, 64, 63 },
  { 7207, 4, 1, 0, 0, 64, 63 },
  { 7211, 4, 1, 0, 0, 64, 63 },
  { 7215, 3, 1, 0, 0, 64, 60 },
  { 7218, 3, 1, 0, 0, 64, 60 },
  { 7221, 3, 1, 0, 0, 64, 60 },
  { 7224, 3, 1, 0, 0, 64, 60 },
  { 7227, 3, 1, 0, 0, 64, 60 },
  { 7230, 3, 1, 0, 0, 64, 60 },
  { 7233, 3, 1, 0, 0, 69, 60 },
  { 7236, 3, 1, 0, 0, 69, 60 },
  { 7239, 3, 1, 0, 0, 69, 60 },
  { 7242, 3, 1, 0, 0, 69, 60 },
  { 7245, 3, 1, 0, 0, 69, 60 },
  { 7248, 3, 1, 0, 0, 69, 60 },
  // 2689: write_imageh, 
  { 7251, 4, 0, 0, 0, 64, 63 },
  { 7255, 4, 0, 0, 0, 64, 63 },
  { 7259, 4, 0, 0, 0, 64, 63 },
  { 7263, 4, 0, 0, 0, 64, 63 },
  { 7267, 4, 0, 0, 0, 64, 63 },
  { 7271, 4, 0, 0, 0, 64, 63 },
  { 7275, 4, 0, 0, 0, 69, 63 },
  { 7279, 4, 0, 0, 0, 69, 63 },
  { 7283, 4, 0, 0, 0, 69, 63 },
  { 7287, 4, 0, 0, 0, 69, 63 },
  { 7291, 4, 0, 0, 0, 69, 63 },
  { 7295, 4, 0, 0, 0, 69, 63 },
  // 2701: work_group_all, work_group_any, 
  { 1008, 2, 0, 0, 1, 70, 63 },
  // 2702: work_group_broadcast, 
  { 7299, 3, 0, 0, 1, 70, 63 },
  { 7302, 4, 0, 0, 1, 70, 63 },
  { 7306, 5, 0, 0, 1, 70, 63 },
  // 2705: work_group_reduce_add, work_group_scan_exclusive_add, work_group_scan_inclusive_add, work_group_reduce_min, work_group_scan_exclusive_min, work_group_scan_inclusive_min, work_group_reduce_max, work_group_scan_exclusive_max, work_group_scan_inclusive_max, 
  { 7311, 2, 0, 0, 1, 70, 63 },
  // 2706: is_valid_reserve_id, 
  { 7313, 2, 0, 0, 0, 68, 63 },
  // 2707: enqueue_marker, 
  { 7315, 5, 0, 0, 0, 65, 63 },
  // 2708: retain_event, release_event, 
  { 7320, 2, 0, 0, 0, 65, 63 },
  // 2709: create_user_event, 
  { 7322, 1, 0, 0, 0, 65, 63 },
  // 2710: is_valid_event, 
  { 7323, 2, 0, 0, 0, 65, 63 },
  // 2711: set_user_event_status, 
  { 7325, 3, 0, 0, 0, 65, 63 },
  // 2712: capture_event_profiling_info, 
  { 7328, 4, 0, 0, 0, 65, 63 },
  // 2713: get_default_queue, 
  { 7332, 1, 0, 0, 0, 65, 63 },
  // 2714: ndrange_1D, 
  { 7333, 2, 0, 0, 0, 65, 63 },
  { 7335, 3, 0, 0, 0, 65, 63 },
  { 7338, 4, 0, 0, 0, 65, 63 },
  // 2717: ndrange_2D, ndrange_3D, 
  { 7342, 2, 0, 0, 0, 65, 63 },
  { 7344, 3, 0, 0, 0, 65, 63 },
  { 7347, 4, 0, 0, 0, 65, 63 },
  // 2720: get_image_num_mip_levels, 
  { 7079, 2, 0, 0, 0, 54, 63 },
  { 7081, 2, 0, 0, 0, 54, 63 },
  { 7083, 2, 0, 0, 0, 54, 63 },
  { 7093, 2, 0, 0, 0, 54, 63 },
  { 7095, 2, 0, 0, 0, 54, 63 },
  { 7097, 2, 0, 0, 0, 54, 63 },
  { 7099, 2, 0, 0, 0, 54, 63 },
  { 7111, 2, 0, 0, 0, 54, 63 },
  { 7115, 2, 0, 0, 0, 54, 63 },
  { 7117, 2, 0, 0, 0, 54, 63 },
  { 7119, 2, 0, 0, 0, 54, 63 },
  { 7121, 2, 0, 0, 0, 54, 63 },
  { 7123, 2, 0, 0, 0, 54, 63 },
  { 7125, 2, 0, 0, 0, 54, 63 },
  { 7143, 2, 0, 0, 0, 54, 63 },
  { 7147, 2, 0, 0, 0, 54, 63 },
  { 7149, 2, 0, 0, 0, 54, 63 },
  { 7151, 2, 0, 0, 0, 54, 63 },
  { 7153, 2, 0, 0, 0, 54, 63 },
  { 7155, 2, 0, 0, 0, 54, 63 },
  { 7157, 2, 0, 0, 0, 54, 63 },
  // 2741: get_image_num_samples, 
  { 7927, 2, 0, 1, 0, 47, 63 },
  { 7933, 2, 0, 1, 0, 47, 63 },
  { 7937, 2, 0, 1, 0, 47, 63 },
  { 7941, 2, 0, 1, 0, 47, 63 },
  { 7947, 2, 0, 1, 0, 47, 63 },
  { 7951, 2, 0, 1, 0, 47, 63 },
  { 7955, 2, 0, 1, 0, 47, 63 },
  { 7959, 2, 0, 1, 0, 47, 63 },
  { 7999, 2, 0, 1, 0, 47, 63 },
  { 8003, 2, 0, 1, 0, 47, 63 },
  { 8007, 2, 0, 1, 0, 47, 63 },
  { 8011, 2, 0, 1, 0, 47, 63 },
  // 2753: get_sub_group_size, get_max_sub_group_size, get_num_sub_groups, get_sub_group_id, get_sub_group_local_id, 
  { 1452, 1, 0, 0, 0, 63, 63 },
  // 2754: get_enqueued_num_sub_groups, 
  { 1452, 1, 0, 0, 0, 63, 56 },
  // 2755: sub_group_barrier, 
  { 3794, 2, 0, 0, 1, 63, 63 },
  { 3796, 3, 0, 0, 1, 63, 56 },
  // 2757: sub_group_all, sub_group_any, 
  { 1008, 2, 0, 0, 1, 63, 63 },
  // 2758: sub_group_broadcast, 
  { 8019, 3, 0, 0, 1, 63, 63 },
  { 8022, 3, 0, 0, 1, 58, 63 },
  { 8025, 3, 0, 0, 1, 58, 63 },
  // 2761: sub_group_reduce_add, sub_group_reduce_min, sub_group_reduce_max, sub_group_scan_exclusive_add, sub_group_scan_exclusive_min, sub_group_scan_exclusive_max, sub_group_scan_inclusive_add, sub_group_scan_inclusive_min, sub_group_scan_inclusive_max, 
  { 7311, 2, 0, 0, 1, 63, 63 },
  { 8028, 2, 0, 0, 1, 58, 63 },
  // 2763: sub_group_elect, 
  { 8030, 1, 0, 0, 0, 60, 63 },
  // 2764: sub_group_non_uniform_all, sub_group_non_uniform_any, 
  { 1008, 2, 0, 0, 0, 60, 63 },
  // 2765: sub_group_non_uniform_all_equal, 
  { 8031, 2, 0, 0, 0, 60, 63 },
  // 2766: sub_group_non_uniform_broadcast, 
  { 8033, 3, 0, 0, 0, 56, 63 },
  // 2767: sub_group_broadcast_first, 
  { 8036, 2, 0, 0, 0, 56, 63 },
  // 2768: sub_group_ballot, 
  { 8038, 2, 0, 0, 0, 56, 63 },
  // 2769: sub_group_inverse_ballot, 
  { 8040, 2, 0, 1, 0, 56, 63 },
  // 2770: sub_group_ballot_bit_extract, 
  { 8042, 3, 0, 1, 0, 56, 63 },
  // 2771: sub_group_ballot_bit_count, 
  { 8045, 2, 0, 1, 0, 56, 63 },
  // 2772: sub_group_ballot_inclusive_scan, sub_group_ballot_exclusive_scan, sub_group_ballot_find_lsb, sub_group_ballot_find_msb, 
  { 8045, 2, 0, 0, 0, 56, 63 },
  // 2773: get_sub_group_eq_mask, get_sub_group_ge_mask, get_sub_group_gt_mask, get_sub_group_le_mask, get_sub_group_lt_mask, 
  { 8047, 1, 0, 1, 0, 56, 63 },
  // 2774: sub_group_non_uniform_reduce_add, sub_group_non_uniform_reduce_min, sub_group_non_uniform_reduce_max, sub_group_non_uniform_reduce_mul, sub_group_non_uniform_scan_exclusive_add, sub_group_non_uniform_scan_exclusive_min, sub_group_non_uniform_scan_exclusive_max, sub_group_non_uniform_scan_exclusive_mul, sub_group_non_uniform_scan_inclusive_add, sub_group_non_uniform_scan_inclusive_min, sub_group_non_uniform_scan_inclusive_max, sub_group_non_uniform_scan_inclusive_mul, 
  { 8036, 2, 0, 0, 0, 59, 63 },
  // 2775: sub_group_non_uniform_reduce_and, sub_group_non_uniform_reduce_or, sub_group_non_uniform_reduce_xor, sub_group_non_uniform_scan_exclusive_and, sub_group_non_uniform_scan_exclusive_or, sub_group_non_uniform_scan_exclusive_xor, sub_group_non_uniform_scan_inclusive_and, sub_group_non_uniform_scan_inclusive_or, sub_group_non_uniform_scan_inclusive_xor, 
  { 8048, 2, 0, 0, 0, 59, 63 },
  // 2776: sub_group_non_uniform_reduce_logical_and, sub_group_non_uniform_reduce_logical_or, sub_group_non_uniform_reduce_logical_xor, sub_group_non_uniform_scan_exclusive_logical_and, sub_group_non_uniform_scan_exclusive_logical_or, sub_group_non_uniform_scan_exclusive_logical_xor, sub_group_non_uniform_scan_inclusive_logical_and, sub_group_non_uniform_scan_inclusive_logical_or, sub_group_non_uniform_scan_inclusive_logical_xor, 
  { 1008, 2, 0, 0, 0, 59, 63 },
  // 2777: sub_group_shuffle, sub_group_shuffle_xor, 
  { 8050, 3, 0, 0, 0, 61, 63 },
  // 2778: sub_group_shuffle_up, sub_group_shuffle_down, 
  { 8050, 3, 0, 0, 0, 62, 63 },
  // 2779: sub_group_clustered_reduce_add, sub_group_clustered_reduce_min, sub_group_clustered_reduce_max, sub_group_clustered_reduce_mul, 
  { 8050, 3, 0, 0, 0, 57, 63 },
  // 2780: sub_group_clustered_reduce_and, sub_group_clustered_reduce_or, sub_group_clustered_reduce_xor, 
  { 8053, 3, 0, 0, 0, 57, 63 },
  // 2781: sub_group_clustered_reduce_logical_and, sub_group_clustered_reduce_logical_or, sub_group_clustered_reduce_logical_xor, 
  { 8056, 3, 0, 0, 0, 57, 63 },
  // 2782: bitfield_insert, 
  { 8059, 5, 0, 1, 0, 46, 63 },
  // 2783: bitfield_extract_signed, 
  { 8064, 4, 0, 1, 0, 46, 63 },
  { 8068, 4, 0, 1, 0, 46, 63 },
  // 2785: bitfield_extract_unsigned, 
  { 8072, 4, 0, 1, 0, 46, 63 },
  { 8076, 4, 0, 1, 0, 46, 63 },
  // 2787: bit_reverse, 
  { 1607, 2, 0, 1, 0, 46, 63 },
  // 2788: dot_acc_sat, 
  { 8092, 4, 0, 1, 0, 118, 63 },
  { 8096, 4, 0, 1, 0, 118, 63 },
  { 8100, 4, 0, 1, 0, 118, 63 },
  { 8104, 4, 0, 1, 0, 118, 63 },
  // 2792: dot_4x8packed_uu_uint, 
  { 8108, 3, 0, 1, 0, 119, 63 },
  // 2793: dot_4x8packed_ss_int, dot_4x8packed_us_int, dot_4x8packed_su_int, 
  { 8111, 3, 0, 1, 0, 119, 63 },
  // 2794: dot_acc_sat_4x8packed_uu_uint, 
  { 8114, 4, 0, 1, 0, 119, 63 },
  // 2795: dot_acc_sat_4x8packed_ss_int, dot_acc_sat_4x8packed_us_int, dot_acc_sat_4x8packed_su_int, 
  { 8118, 4, 0, 1, 0, 119, 63 },
  // 2796: sub_group_rotate, 
  { 8122, 3, 0, 0, 1, 120, 63 },
  // 2797: sub_group_clustered_rotate, 
  { 8125, 4, 0, 0, 1, 120, 63 },
  // 2798: clock_read_device, 
  { 8129, 1, 0, 0, 0, 121, 63 },
  // 2799: clock_read_hilo_device, 
  { 8130, 1, 0, 0, 0, 121, 63 },
  // 2800: clock_read_work_group, 
  { 8129, 1, 0, 0, 0, 122, 63 },
  // 2801: clock_read_hilo_work_group, 
  { 8130, 1, 0, 0, 0, 122, 63 },
  // 2802: clock_read_sub_group, 
  { 8129, 1, 0, 0, 0, 123, 63 },
  // 2803: clock_read_hilo_sub_group, 
  { 8130, 1, 0, 0, 0, 123, 63 },
  // 2804: intel_sub_group_shuffle, intel_sub_group_shuffle_xor, 
  { 8131, 3, 0, 0, 1, 31, 63 },
  { 8134, 3, 0, 0, 1, 31, 63 },
  { 8137, 3, 0, 0, 1, 31, 63 },
  { 8140, 3, 0, 0, 1, 31, 63 },
  { 8143, 3, 0, 0, 1, 31, 63 },
  { 8146, 3, 0, 0, 1, 31, 63 },
  { 8149, 3, 0, 0, 1, 31, 63 },
  { 8248, 3, 0, 0, 1, 42, 63 },
  { 8251, 3, 0, 0, 1, 42, 63 },
  { 8363, 3, 0, 0, 1, 32, 63 },
  { 8366, 3, 0, 0, 1, 32, 63 },
  // 2815: intel_sub_group_shuffle_down, intel_sub_group_shuffle_up, 
  { 8152, 4, 0, 0, 1, 31, 63 },
  { 8156, 4, 0, 0, 1, 31, 63 },
  { 8160, 4, 0, 0, 1, 31, 63 },
  { 8164, 4, 0, 0, 1, 31, 63 },
  { 8168, 4, 0, 0, 1, 31, 63 },
  { 8172, 4, 0, 0, 1, 31, 63 },
  { 8176, 4, 0, 0, 1, 31, 63 },
  { 8254, 4, 0, 0, 1, 42, 63 },
  { 8258, 4, 0, 0, 1, 42, 63 },
  { 8369, 4, 0, 0, 1, 32, 63 },
  { 8373, 4, 0, 0, 1, 32, 63 },
  // 2826: intel_sub_group_block_read, intel_sub_group_block_read_ui, 
  { 8180, 3, 0, 0, 1, 31, 63 },
  { 8189, 2, 0, 0, 1, 31, 63 },
  { 8221, 3, 0, 0, 1, 41, 63 },
  { 8570, 2, 0, 0, 1, 30, 63 },
  // 2830: intel_sub_group_block_read2, intel_sub_group_block_read_ui2, 
  { 8183, 3, 0, 0, 1, 31, 63 },
  { 8191, 2, 0, 0, 1, 31, 63 },
  { 8224, 3, 0, 0, 1, 41, 63 },
  { 8572, 2, 0, 0, 1, 30, 63 },
  // 2834: intel_sub_group_block_read4, intel_sub_group_block_read_ui4, 
  { 6805, 3, 0, 0, 1, 31, 63 },
  { 8193, 2, 0, 0, 1, 31, 63 },
  { 6865, 3, 0, 0, 1, 41, 63 },
  { 8574, 2, 0, 0, 1, 30, 63 },
  // 2838: intel_sub_group_block_read8, intel_sub_group_block_read_ui8, 
  { 8186, 3, 0, 0, 1, 31, 63 },
  { 8195, 2, 0, 0, 1, 31, 63 },
  { 8227, 3, 0, 0, 1, 41, 63 },
  { 8576, 2, 0, 0, 1, 30, 63 },
  // 2842: intel_sub_group_block_write, intel_sub_group_block_write_ui, 
  { 8197, 4, 0, 0, 1, 31, 63 },
  { 8209, 3, 0, 0, 1, 31, 63 },
  { 8230, 4, 0, 0, 1, 41, 63 },
  { 8578, 3, 0, 0, 1, 30, 63 },
  // 2846: intel_sub_group_block_write2, intel_sub_group_block_write_ui2, 
  { 8201, 4, 0, 0, 1, 31, 63 },
  { 8212, 3, 0, 0, 1, 31, 63 },
  { 8234, 4, 0, 0, 1, 41, 63 },
  { 8581, 3, 0, 0, 1, 30, 63 },
  // 2850: intel_sub_group_block_write4, intel_sub_group_block_write_ui4, 
  { 6927, 4, 0, 0, 1, 31, 63 },
  { 8215, 3, 0, 0, 1, 31, 63 },
  { 7007, 4, 0, 0, 1, 41, 63 },
  { 8584, 3, 0, 0, 1, 30, 63 },
  // 2854: intel_sub_group_block_write8, intel_sub_group_block_write_ui8, 
  { 8205, 4, 0, 0, 1, 31, 63 },
  { 8218, 3, 0, 0, 1, 31, 63 },
  { 8238, 4, 0, 0, 1, 41, 63 },
  { 8587, 3, 0, 0, 1, 30, 63 },
  // 2858: intel_sub_group_broadcast, 
  { 8242, 3, 0, 0, 1, 42, 63 },
  { 8245, 3, 0, 0, 1, 42, 63 },
  { 8357, 3, 0, 0, 1, 32, 63 },
  { 8360, 3, 0, 0, 1, 32, 63 },
  // 2862: intel_sub_group_reduce_add, intel_sub_group_reduce_min, intel_sub_group_reduce_max, intel_sub_group_scan_exclusive_add, intel_sub_group_scan_exclusive_min, intel_sub_group_scan_exclusive_max, intel_sub_group_scan_inclusive_add, intel_sub_group_scan_inclusive_min, intel_sub_group_scan_inclusive_max, 
  { 720, 2, 0, 0, 1, 42, 63 },
  { 864, 2, 0, 0, 1, 42, 63 },
  { 432, 2, 0, 0, 1, 32, 63 },
  { 576, 2, 0, 0, 1, 32, 63 },
  // 2866: intel_sub_group_block_read_us, 
  { 8262, 3, 0, 0, 1, 42, 63 },
  { 8277, 2, 0, 0, 1, 42, 63 },
  { 8322, 3, 0, 0, 1, 45, 63 },
  { 8615, 2, 0, 0, 1, 43, 63 },
  // 2870: intel_sub_group_block_read_us2, 
  { 8265, 3, 0, 0, 1, 42, 63 },
  { 8279, 2, 0, 0, 1, 42, 63 },
  { 8325, 3, 0, 0, 1, 45, 63 },
  { 8617, 2, 0, 0, 1, 43, 63 },
  // 2874: intel_sub_group_block_read_us4, 
  { 8268, 3, 0, 0, 1, 42, 63 },
  { 8281, 2, 0, 0, 1, 42, 63 },
  { 8328, 3, 0, 0, 1, 45, 63 },
  { 8619, 2, 0, 0, 1, 43, 63 },
  // 2878: intel_sub_group_block_read_us8, 
  { 8271, 3, 0, 0, 1, 42, 63 },
  { 8283, 2, 0, 0, 1, 42, 63 },
  { 8331, 3, 0, 0, 1, 45, 63 },
  { 8621, 2, 0, 0, 1, 43, 63 },
  // 2882: intel_sub_group_block_read_us16, 
  { 8274, 3, 0, 0, 1, 42, 63 },
  { 8285, 2, 0, 0, 1, 42, 63 },
  { 8334, 3, 0, 0, 1, 45, 63 },
  { 8623, 2, 0, 0, 1, 43, 63 },
  // 2886: intel_sub_group_block_write_us, 
  { 8287, 4, 0, 0, 1, 42, 63 },
  { 8307, 3, 0, 0, 1, 42, 63 },
  { 8337, 4, 0, 0, 1, 45, 63 },
  { 8625, 3, 0, 0, 1, 43, 63 },
  // 2890: intel_sub_group_block_write_us2, 
  { 8291, 4, 0, 0, 1, 42, 63 },
  { 8310, 3, 0, 0, 1, 42, 63 },
  { 8341, 4, 0, 0, 1, 45, 63 },
  { 8628, 3, 0, 0, 1, 43, 63 },
  // 2894: intel_sub_group_block_write_us4, 
  { 8295, 4, 0, 0, 1, 42, 63 },
  { 8313, 3, 0, 0, 1, 42, 63 },
  { 8345, 4, 0, 0, 1, 45, 63 },
  { 8631, 3, 0, 0, 1, 43, 63 },
  // 2898: intel_sub_group_block_write_us8, 
  { 8299, 4, 0, 0, 1, 42, 63 },
  { 8316, 3, 0, 0, 1, 42, 63 },
  { 8349, 4, 0, 0, 1, 45, 63 },
  { 8634, 3, 0, 0, 1, 43, 63 },
  // 2902: intel_sub_group_block_write_us16, 
  { 8303, 4, 0, 0, 1, 42, 63 },
  { 8319, 3, 0, 0, 1, 42, 63 },
  { 8353, 4, 0, 0, 1, 45, 63 },
  { 8637, 3, 0, 0, 1, 43, 63 },
  // 2906: intel_sub_group_block_read_uc, 
  { 8377, 3, 0, 0, 1, 32, 63 },
  { 8392, 2, 0, 0, 1, 32, 63 },
  { 8437, 3, 0, 0, 1, 35, 63 },
  { 8590, 2, 0, 0, 1, 33, 63 },
  // 2910: intel_sub_group_block_read_uc2, 
  { 8380, 3, 0, 0, 1, 32, 63 },
  { 8394, 2, 0, 0, 1, 32, 63 },
  { 8440, 3, 0, 0, 1, 35, 63 },
  { 8592, 2, 0, 0, 1, 33, 63 },
  // 2914: intel_sub_group_block_read_uc4, 
  { 8383, 3, 0, 0, 1, 32, 63 },
  { 8396, 2, 0, 0, 1, 32, 63 },
  { 8443, 3, 0, 0, 1, 35, 63 },
  { 8594, 2, 0, 0, 1, 33, 63 },
  // 2918: intel_sub_group_block_read_uc8, 
  { 8386, 3, 0, 0, 1, 32, 63 },
  { 8398, 2, 0, 0, 1, 32, 63 },
  { 8446, 3, 0, 0, 1, 35, 63 },
  { 8596, 2, 0, 0, 1, 33, 63 },
  // 2922: intel_sub_group_block_read_uc16, 
  { 8389, 3, 0, 0, 1, 32, 63 },
  { 8400, 2, 0, 0, 1, 32, 63 },
  { 8449, 3, 0, 0, 1, 35, 63 },
  { 8598, 2, 0, 0, 1, 33, 63 },
  // 2926: intel_sub_group_block_write_uc, 
  { 8402, 4, 0, 0, 1, 32, 63 },
  { 8422, 3, 0, 0, 1, 32, 63 },
  { 8452, 4, 0, 0, 1, 35, 63 },
  { 8600, 3, 0, 0, 1, 33, 63 },
  // 2930: intel_sub_group_block_write_uc2, 
  { 8406, 4, 0, 0, 1, 32, 63 },
  { 8425, 3, 0, 0, 1, 32, 63 },
  { 8456, 4, 0, 0, 1, 35, 63 },
  { 8603, 3, 0, 0, 1, 33, 63 },
  // 2934: intel_sub_group_block_write_uc4, 
  { 8410, 4, 0, 0, 1, 32, 63 },
  { 8428, 3, 0, 0, 1, 32, 63 },
  { 8460, 4, 0, 0, 1, 35, 63 },
  { 8606, 3, 0, 0, 1, 33, 63 },
  // 2938: intel_sub_group_block_write_uc8, 
  { 8414, 4, 0, 0, 1, 32, 63 },
  { 8431, 3, 0, 0, 1, 32, 63 },
  { 8464, 4, 0, 0, 1, 35, 63 },
  { 8609, 3, 0, 0, 1, 33, 63 },
  // 2942: intel_sub_group_block_write_uc16, 
  { 8418, 4, 0, 0, 1, 32, 63 },
  { 8434, 3, 0, 0, 1, 32, 63 },
  { 8468, 4, 0, 0, 1, 35, 63 },
  { 8612, 3, 0, 0, 1, 33, 63 },
  // 2946: intel_sub_group_block_read_ul, 
  { 8472, 3, 0, 0, 1, 36, 63 },
  { 8487, 2, 0, 0, 1, 36, 63 },
  { 8527, 3, 0, 0, 1, 39, 63 },
  { 8640, 2, 0, 0, 1, 37, 63 },
  // 2950: intel_sub_group_block_read_ul2, 
  { 8475, 3, 0, 0, 1, 36, 63 },
  { 8489, 2, 0, 0, 1, 36, 63 },
  { 8530, 3, 0, 0, 1, 39, 63 },
  { 8642, 2, 0, 0, 1, 37, 63 },
  // 2954: intel_sub_group_block_read_ul4, 
  { 8478, 3, 0, 0, 1, 36, 63 },
  { 8491, 2, 0, 0, 1, 36, 63 },
  { 8533, 3, 0, 0, 1, 39, 63 },
  { 8644, 2, 0, 0, 1, 37, 63 },
  // 2958: intel_sub_group_block_read_ul8, 
  { 8481, 3, 0, 0, 1, 36, 63 },
  { 8493, 2, 0, 0, 1, 36, 63 },
  { 8536, 3, 0, 0, 1, 39, 63 },
  { 8646, 2, 0, 0, 1, 37, 63 },
  // 2962: intel_sub_group_block_read_ul16, 
  { 8484, 3, 0, 0, 1, 36, 63 },
  { 8539, 3, 0, 0, 1, 39, 63 },
  // 2964: intel_sub_group_block_write_ul, 
  { 8495, 4, 0, 0, 1, 36, 63 },
  { 8515, 3, 0, 0, 1, 36, 63 },
  { 8542, 4, 0, 0, 1, 39, 63 },
  { 8648, 3, 0, 0, 1, 37, 63 },
  // 2968: intel_sub_group_block_write_ul2, 
  { 8499, 4, 0, 0, 1, 36, 63 },
  { 8518, 3, 0, 0, 1, 36, 63 },
  { 8546, 4, 0, 0, 1, 39, 63 },
  { 8651, 3, 0, 0, 1, 37, 63 },
  // 2972: intel_sub_group_block_write_ul4, 
  { 8503, 4, 0, 0, 1, 36, 63 },
  { 8521, 3, 0, 0, 1, 36, 63 },
  { 8550, 4, 0, 0, 1, 39, 63 },
  { 8654, 3, 0, 0, 1, 37, 63 },
  // 2976: intel_sub_group_block_write_ul8, 
  { 8507, 4, 0, 0, 1, 36, 63 },
  { 8524, 3, 0, 0, 1, 36, 63 },
  { 8554, 4, 0, 0, 1, 39, 63 },
  { 8657, 3, 0, 0, 1, 37, 63 },
  // 2980: intel_sub_group_block_write_ul16, 
  { 8511, 4, 0, 0, 1, 36, 63 },
  { 8558, 4, 0, 0, 1, 39, 63 },
  // 2982: intel_sub_group_block_prefetch_ui, intel_sub_group_block_prefetch_ui2, intel_sub_group_block_prefetch_ui4, intel_sub_group_block_prefetch_ui8, 
  { 8562, 2, 0, 0, 1, 40, 63 },
  // 2983: intel_sub_group_block_prefetch_us, intel_sub_group_block_prefetch_us2, intel_sub_group_block_prefetch_us4, intel_sub_group_block_prefetch_us8, intel_sub_group_block_prefetch_us16, 
  { 8564, 2, 0, 0, 1, 44, 63 },
  // 2984: intel_sub_group_block_prefetch_uc, intel_sub_group_block_prefetch_uc2, intel_sub_group_block_prefetch_uc4, intel_sub_group_block_prefetch_uc8, intel_sub_group_block_prefetch_uc16, 
  { 8566, 2, 0, 0, 1, 34, 63 },
  // 2985: intel_sub_group_block_prefetch_ul, intel_sub_group_block_prefetch_ul2, intel_sub_group_block_prefetch_ul4, intel_sub_group_block_prefetch_ul8, 
  { 8568, 2, 0, 0, 1, 38, 63 },
  // 2986: intel_convert_bfloat16_as_ushort, 
  { 792, 2, 0, 1, 0, 27, 63 },
  // 2987: intel_convert_bfloat162_as_ushort2, 
  { 794, 2, 0, 1, 0, 27, 63 },
  // 2988: intel_convert_bfloat163_as_ushort3, 
  { 796, 2, 0, 1, 0, 27, 63 },
  // 2989: intel_convert_bfloat164_as_ushort4, 
  { 798, 2, 0, 1, 0, 27, 63 },
  // 2990: intel_convert_bfloat168_as_ushort8, 
  { 800, 2, 0, 1, 0, 27, 63 },
  // 2991: intel_convert_bfloat1616_as_ushort16, 
  { 802, 2, 0, 1, 0, 27, 63 },
  // 2992: intel_convert_as_bfloat16_float, 
  { 72, 2, 0, 1, 0, 27, 63 },
  // 2993: intel_convert_as_bfloat162_float2, 
  { 74, 2, 0, 1, 0, 27, 63 },
  // 2994: intel_convert_as_bfloat163_float3, 
  { 76, 2, 0, 1, 0, 27, 63 },
  // 2995: intel_convert_as_bfloat164_float4, 
  { 78, 2, 0, 1, 0, 27, 63 },
  // 2996: intel_convert_as_bfloat168_float8, 
  { 80, 2, 0, 1, 0, 27, 63 },
  // 2997: intel_convert_as_bfloat1616_float16, 
  { 82, 2, 0, 1, 0, 27, 63 },
  // 2998: intel_work_group_barrier_arrive, intel_work_group_barrier_wait, 
  { 3794, 2, 0, 0, 1, 28, 63 },
  { 3796, 3, 0, 0, 1, 28, 56 },
  // 3000: arm_dot, 
  { 8080, 3, 0, 0, 0, 5, 63 },
  { 8083, 3, 0, 0, 0, 5, 63 },
  // 3002: arm_dot_acc, 
  { 8092, 4, 0, 0, 0, 2, 63 },
  { 8096, 4, 0, 0, 0, 2, 63 },
  { 8660, 4, 0, 0, 0, 3, 63 },
  { 8664, 4, 0, 0, 0, 3, 63 },
  // 3006: arm_dot_acc_sat, 
  { 8092, 4, 0, 0, 0, 4, 63 },
  { 8096, 4, 0, 0, 0, 4, 63 },
  // 3008: amd_bitalign, amd_bytealign, amd_lerp, amd_sadhi, amd_sad, 
  { 1648, 4, 0, 0, 0, 0, 63 },
  // 3009: amd_unpack0, amd_unpack1, amd_unpack2, amd_unpack3, 
  { 1458, 2, 0, 0, 0, 0, 63 },
  // 3010: amd_pack, 
  { 8668, 2, 0, 0, 0, 0, 63 },
  // 3011: amd_sad4, 
  { 8670, 4, 0, 0, 0, 0, 63 },
  // 3012: amd_bfe, 
  { 8674, 4, 0, 0, 0, 1, 63 },
  { 1648, 4, 0, 0, 0, 1, 63 },
  // 3014: amd_msad, amd_sadd, amd_sadw, 
  { 1648, 4, 0, 0, 0, 1, 63 },
  // 3015: amd_bfm, 
  { 1655, 3, 0, 0, 0, 1, 63 },
  // 3016: amd_max3, amd_median3, amd_min3, 
  { 8678, 4, 0, 0, 0, 1, 63 },
  { 1644, 4, 0, 0, 0, 1, 63 },
  { 1648, 4, 0, 0, 0, 1, 63 },
  // 3019: amd_mqsad, amd_qsad, 
  { 8682, 4, 0, 0, 0, 1, 63 },
};


// Find out whether a string matches an existing OpenCL builtin function name.
// Returns: A pair <0, 0> if no name matches.
//          A pair <Index, Len> indexing the BuiltinTable if the name is
//          matching an OpenCL builtin function.
static std::pair<unsigned, unsigned> isOpenCLBuiltin(llvm::StringRef Name) {

  switch (Name.size()) {
  default: break;
  case 3:	 // 19 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'b':	 // 1 string to match.
        if (Name[2] != 's')
          break;
        return std::make_pair(783, 1);	 // "abs"
      case 'l':	 // 1 string to match.
        if (Name[2] != 'l')
          break;
        return std::make_pair(854, 1);	 // "all"
      case 'n':	 // 1 string to match.
        if (Name[2] != 'y')
          break;
        return std::make_pair(854, 1);	 // "any"
      }
      break;
    case 'c':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (Name[2] != 'z')
          break;
        return std::make_pair(784, 1);	 // "clz"
      case 'o':	 // 1 string to match.
        if (Name[2] != 's')
          break;
        return std::make_pair(731, 1);	 // "cos"
      case 't':	 // 1 string to match.
        if (Name[2] != 'z')
          break;
        return std::make_pair(786, 1);	 // "ctz"
      }
      break;
    case 'd':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ot", 2) != 0)
        break;
      return std::make_pair(838, 7);	 // "dot"
    case 'e':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 'r':	 // 1 string to match.
        if (Name[2] != 'f')
          break;
        return std::make_pair(731, 1);	 // "erf"
      case 'x':	 // 1 string to match.
        if (Name[2] != 'p')
          break;
        return std::make_pair(731, 1);	 // "exp"
      }
      break;
    case 'f':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ma", 2) != 0)
        break;
      return std::make_pair(752, 1);	 // "fma"
    case 'l':	 // 1 string to match.
      if (memcmp(Name.data()+1, "og", 2) != 0)
        break;
      return std::make_pair(731, 1);	 // "log"
    case 'm':	 // 4 strings to match.
      switch (Name[1]) {
      default: break;
      case 'a':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'd':	 // 1 string to match.
          return std::make_pair(752, 1);	 // "mad"
        case 'x':	 // 1 string to match.
          return std::make_pair(789, 6);	 // "max"
        }
        break;
      case 'i':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'n':	 // 1 string to match.
          return std::make_pair(789, 6);	 // "min"
        case 'x':	 // 1 string to match.
          return std::make_pair(816, 4);	 // "mix"
        }
        break;
      }
      break;
    case 'n':	 // 1 string to match.
      if (memcmp(Name.data()+1, "an", 2) != 0)
        break;
      return std::make_pair(732, 3);	 // "nan"
    case 'p':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ow", 2) != 0)
        break;
      return std::make_pair(735, 1);	 // "pow"
    case 's':	 // 1 string to match.
      if (memcmp(Name.data()+1, "in", 2) != 0)
        break;
      return std::make_pair(731, 1);	 // "sin"
    case 't':	 // 1 string to match.
      if (memcmp(Name.data()+1, "an", 2) != 0)
        break;
      return std::make_pair(731, 1);	 // "tan"
    }
    break;
  case 4:	 // 25 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'c':	 // 1 string to match.
        if (memcmp(Name.data()+2, "os", 2) != 0)
          break;
        return std::make_pair(731, 1);	 // "acos"
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+2, "in", 2) != 0)
          break;
        return std::make_pair(731, 1);	 // "asin"
      case 't':	 // 1 string to match.
        if (memcmp(Name.data()+2, "an", 2) != 0)
          break;
        return std::make_pair(731, 1);	 // "atan"
      }
      break;
    case 'c':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'b':	 // 1 string to match.
        if (memcmp(Name.data()+2, "rt", 2) != 0)
          break;
        return std::make_pair(731, 1);	 // "cbrt"
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+2, "il", 2) != 0)
          break;
        return std::make_pair(731, 1);	 // "ceil"
      case 'o':	 // 1 string to match.
        if (memcmp(Name.data()+2, "sh", 2) != 0)
          break;
        return std::make_pair(731, 1);	 // "cosh"
      }
      break;
    case 'e':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 'r':	 // 1 string to match.
        if (memcmp(Name.data()+2, "fc", 2) != 0)
          break;
        return std::make_pair(731, 1);	 // "erfc"
      case 'x':	 // 1 string to match.
        if (memcmp(Name.data()+2, "p2", 2) != 0)
          break;
        return std::make_pair(731, 1);	 // "exp2"
      }
      break;
    case 'f':	 // 5 strings to match.
      switch (Name[1]) {
      default: break;
      case 'a':	 // 1 string to match.
        if (memcmp(Name.data()+2, "bs", 2) != 0)
          break;
        return std::make_pair(731, 1);	 // "fabs"
      case 'd':	 // 1 string to match.
        if (memcmp(Name.data()+2, "im", 2) != 0)
          break;
        return std::make_pair(735, 1);	 // "fdim"
      case 'm':	 // 3 strings to match.
        switch (Name[2]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (Name[3] != 'x')
            break;
          return std::make_pair(736, 4);	 // "fmax"
        case 'i':	 // 1 string to match.
          if (Name[3] != 'n')
            break;
          return std::make_pair(736, 4);	 // "fmin"
        case 'o':	 // 1 string to match.
          if (Name[3] != 'd')
            break;
          return std::make_pair(735, 1);	 // "fmod"
        }
        break;
      }
      break;
    case 'h':	 // 1 string to match.
      if (memcmp(Name.data()+1, "add", 3) != 0)
        break;
      return std::make_pair(788, 1);	 // "hadd"
    case 'l':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "og", 2) != 0)
        break;
      switch (Name[3]) {
      default: break;
      case '2':	 // 1 string to match.
        return std::make_pair(731, 1);	 // "log2"
      case 'b':	 // 1 string to match.
        return std::make_pair(731, 1);	 // "logb"
      }
      break;
    case 'm':	 // 1 string to match.
      if (memcmp(Name.data()+1, "odf", 3) != 0)
        break;
      return std::make_pair(753, 4);	 // "modf"
    case 'p':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "ow", 2) != 0)
        break;
      switch (Name[3]) {
      default: break;
      case 'n':	 // 1 string to match.
        return std::make_pair(749, 3);	 // "pown"
      case 'r':	 // 1 string to match.
        return std::make_pair(735, 1);	 // "powr"
      }
      break;
    case 'r':	 // 1 string to match.
      if (memcmp(Name.data()+1, "int", 3) != 0)
        break;
      return std::make_pair(731, 1);	 // "rint"
    case 's':	 // 4 strings to match.
      switch (Name[1]) {
      default: break;
      case 'i':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'g':	 // 1 string to match.
          if (Name[3] != 'n')
            break;
          return std::make_pair(731, 1);	 // "sign"
        case 'n':	 // 1 string to match.
          if (Name[3] != 'h')
            break;
          return std::make_pair(731, 1);	 // "sinh"
        }
        break;
      case 'q':	 // 1 string to match.
        if (memcmp(Name.data()+2, "rt", 2) != 0)
          break;
        return std::make_pair(731, 1);	 // "sqrt"
      case 't':	 // 1 string to match.
        if (memcmp(Name.data()+2, "ep", 2) != 0)
          break;
        return std::make_pair(812, 4);	 // "step"
      }
      break;
    case 't':	 // 1 string to match.
      if (memcmp(Name.data()+1, "anh", 3) != 0)
        break;
      return std::make_pair(731, 1);	 // "tanh"
    }
    break;
  case 5:	 // 28 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 4 strings to match.
      switch (Name[1]) {
      default: break;
      case 'c':	 // 1 string to match.
        if (memcmp(Name.data()+2, "osh", 3) != 0)
          break;
        return std::make_pair(731, 1);	 // "acosh"
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+2, "inh", 3) != 0)
          break;
        return std::make_pair(731, 1);	 // "asinh"
      case 't':	 // 2 strings to match.
        if (memcmp(Name.data()+2, "an", 2) != 0)
          break;
        switch (Name[4]) {
        default: break;
        case '2':	 // 1 string to match.
          return std::make_pair(735, 1);	 // "atan2"
        case 'h':	 // 1 string to match.
          return std::make_pair(731, 1);	 // "atanh"
        }
        break;
      }
      break;
    case 'c':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+2, "amp", 3) != 0)
          break;
        return std::make_pair(801, 6);	 // "clamp"
      case 'o':	 // 1 string to match.
        if (memcmp(Name.data()+2, "spi", 3) != 0)
          break;
        return std::make_pair(731, 1);	 // "cospi"
      case 'r':	 // 1 string to match.
        if (memcmp(Name.data()+2, "oss", 3) != 0)
          break;
        return std::make_pair(832, 6);	 // "cross"
      }
      break;
    case 'e':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "xp", 2) != 0)
        break;
      switch (Name[3]) {
      default: break;
      case '1':	 // 1 string to match.
        if (Name[4] != '0')
          break;
        return std::make_pair(731, 1);	 // "exp10"
      case 'm':	 // 1 string to match.
        if (Name[4] != '1')
          break;
        return std::make_pair(731, 1);	 // "expm1"
      }
      break;
    case 'f':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+2, "oor", 3) != 0)
          break;
        return std::make_pair(731, 1);	 // "floor"
      case 'r':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (memcmp(Name.data()+3, "ct", 2) != 0)
            break;
          return std::make_pair(753, 4);	 // "fract"
        case 'e':	 // 1 string to match.
          if (memcmp(Name.data()+3, "xp", 2) != 0)
            break;
          return std::make_pair(757, 12);	 // "frexp"
        }
        break;
      }
      break;
    case 'h':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ypot", 4) != 0)
        break;
      return std::make_pair(735, 1);	 // "hypot"
    case 'i':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+2, "ogb", 3) != 0)
          break;
        return std::make_pair(740, 3);	 // "ilogb"
      case 's':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'i':	 // 1 string to match.
          if (memcmp(Name.data()+3, "nf", 2) != 0)
            break;
          return std::make_pair(849, 5);	 // "isinf"
        case 'n':	 // 1 string to match.
          if (memcmp(Name.data()+3, "an", 2) != 0)
            break;
          return std::make_pair(849, 5);	 // "isnan"
        }
        break;
      }
      break;
    case 'l':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'd':	 // 1 string to match.
        if (memcmp(Name.data()+2, "exp", 3) != 0)
          break;
        return std::make_pair(743, 6);	 // "ldexp"
      case 'o':	 // 2 strings to match.
        if (memcmp(Name.data()+2, "g1", 2) != 0)
          break;
        switch (Name[4]) {
        default: break;
        case '0':	 // 1 string to match.
          return std::make_pair(731, 1);	 // "log10"
        case 'p':	 // 1 string to match.
          return std::make_pair(731, 1);	 // "log1p"
        }
        break;
      }
      break;
    case 'm':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 'a':	 // 1 string to match.
        if (memcmp(Name.data()+2, "d24", 3) != 0)
          break;
        return std::make_pair(808, 2);	 // "mad24"
      case 'u':	 // 1 string to match.
        if (memcmp(Name.data()+2, "l24", 3) != 0)
          break;
        return std::make_pair(810, 2);	 // "mul24"
      }
      break;
    case 'r':	 // 4 strings to match.
      switch (Name[1]) {
      default: break;
      case 'h':	 // 1 string to match.
        if (memcmp(Name.data()+2, "add", 3) != 0)
          break;
        return std::make_pair(788, 1);	 // "rhadd"
      case 'o':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'o':	 // 1 string to match.
          if (memcmp(Name.data()+3, "tn", 2) != 0)
            break;
          return std::make_pair(749, 3);	 // "rootn"
        case 'u':	 // 1 string to match.
          if (memcmp(Name.data()+3, "nd", 2) != 0)
            break;
          return std::make_pair(731, 1);	 // "round"
        }
        break;
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+2, "qrt", 3) != 0)
          break;
        return std::make_pair(731, 1);	 // "rsqrt"
      }
      break;
    case 's':	 // 1 string to match.
      if (memcmp(Name.data()+1, "inpi", 4) != 0)
        break;
      return std::make_pair(731, 1);	 // "sinpi"
    case 't':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 'a':	 // 1 string to match.
        if (memcmp(Name.data()+2, "npi", 3) != 0)
          break;
        return std::make_pair(731, 1);	 // "tanpi"
      case 'r':	 // 1 string to match.
        if (memcmp(Name.data()+2, "unc", 3) != 0)
          break;
        return std::make_pair(731, 1);	 // "trunc"
      }
      break;
    }
    break;
  case 6:	 // 19 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'c':	 // 1 string to match.
        if (memcmp(Name.data()+2, "ospi", 4) != 0)
          break;
        return std::make_pair(731, 1);	 // "acospi"
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+2, "inpi", 4) != 0)
          break;
        return std::make_pair(731, 1);	 // "asinpi"
      case 't':	 // 1 string to match.
        if (memcmp(Name.data()+2, "anpi", 4) != 0)
          break;
        return std::make_pair(731, 1);	 // "atanpi"
      }
      break;
    case 'i':	 // 1 string to match.
      if (memcmp(Name.data()+1, "sless", 5) != 0)
        break;
      return std::make_pair(855, 5);	 // "isless"
    case 'l':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+2, "ngth", 4) != 0)
          break;
        return std::make_pair(824, 3);	 // "length"
      case 'g':	 // 1 string to match.
        if (memcmp(Name.data()+2, "amma", 4) != 0)
          break;
        return std::make_pair(731, 1);	 // "lgamma"
      }
      break;
    case 'm':	 // 4 strings to match.
      switch (Name[1]) {
      default: break;
      case 'a':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'd':	 // 1 string to match.
          if (memcmp(Name.data()+3, "_hi", 3) != 0)
            break;
          return std::make_pair(807, 1);	 // "mad_hi"
        case 'x':	 // 1 string to match.
          if (memcmp(Name.data()+3, "mag", 3) != 0)
            break;
          return std::make_pair(735, 1);	 // "maxmag"
        }
        break;
      case 'i':	 // 1 string to match.
        if (memcmp(Name.data()+2, "nmag", 4) != 0)
          break;
        return std::make_pair(735, 1);	 // "minmag"
      case 'u':	 // 1 string to match.
        if (memcmp(Name.data()+2, "l_hi", 4) != 0)
          break;
        return std::make_pair(788, 1);	 // "mul_hi"
      }
      break;
    case 'r':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+2, "mquo", 4) != 0)
          break;
        return std::make_pair(769, 12);	 // "remquo"
      case 'o':	 // 1 string to match.
        if (memcmp(Name.data()+2, "tate", 4) != 0)
          break;
        return std::make_pair(788, 1);	 // "rotate"
      }
      break;
    case 's':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+2, "lect", 4) != 0)
          break;
        return std::make_pair(861, 10);	 // "select"
      case 'i':	 // 1 string to match.
        if (memcmp(Name.data()+2, "ncos", 4) != 0)
          break;
        return std::make_pair(753, 4);	 // "sincos"
      }
      break;
    case 't':	 // 1 string to match.
      if (memcmp(Name.data()+1, "gamma", 5) != 0)
        break;
      return std::make_pair(731, 1);	 // "tgamma"
    case 'v':	 // 4 strings to match.
      if (memcmp(Name.data()+1, "load", 4) != 0)
        break;
      switch (Name[5]) {
      default: break;
      case '2':	 // 1 string to match.
        return std::make_pair(871, 55);	 // "vload2"
      case '3':	 // 1 string to match.
        return std::make_pair(970, 55);	 // "vload3"
      case '4':	 // 1 string to match.
        return std::make_pair(1069, 55);	 // "vload4"
      case '8':	 // 1 string to match.
        return std::make_pair(1168, 55);	 // "vload8"
      }
      break;
    }
    break;
  case 7:	 // 20 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 7 strings to match.
      switch (Name[1]) {
      default: break;
      case 'd':	 // 1 string to match.
        if (memcmp(Name.data()+2, "d_sat", 5) != 0)
          break;
        return std::make_pair(788, 1);	 // "add_sat"
      case 'm':	 // 3 strings to match.
        if (memcmp(Name.data()+2, "d_", 2) != 0)
          break;
        switch (Name[4]) {
        default: break;
        case 'b':	 // 2 strings to match.
          if (Name[5] != 'f')
            break;
          switch (Name[6]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(3012, 2);	 // "amd_bfe"
          case 'm':	 // 1 string to match.
            return std::make_pair(3015, 1);	 // "amd_bfm"
          }
          break;
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+5, "ad", 2) != 0)
            break;
          return std::make_pair(3008, 1);	 // "amd_sad"
        }
        break;
      case 'r':	 // 1 string to match.
        if (memcmp(Name.data()+2, "m_dot", 5) != 0)
          break;
        return std::make_pair(3000, 2);	 // "arm_dot"
      case 't':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (memcmp(Name.data()+3, "n2pi", 4) != 0)
            break;
          return std::make_pair(735, 1);	 // "atan2pi"
        case 'o':	 // 1 string to match.
          if (memcmp(Name.data()+3, "m_or", 4) != 0)
            break;
          return std::make_pair(1481, 8);	 // "atom_or"
        }
        break;
      }
      break;
    case 'b':	 // 1 string to match.
      if (memcmp(Name.data()+1, "arrier", 6) != 0)
        break;
      return std::make_pair(1444, 1);	 // "barrier"
    case 'd':	 // 1 string to match.
      if (memcmp(Name.data()+1, "egrees", 6) != 0)
        break;
      return std::make_pair(731, 1);	 // "degrees"
    case 'i':	 // 1 string to match.
      if (memcmp(Name.data()+1, "sequal", 6) != 0)
        break;
      return std::make_pair(855, 5);	 // "isequal"
    case 'm':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ad_sat", 6) != 0)
        break;
      return std::make_pair(807, 1);	 // "mad_sat"
    case 'r':	 // 1 string to match.
      if (memcmp(Name.data()+1, "adians", 6) != 0)
        break;
      return std::make_pair(731, 1);	 // "radians"
    case 's':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'h':	 // 1 string to match.
        if (memcmp(Name.data()+2, "uffle", 5) != 0)
          break;
        return std::make_pair(1975, 176);	 // "shuffle"
      case 'i':	 // 1 string to match.
        if (memcmp(Name.data()+2, "gnbit", 5) != 0)
          break;
        return std::make_pair(849, 5);	 // "signbit"
      case 'u':	 // 1 string to match.
        if (memcmp(Name.data()+2, "b_sat", 5) != 0)
          break;
        return std::make_pair(788, 1);	 // "sub_sat"
      }
      break;
    case 'v':	 // 5 strings to match.
      switch (Name[1]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+2, "oad16", 5) != 0)
          break;
        return std::make_pair(1267, 55);	 // "vload16"
      case 's':	 // 4 strings to match.
        if (memcmp(Name.data()+2, "tore", 4) != 0)
          break;
        switch (Name[6]) {
        default: break;
        case '2':	 // 1 string to match.
          return std::make_pair(926, 44);	 // "vstore2"
        case '3':	 // 1 string to match.
          return std::make_pair(1025, 44);	 // "vstore3"
        case '4':	 // 1 string to match.
          return std::make_pair(1124, 44);	 // "vstore4"
        case '8':	 // 1 string to match.
          return std::make_pair(1223, 44);	 // "vstore8"
        }
        break;
      }
      break;
    }
    break;
  case 8:	 // 33 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 18 strings to match.
      switch (Name[1]) {
      default: break;
      case 'b':	 // 1 string to match.
        if (memcmp(Name.data()+2, "s_diff", 6) != 0)
          break;
        return std::make_pair(787, 1);	 // "abs_diff"
      case 'm':	 // 9 strings to match.
        if (memcmp(Name.data()+2, "d_", 2) != 0)
          break;
        switch (Name[4]) {
        default: break;
        case 'l':	 // 1 string to match.
          if (memcmp(Name.data()+5, "erp", 3) != 0)
            break;
          return std::make_pair(3008, 1);	 // "amd_lerp"
        case 'm':	 // 3 strings to match.
          switch (Name[5]) {
          default: break;
          case 'a':	 // 1 string to match.
            if (memcmp(Name.data()+6, "x3", 2) != 0)
              break;
            return std::make_pair(3016, 3);	 // "amd_max3"
          case 'i':	 // 1 string to match.
            if (memcmp(Name.data()+6, "n3", 2) != 0)
              break;
            return std::make_pair(3016, 3);	 // "amd_min3"
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+6, "ad", 2) != 0)
              break;
            return std::make_pair(3014, 1);	 // "amd_msad"
          }
          break;
        case 'p':	 // 1 string to match.
          if (memcmp(Name.data()+5, "ack", 3) != 0)
            break;
          return std::make_pair(3010, 1);	 // "amd_pack"
        case 'q':	 // 1 string to match.
          if (memcmp(Name.data()+5, "sad", 3) != 0)
            break;
          return std::make_pair(3019, 1);	 // "amd_qsad"
        case 's':	 // 3 strings to match.
          if (memcmp(Name.data()+5, "ad", 2) != 0)
            break;
          switch (Name[7]) {
          default: break;
          case '4':	 // 1 string to match.
            return std::make_pair(3011, 1);	 // "amd_sad4"
          case 'd':	 // 1 string to match.
            return std::make_pair(3014, 1);	 // "amd_sadd"
          case 'w':	 // 1 string to match.
            return std::make_pair(3014, 1);	 // "amd_sadw"
          }
          break;
        }
        break;
      case 't':	 // 8 strings to match.
        if (memcmp(Name.data()+2, "om_", 3) != 0)
          break;
        switch (Name[5]) {
        default: break;
        case 'a':	 // 2 strings to match.
          switch (Name[6]) {
          default: break;
          case 'd':	 // 1 string to match.
            if (Name[7] != 'd')
              break;
            return std::make_pair(1457, 8);	 // "atom_add"
          case 'n':	 // 1 string to match.
            if (Name[7] != 'd')
              break;
            return std::make_pair(1481, 8);	 // "atom_and"
          }
          break;
        case 'd':	 // 1 string to match.
          if (memcmp(Name.data()+6, "ec", 2) != 0)
            break;
          return std::make_pair(1465, 8);	 // "atom_dec"
        case 'i':	 // 1 string to match.
          if (memcmp(Name.data()+6, "nc", 2) != 0)
            break;
          return std::make_pair(1465, 8);	 // "atom_inc"
        case 'm':	 // 2 strings to match.
          switch (Name[6]) {
          default: break;
          case 'a':	 // 1 string to match.
            if (Name[7] != 'x')
              break;
            return std::make_pair(1481, 8);	 // "atom_max"
          case 'i':	 // 1 string to match.
            if (Name[7] != 'n')
              break;
            return std::make_pair(1481, 8);	 // "atom_min"
          }
          break;
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+6, "ub", 2) != 0)
            break;
          return std::make_pair(1457, 8);	 // "atom_sub"
        case 'x':	 // 1 string to match.
          if (memcmp(Name.data()+6, "or", 2) != 0)
            break;
          return std::make_pair(1481, 8);	 // "atom_xor"
        }
        break;
      }
      break;
    case 'c':	 // 1 string to match.
      if (memcmp(Name.data()+1, "opysign", 7) != 0)
        break;
      return std::make_pair(735, 1);	 // "copysign"
    case 'd':	 // 1 string to match.
      if (memcmp(Name.data()+1, "istance", 7) != 0)
        break;
      return std::make_pair(845, 3);	 // "distance"
    case 'h':	 // 5 strings to match.
      if (memcmp(Name.data()+1, "alf_", 4) != 0)
        break;
      switch (Name[5]) {
      default: break;
      case 'c':	 // 1 string to match.
        if (memcmp(Name.data()+6, "os", 2) != 0)
          break;
        return std::make_pair(781, 1);	 // "half_cos"
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+6, "xp", 2) != 0)
          break;
        return std::make_pair(781, 1);	 // "half_exp"
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+6, "og", 2) != 0)
          break;
        return std::make_pair(781, 1);	 // "half_log"
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+6, "in", 2) != 0)
          break;
        return std::make_pair(781, 1);	 // "half_sin"
      case 't':	 // 1 string to match.
        if (memcmp(Name.data()+6, "an", 2) != 0)
          break;
        return std::make_pair(781, 1);	 // "half_tan"
      }
      break;
    case 'i':	 // 2 strings to match.
      if (Name[1] != 's')
        break;
      switch (Name[2]) {
      default: break;
      case 'f':	 // 1 string to match.
        if (memcmp(Name.data()+3, "inite", 5) != 0)
          break;
        return std::make_pair(849, 5);	 // "isfinite"
      case 'n':	 // 1 string to match.
        if (memcmp(Name.data()+3, "ormal", 5) != 0)
          break;
        return std::make_pair(849, 5);	 // "isnormal"
      }
      break;
    case 'l':	 // 1 string to match.
      if (memcmp(Name.data()+1, "gamma_r", 7) != 0)
        break;
      return std::make_pair(757, 12);	 // "lgamma_r"
    case 'p':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 'o':	 // 1 string to match.
        if (memcmp(Name.data()+2, "pcount", 6) != 0)
          break;
        return std::make_pair(785, 1);	 // "popcount"
      case 'r':	 // 1 string to match.
        if (memcmp(Name.data()+2, "efetch", 6) != 0)
          break;
        return std::make_pair(1454, 1);	 // "prefetch"
      }
      break;
    case 's':	 // 1 string to match.
      if (memcmp(Name.data()+1, "huffle2", 7) != 0)
        break;
      return std::make_pair(2151, 176);	 // "shuffle2"
    case 'u':	 // 1 string to match.
      if (memcmp(Name.data()+1, "psample", 7) != 0)
        break;
      return std::make_pair(795, 6);	 // "upsample"
    case 'v':	 // 1 string to match.
      if (memcmp(Name.data()+1, "store16", 7) != 0)
        break;
      return std::make_pair(1322, 44);	 // "vstore16"
    }
    break;
  case 9:	 // 16 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 4 strings to match.
      switch (Name[1]) {
      default: break;
      case 'm':	 // 2 strings to match.
        if (memcmp(Name.data()+2, "d_", 2) != 0)
          break;
        switch (Name[4]) {
        default: break;
        case 'm':	 // 1 string to match.
          if (memcmp(Name.data()+5, "qsad", 4) != 0)
            break;
          return std::make_pair(3019, 1);	 // "amd_mqsad"
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+5, "adhi", 4) != 0)
            break;
          return std::make_pair(3008, 1);	 // "amd_sadhi"
        }
        break;
      case 't':	 // 2 strings to match.
        if (memcmp(Name.data()+2, "om", 2) != 0)
          break;
        switch (Name[4]) {
        default: break;
        case '_':	 // 1 string to match.
          if (memcmp(Name.data()+5, "xchg", 4) != 0)
            break;
          return std::make_pair(1457, 8);	 // "atom_xchg"
        case 'i':	 // 1 string to match.
          if (memcmp(Name.data()+5, "c_or", 4) != 0)
            break;
          return std::make_pair(1497, 6);	 // "atomic_or"
        }
        break;
      }
      break;
    case 'b':	 // 1 string to match.
      if (memcmp(Name.data()+1, "itselect", 8) != 0)
        break;
      return std::make_pair(860, 1);	 // "bitselect"
    case 'g':	 // 1 string to match.
      if (memcmp(Name.data()+1, "et_fence", 8) != 0)
        break;
      return std::make_pair(1448, 2);	 // "get_fence"
    case 'h':	 // 4 strings to match.
      if (memcmp(Name.data()+1, "alf_", 4) != 0)
        break;
      switch (Name[5]) {
      default: break;
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+6, "xp2", 3) != 0)
          break;
        return std::make_pair(781, 1);	 // "half_exp2"
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+6, "og2", 3) != 0)
          break;
        return std::make_pair(781, 1);	 // "half_log2"
      case 'p':	 // 1 string to match.
        if (memcmp(Name.data()+6, "owr", 3) != 0)
          break;
        return std::make_pair(782, 1);	 // "half_powr"
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+6, "qrt", 3) != 0)
          break;
        return std::make_pair(781, 1);	 // "half_sqrt"
      }
      break;
    case 'i':	 // 2 strings to match.
      if (Name[1] != 's')
        break;
      switch (Name[2]) {
      default: break;
      case 'g':	 // 1 string to match.
        if (memcmp(Name.data()+3, "reater", 6) != 0)
          break;
        return std::make_pair(855, 5);	 // "isgreater"
      case 'o':	 // 1 string to match.
        if (memcmp(Name.data()+3, "rdered", 6) != 0)
          break;
        return std::make_pair(855, 5);	 // "isordered"
      }
      break;
    case 'm':	 // 1 string to match.
      if (memcmp(Name.data()+1, "em_fence", 8) != 0)
        break;
      return std::make_pair(1447, 1);	 // "mem_fence"
    case 'n':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+2, "xtafter", 7) != 0)
          break;
        return std::make_pair(735, 1);	 // "nextafter"
      case 'o':	 // 1 string to match.
        if (memcmp(Name.data()+2, "rmalize", 7) != 0)
          break;
        return std::make_pair(827, 3);	 // "normalize"
      }
      break;
    case 'r':	 // 1 string to match.
      if (memcmp(Name.data()+1, "emainder", 8) != 0)
        break;
      return std::make_pair(735, 1);	 // "remainder"
    }
    break;
  case 10:	 // 23 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 8 strings to match.
      if (memcmp(Name.data()+1, "tomic_", 6) != 0)
        break;
      switch (Name[7]) {
      default: break;
      case 'a':	 // 2 strings to match.
        switch (Name[8]) {
        default: break;
        case 'd':	 // 1 string to match.
          if (Name[9] != 'd')
            break;
          return std::make_pair(1497, 6);	 // "atomic_add"
        case 'n':	 // 1 string to match.
          if (Name[9] != 'd')
            break;
          return std::make_pair(1497, 6);	 // "atomic_and"
        }
        break;
      case 'd':	 // 1 string to match.
        if (memcmp(Name.data()+8, "ec", 2) != 0)
          break;
        return std::make_pair(1503, 6);	 // "atomic_dec"
      case 'i':	 // 1 string to match.
        if (memcmp(Name.data()+8, "nc", 2) != 0)
          break;
        return std::make_pair(1503, 6);	 // "atomic_inc"
      case 'm':	 // 2 strings to match.
        switch (Name[8]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (Name[9] != 'x')
            break;
          return std::make_pair(1497, 6);	 // "atomic_max"
        case 'i':	 // 1 string to match.
          if (Name[9] != 'n')
            break;
          return std::make_pair(1497, 6);	 // "atomic_min"
        }
        break;
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+8, "ub", 2) != 0)
          break;
        return std::make_pair(1497, 6);	 // "atomic_sub"
      case 'x':	 // 1 string to match.
        if (memcmp(Name.data()+8, "or", 2) != 0)
          break;
        return std::make_pair(1497, 6);	 // "atomic_xor"
      }
      break;
    case 'h':	 // 4 strings to match.
      if (memcmp(Name.data()+1, "alf_", 4) != 0)
        break;
      switch (Name[5]) {
      default: break;
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+6, "xp10", 4) != 0)
          break;
        return std::make_pair(781, 1);	 // "half_exp10"
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+6, "og10", 4) != 0)
          break;
        return std::make_pair(781, 1);	 // "half_log10"
      case 'r':	 // 2 strings to match.
        switch (Name[6]) {
        default: break;
        case 'e':	 // 1 string to match.
          if (memcmp(Name.data()+7, "cip", 3) != 0)
            break;
          return std::make_pair(781, 1);	 // "half_recip"
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+7, "qrt", 3) != 0)
            break;
          return std::make_pair(781, 1);	 // "half_rsqrt"
        }
        break;
      }
      break;
    case 'i':	 // 1 string to match.
      if (memcmp(Name.data()+1, "snotequal", 9) != 0)
        break;
      return std::make_pair(855, 5);	 // "isnotequal"
    case 'n':	 // 8 strings to match.
      switch (Name[1]) {
      default: break;
      case 'a':	 // 5 strings to match.
        if (memcmp(Name.data()+2, "tive_", 5) != 0)
          break;
        switch (Name[7]) {
        default: break;
        case 'c':	 // 1 string to match.
          if (memcmp(Name.data()+8, "os", 2) != 0)
            break;
          return std::make_pair(781, 1);	 // "native_cos"
        case 'e':	 // 1 string to match.
          if (memcmp(Name.data()+8, "xp", 2) != 0)
            break;
          return std::make_pair(781, 1);	 // "native_exp"
        case 'l':	 // 1 string to match.
          if (memcmp(Name.data()+8, "og", 2) != 0)
            break;
          return std::make_pair(781, 1);	 // "native_log"
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+8, "in", 2) != 0)
            break;
          return std::make_pair(781, 1);	 // "native_sin"
        case 't':	 // 1 string to match.
          if (memcmp(Name.data()+8, "an", 2) != 0)
            break;
          return std::make_pair(781, 1);	 // "native_tan"
        }
        break;
      case 'd':	 // 3 strings to match.
        if (memcmp(Name.data()+2, "range_", 6) != 0)
          break;
        switch (Name[8]) {
        default: break;
        case '1':	 // 1 string to match.
          if (Name[9] != 'D')
            break;
          return std::make_pair(2714, 3);	 // "ndrange_1D"
        case '2':	 // 1 string to match.
          if (Name[9] != 'D')
            break;
          return std::make_pair(2717, 3);	 // "ndrange_2D"
        case '3':	 // 1 string to match.
          if (Name[9] != 'D')
            break;
          return std::make_pair(2717, 3);	 // "ndrange_3D"
        }
        break;
      }
      break;
    case 's':	 // 1 string to match.
      if (memcmp(Name.data()+1, "moothstep", 9) != 0)
        break;
      return std::make_pair(820, 4);	 // "smoothstep"
    case 'v':	 // 1 string to match.
      if (memcmp(Name.data()+1, "load_half", 9) != 0)
        break;
      return std::make_pair(1366, 5);	 // "vload_half"
    }
    break;
  case 11:	 // 28 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 9 strings to match.
      switch (Name[1]) {
      default: break;
      case 'm':	 // 5 strings to match.
        if (memcmp(Name.data()+2, "d_", 2) != 0)
          break;
        switch (Name[4]) {
        default: break;
        case 'm':	 // 1 string to match.
          if (memcmp(Name.data()+5, "edian3", 6) != 0)
            break;
          return std::make_pair(3016, 3);	 // "amd_median3"
        case 'u':	 // 4 strings to match.
          if (memcmp(Name.data()+5, "npack", 5) != 0)
            break;
          switch (Name[10]) {
          default: break;
          case '0':	 // 1 string to match.
            return std::make_pair(3009, 1);	 // "amd_unpack0"
          case '1':	 // 1 string to match.
            return std::make_pair(3009, 1);	 // "amd_unpack1"
          case '2':	 // 1 string to match.
            return std::make_pair(3009, 1);	 // "amd_unpack2"
          case '3':	 // 1 string to match.
            return std::make_pair(3009, 1);	 // "amd_unpack3"
          }
          break;
        }
        break;
      case 'r':	 // 1 string to match.
        if (memcmp(Name.data()+2, "m_dot_acc", 9) != 0)
          break;
        return std::make_pair(3002, 4);	 // "arm_dot_acc"
      case 't':	 // 3 strings to match.
        if (memcmp(Name.data()+2, "omic_", 5) != 0)
          break;
        switch (Name[7]) {
        default: break;
        case 'i':	 // 1 string to match.
          if (memcmp(Name.data()+8, "nit", 3) != 0)
            break;
          return std::make_pair(1578, 18);	 // "atomic_init"
        case 'l':	 // 1 string to match.
          if (memcmp(Name.data()+8, "oad", 3) != 0)
            break;
          return std::make_pair(1723, 21);	 // "atomic_load"
        case 'x':	 // 1 string to match.
          if (memcmp(Name.data()+8, "chg", 3) != 0)
            break;
          return std::make_pair(1489, 8);	 // "atomic_xchg"
        }
        break;
      }
      break;
    case 'b':	 // 1 string to match.
      if (memcmp(Name.data()+1, "it_reverse", 10) != 0)
        break;
      return std::make_pair(2787, 1);	 // "bit_reverse"
    case 'c':	 // 1 string to match.
      if (memcmp(Name.data()+1, "onvert_int", 10) != 0)
        break;
      return std::make_pair(463, 11);	 // "convert_int"
    case 'd':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ot_acc_sat", 10) != 0)
        break;
      return std::make_pair(2788, 4);	 // "dot_acc_sat"
    case 'f':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ast_length", 10) != 0)
        break;
      return std::make_pair(830, 1);	 // "fast_length"
    case 'h':	 // 1 string to match.
      if (memcmp(Name.data()+1, "alf_divide", 10) != 0)
        break;
      return std::make_pair(782, 1);	 // "half_divide"
    case 'i':	 // 2 strings to match.
      if (Name[1] != 's')
        break;
      switch (Name[2]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+3, "essequal", 8) != 0)
          break;
        return std::make_pair(855, 5);	 // "islessequal"
      case 'u':	 // 1 string to match.
        if (memcmp(Name.data()+3, "nordered", 8) != 0)
          break;
        return std::make_pair(855, 5);	 // "isunordered"
      }
      break;
    case 'n':	 // 4 strings to match.
      if (memcmp(Name.data()+1, "ative_", 6) != 0)
        break;
      switch (Name[7]) {
      default: break;
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+8, "xp2", 3) != 0)
          break;
        return std::make_pair(781, 1);	 // "native_exp2"
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+8, "og2", 3) != 0)
          break;
        return std::make_pair(781, 1);	 // "native_log2"
      case 'p':	 // 1 string to match.
        if (memcmp(Name.data()+8, "owr", 3) != 0)
          break;
        return std::make_pair(782, 1);	 // "native_powr"
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+8, "qrt", 3) != 0)
          break;
        return std::make_pair(781, 1);	 // "native_sqrt"
      }
      break;
    case 'r':	 // 3 strings to match.
      if (memcmp(Name.data()+1, "ead_image", 9) != 0)
        break;
      switch (Name[10]) {
      default: break;
      case 'f':	 // 1 string to match.
        return std::make_pair(2327, 66);	 // "read_imagef"
      case 'h':	 // 1 string to match.
        return std::make_pair(2667, 22);	 // "read_imageh"
      case 'i':	 // 1 string to match.
        return std::make_pair(2393, 46);	 // "read_imagei"
      }
      break;
    case 'v':	 // 5 strings to match.
      switch (Name[1]) {
      default: break;
      case 'l':	 // 4 strings to match.
        if (memcmp(Name.data()+2, "oad_half", 8) != 0)
          break;
        switch (Name[10]) {
        default: break;
        case '2':	 // 1 string to match.
          return std::make_pair(1371, 5);	 // "vload_half2"
        case '3':	 // 1 string to match.
          return std::make_pair(1392, 5);	 // "vload_half3"
        case '4':	 // 1 string to match.
          return std::make_pair(1397, 5);	 // "vload_half4"
        case '8':	 // 1 string to match.
          return std::make_pair(1402, 5);	 // "vload_half8"
        }
        break;
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+2, "tore_half", 9) != 0)
          break;
        return std::make_pair(1376, 8);	 // "vstore_half"
      }
      break;
    }
    break;
  case 12:	 // 32 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'm':	 // 1 string to match.
        if (memcmp(Name.data()+2, "d_bitalign", 10) != 0)
          break;
        return std::make_pair(3008, 1);	 // "amd_bitalign"
      case 't':	 // 2 strings to match.
        if (memcmp(Name.data()+2, "om", 2) != 0)
          break;
        switch (Name[4]) {
        default: break;
        case '_':	 // 1 string to match.
          if (memcmp(Name.data()+5, "cmpxchg", 7) != 0)
            break;
          return std::make_pair(1473, 8);	 // "atom_cmpxchg"
        case 'i':	 // 1 string to match.
          if (memcmp(Name.data()+5, "c_store", 7) != 0)
            break;
          return std::make_pair(1515, 21);	 // "atomic_store"
        }
        break;
      }
      break;
    case 'c':	 // 8 strings to match.
      if (memcmp(Name.data()+1, "onvert_", 7) != 0)
        break;
      switch (Name[8]) {
      default: break;
      case 'c':	 // 1 string to match.
        if (memcmp(Name.data()+9, "har", 3) != 0)
          break;
        return std::make_pair(199, 11);	 // "convert_char"
      case 'h':	 // 1 string to match.
        if (memcmp(Name.data()+9, "alf", 3) != 0)
          break;
        return std::make_pair(133, 11);	 // "convert_half"
      case 'i':	 // 4 strings to match.
        if (memcmp(Name.data()+9, "nt", 2) != 0)
          break;
        switch (Name[11]) {
        default: break;
        case '2':	 // 1 string to match.
          return std::make_pair(474, 11);	 // "convert_int2"
        case '3':	 // 1 string to match.
          return std::make_pair(485, 11);	 // "convert_int3"
        case '4':	 // 1 string to match.
          return std::make_pair(496, 11);	 // "convert_int4"
        case '8':	 // 1 string to match.
          return std::make_pair(507, 11);	 // "convert_int8"
        }
        break;
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+9, "ong", 3) != 0)
          break;
        return std::make_pair(595, 11);	 // "convert_long"
      case 'u':	 // 1 string to match.
        if (memcmp(Name.data()+9, "int", 3) != 0)
          break;
        return std::make_pair(529, 11);	 // "convert_uint"
      }
      break;
    case 'g':	 // 3 strings to match.
      if (memcmp(Name.data()+1, "et_", 3) != 0)
        break;
      switch (Name[4]) {
      default: break;
      case 'g':	 // 1 string to match.
        if (memcmp(Name.data()+5, "roup_id", 7) != 0)
          break;
        return std::make_pair(728, 1);	 // "get_group_id"
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+5, "ocal_id", 7) != 0)
          break;
        return std::make_pair(728, 1);	 // "get_local_id"
      case 'w':	 // 1 string to match.
        if (memcmp(Name.data()+5, "ork_dim", 7) != 0)
          break;
        return std::make_pair(727, 1);	 // "get_work_dim"
      }
      break;
    case 'n':	 // 4 strings to match.
      if (memcmp(Name.data()+1, "ative_", 6) != 0)
        break;
      switch (Name[7]) {
      default: break;
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+8, "xp10", 4) != 0)
          break;
        return std::make_pair(781, 1);	 // "native_exp10"
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+8, "og10", 4) != 0)
          break;
        return std::make_pair(781, 1);	 // "native_log10"
      case 'r':	 // 2 strings to match.
        switch (Name[8]) {
        default: break;
        case 'e':	 // 1 string to match.
          if (memcmp(Name.data()+9, "cip", 3) != 0)
            break;
          return std::make_pair(781, 1);	 // "native_recip"
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+9, "qrt", 3) != 0)
            break;
          return std::make_pair(781, 1);	 // "native_rsqrt"
        }
        break;
      }
      break;
    case 'r':	 // 2 strings to match.
      if (Name[1] != 'e')
        break;
      switch (Name[2]) {
      default: break;
      case 'a':	 // 1 string to match.
        if (memcmp(Name.data()+3, "d_imageui", 9) != 0)
          break;
        return std::make_pair(2439, 46);	 // "read_imageui"
      case 't':	 // 1 string to match.
        if (memcmp(Name.data()+3, "ain_event", 9) != 0)
          break;
        return std::make_pair(2708, 1);	 // "retain_event"
      }
      break;
    case 'v':	 // 9 strings to match.
      switch (Name[1]) {
      default: break;
      case 'l':	 // 5 strings to match.
        if (memcmp(Name.data()+2, "oad", 3) != 0)
          break;
        switch (Name[5]) {
        default: break;
        case '_':	 // 1 string to match.
          if (memcmp(Name.data()+6, "half16", 6) != 0)
            break;
          return std::make_pair(1407, 5);	 // "vload_half16"
        case 'a':	 // 4 strings to match.
          if (memcmp(Name.data()+6, "_half", 5) != 0)
            break;
          switch (Name[11]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(1371, 5);	 // "vloada_half2"
          case '3':	 // 1 string to match.
            return std::make_pair(1392, 5);	 // "vloada_half3"
          case '4':	 // 1 string to match.
            return std::make_pair(1397, 5);	 // "vloada_half4"
          case '8':	 // 1 string to match.
            return std::make_pair(1402, 5);	 // "vloada_half8"
          }
          break;
        }
        break;
      case 's':	 // 4 strings to match.
        if (memcmp(Name.data()+2, "tore_half", 9) != 0)
          break;
        switch (Name[11]) {
        default: break;
        case '2':	 // 1 string to match.
          return std::make_pair(1384, 8);	 // "vstore_half2"
        case '3':	 // 1 string to match.
          return std::make_pair(1412, 8);	 // "vstore_half3"
        case '4':	 // 1 string to match.
          return std::make_pair(1420, 8);	 // "vstore_half4"
        case '8':	 // 1 string to match.
          return std::make_pair(1428, 8);	 // "vstore_half8"
        }
        break;
      }
      break;
    case 'w':	 // 3 strings to match.
      if (memcmp(Name.data()+1, "rite_image", 10) != 0)
        break;
      switch (Name[11]) {
      default: break;
      case 'f':	 // 1 string to match.
        return std::make_pair(2485, 30);	 // "write_imagef"
      case 'h':	 // 1 string to match.
        return std::make_pair(2689, 12);	 // "write_imageh"
      case 'i':	 // 1 string to match.
        return std::make_pair(2515, 22);	 // "write_imagei"
      }
      break;
    }
    break;
  case 13:	 // 37 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 1 string to match.
      if (memcmp(Name.data()+1, "md_bytealign", 12) != 0)
        break;
      return std::make_pair(3008, 1);	 // "amd_bytealign"
    case 'c':	 // 21 strings to match.
      if (memcmp(Name.data()+1, "onvert_", 7) != 0)
        break;
      switch (Name[8]) {
      default: break;
      case 'c':	 // 4 strings to match.
        if (memcmp(Name.data()+9, "har", 3) != 0)
          break;
        switch (Name[12]) {
        default: break;
        case '2':	 // 1 string to match.
          return std::make_pair(210, 11);	 // "convert_char2"
        case '3':	 // 1 string to match.
          return std::make_pair(221, 11);	 // "convert_char3"
        case '4':	 // 1 string to match.
          return std::make_pair(232, 11);	 // "convert_char4"
        case '8':	 // 1 string to match.
          return std::make_pair(243, 11);	 // "convert_char8"
        }
        break;
      case 'f':	 // 1 string to match.
        if (memcmp(Name.data()+9, "loat", 4) != 0)
          break;
        return std::make_pair(1, 11);	 // "convert_float"
      case 'h':	 // 4 strings to match.
        if (memcmp(Name.data()+9, "alf", 3) != 0)
          break;
        switch (Name[12]) {
        default: break;
        case '2':	 // 1 string to match.
          return std::make_pair(144, 11);	 // "convert_half2"
        case '3':	 // 1 string to match.
          return std::make_pair(155, 11);	 // "convert_half3"
        case '4':	 // 1 string to match.
          return std::make_pair(166, 11);	 // "convert_half4"
        case '8':	 // 1 string to match.
          return std::make_pair(177, 11);	 // "convert_half8"
        }
        break;
      case 'i':	 // 1 string to match.
        if (memcmp(Name.data()+9, "nt16", 4) != 0)
          break;
        return std::make_pair(518, 11);	 // "convert_int16"
      case 'l':	 // 4 strings to match.
        if (memcmp(Name.data()+9, "ong", 3) != 0)
          break;
        switch (Name[12]) {
        default: break;
        case '2':	 // 1 string to match.
          return std::make_pair(606, 11);	 // "convert_long2"
        case '3':	 // 1 string to match.
          return std::make_pair(617, 11);	 // "convert_long3"
        case '4':	 // 1 string to match.
          return std::make_pair(628, 11);	 // "convert_long4"
        case '8':	 // 1 string to match.
          return std::make_pair(639, 11);	 // "convert_long8"
        }
        break;
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+9, "hort", 4) != 0)
          break;
        return std::make_pair(331, 11);	 // "convert_short"
      case 'u':	 // 6 strings to match.
        switch (Name[9]) {
        default: break;
        case 'c':	 // 1 string to match.
          if (memcmp(Name.data()+10, "har", 3) != 0)
            break;
          return std::make_pair(265, 11);	 // "convert_uchar"
        case 'i':	 // 4 strings to match.
          if (memcmp(Name.data()+10, "nt", 2) != 0)
            break;
          switch (Name[12]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(540, 11);	 // "convert_uint2"
          case '3':	 // 1 string to match.
            return std::make_pair(551, 11);	 // "convert_uint3"
          case '4':	 // 1 string to match.
            return std::make_pair(562, 11);	 // "convert_uint4"
          case '8':	 // 1 string to match.
            return std::make_pair(573, 11);	 // "convert_uint8"
          }
          break;
        case 'l':	 // 1 string to match.
          if (memcmp(Name.data()+10, "ong", 3) != 0)
            break;
          return std::make_pair(661, 11);	 // "convert_ulong"
        }
        break;
      }
      break;
    case 'f':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ast_distance", 12) != 0)
        break;
      return std::make_pair(848, 1);	 // "fast_distance"
    case 'g':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "et_", 3) != 0)
        break;
      switch (Name[4]) {
      default: break;
      case 'g':	 // 1 string to match.
        if (memcmp(Name.data()+5, "lobal_id", 8) != 0)
          break;
        return std::make_pair(728, 1);	 // "get_global_id"
      case 'i':	 // 1 string to match.
        if (memcmp(Name.data()+5, "mage_dim", 8) != 0)
          break;
        return std::make_pair(2625, 27);	 // "get_image_dim"
      }
      break;
    case 'i':	 // 1 string to match.
      if (memcmp(Name.data()+1, "slessgreater", 12) != 0)
        break;
      return std::make_pair(855, 5);	 // "islessgreater"
    case 'n':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ative_divide", 12) != 0)
        break;
      return std::make_pair(782, 1);	 // "native_divide"
    case 'r':	 // 1 string to match.
      if (memcmp(Name.data()+1, "elease_event", 12) != 0)
        break;
      return std::make_pair(2708, 1);	 // "release_event"
    case 's':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "ub_group_a", 10) != 0)
        break;
      switch (Name[11]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (Name[12] != 'l')
          break;
        return std::make_pair(2757, 1);	 // "sub_group_all"
      case 'n':	 // 1 string to match.
        if (Name[12] != 'y')
          break;
        return std::make_pair(2757, 1);	 // "sub_group_any"
      }
      break;
    case 'v':	 // 6 strings to match.
      switch (Name[1]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+2, "oada_half16", 11) != 0)
          break;
        return std::make_pair(1407, 5);	 // "vloada_half16"
      case 's':	 // 5 strings to match.
        if (memcmp(Name.data()+2, "tore", 4) != 0)
          break;
        switch (Name[6]) {
        default: break;
        case '_':	 // 1 string to match.
          if (memcmp(Name.data()+7, "half16", 6) != 0)
            break;
          return std::make_pair(1436, 8);	 // "vstore_half16"
        case 'a':	 // 4 strings to match.
          if (memcmp(Name.data()+7, "_half", 5) != 0)
            break;
          switch (Name[12]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(1384, 8);	 // "vstorea_half2"
          case '3':	 // 1 string to match.
            return std::make_pair(1412, 8);	 // "vstorea_half3"
          case '4':	 // 1 string to match.
            return std::make_pair(1420, 8);	 // "vstorea_half4"
          case '8':	 // 1 string to match.
            return std::make_pair(1428, 8);	 // "vstorea_half8"
          }
          break;
        }
        break;
      }
      break;
    case 'w':	 // 1 string to match.
      if (memcmp(Name.data()+1, "rite_imageui", 12) != 0)
        break;
      return std::make_pair(2537, 22);	 // "write_imageui"
    }
    break;
  case 14:	 // 33 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 1 string to match.
      if (memcmp(Name.data()+1, "tomic_cmpxchg", 13) != 0)
        break;
      return std::make_pair(1509, 6);	 // "atomic_cmpxchg"
    case 'c':	 // 22 strings to match.
      if (memcmp(Name.data()+1, "onvert_", 7) != 0)
        break;
      switch (Name[8]) {
      default: break;
      case 'c':	 // 1 string to match.
        if (memcmp(Name.data()+9, "har16", 5) != 0)
          break;
        return std::make_pair(254, 11);	 // "convert_char16"
      case 'd':	 // 1 string to match.
        if (memcmp(Name.data()+9, "ouble", 5) != 0)
          break;
        return std::make_pair(67, 11);	 // "convert_double"
      case 'f':	 // 4 strings to match.
        if (memcmp(Name.data()+9, "loat", 4) != 0)
          break;
        switch (Name[13]) {
        default: break;
        case '2':	 // 1 string to match.
          return std::make_pair(12, 11);	 // "convert_float2"
        case '3':	 // 1 string to match.
          return std::make_pair(23, 11);	 // "convert_float3"
        case '4':	 // 1 string to match.
          return std::make_pair(34, 11);	 // "convert_float4"
        case '8':	 // 1 string to match.
          return std::make_pair(45, 11);	 // "convert_float8"
        }
        break;
      case 'h':	 // 1 string to match.
        if (memcmp(Name.data()+9, "alf16", 5) != 0)
          break;
        return std::make_pair(188, 11);	 // "convert_half16"
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+9, "ong16", 5) != 0)
          break;
        return std::make_pair(650, 11);	 // "convert_long16"
      case 's':	 // 4 strings to match.
        if (memcmp(Name.data()+9, "hort", 4) != 0)
          break;
        switch (Name[13]) {
        default: break;
        case '2':	 // 1 string to match.
          return std::make_pair(342, 11);	 // "convert_short2"
        case '3':	 // 1 string to match.
          return std::make_pair(353, 11);	 // "convert_short3"
        case '4':	 // 1 string to match.
          return std::make_pair(364, 11);	 // "convert_short4"
        case '8':	 // 1 string to match.
          return std::make_pair(375, 11);	 // "convert_short8"
        }
        break;
      case 'u':	 // 10 strings to match.
        switch (Name[9]) {
        default: break;
        case 'c':	 // 4 strings to match.
          if (memcmp(Name.data()+10, "har", 3) != 0)
            break;
          switch (Name[13]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(276, 11);	 // "convert_uchar2"
          case '3':	 // 1 string to match.
            return std::make_pair(287, 11);	 // "convert_uchar3"
          case '4':	 // 1 string to match.
            return std::make_pair(298, 11);	 // "convert_uchar4"
          case '8':	 // 1 string to match.
            return std::make_pair(309, 11);	 // "convert_uchar8"
          }
          break;
        case 'i':	 // 1 string to match.
          if (memcmp(Name.data()+10, "nt16", 4) != 0)
            break;
          return std::make_pair(584, 11);	 // "convert_uint16"
        case 'l':	 // 4 strings to match.
          if (memcmp(Name.data()+10, "ong", 3) != 0)
            break;
          switch (Name[13]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(672, 11);	 // "convert_ulong2"
          case '3':	 // 1 string to match.
            return std::make_pair(683, 11);	 // "convert_ulong3"
          case '4':	 // 1 string to match.
            return std::make_pair(694, 11);	 // "convert_ulong4"
          case '8':	 // 1 string to match.
            return std::make_pair(705, 11);	 // "convert_ulong8"
          }
          break;
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+10, "hort", 4) != 0)
            break;
          return std::make_pair(397, 11);	 // "convert_ushort"
        }
        break;
      }
      break;
    case 'e':	 // 1 string to match.
      if (memcmp(Name.data()+1, "nqueue_marker", 13) != 0)
        break;
      return std::make_pair(2707, 1);	 // "enqueue_marker"
    case 'f':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ast_normalize", 13) != 0)
        break;
      return std::make_pair(831, 1);	 // "fast_normalize"
    case 'g':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "et_", 3) != 0)
        break;
      switch (Name[4]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+5, "ocal_size", 9) != 0)
          break;
        return std::make_pair(728, 1);	 // "get_local_size"
      case 'n':	 // 1 string to match.
        if (memcmp(Name.data()+5, "um_groups", 9) != 0)
          break;
        return std::make_pair(728, 1);	 // "get_num_groups"
      }
      break;
    case 'i':	 // 2 strings to match.
      if (Name[1] != 's')
        break;
      switch (Name[2]) {
      default: break;
      case '_':	 // 1 string to match.
        if (memcmp(Name.data()+3, "valid_event", 11) != 0)
          break;
        return std::make_pair(2710, 1);	 // "is_valid_event"
      case 'g':	 // 1 string to match.
        if (memcmp(Name.data()+3, "reaterequal", 11) != 0)
          break;
        return std::make_pair(855, 5);	 // "isgreaterequal"
      }
      break;
    case 'r':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ead_mem_fence", 13) != 0)
        break;
      return std::make_pair(1447, 1);	 // "read_mem_fence"
    case 'v':	 // 1 string to match.
      if (memcmp(Name.data()+1, "storea_half16", 13) != 0)
        break;
      return std::make_pair(1436, 8);	 // "vstorea_half16"
    case 'w':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "ork_group_a", 11) != 0)
        break;
      switch (Name[12]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (Name[13] != 'l')
          break;
        return std::make_pair(2701, 1);	 // "work_group_all"
      case 'n':	 // 1 string to match.
        if (Name[13] != 'y')
          break;
        return std::make_pair(2701, 1);	 // "work_group_any"
      }
      break;
    }
    break;
  case 15:	 // 30 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'r':	 // 1 string to match.
        if (memcmp(Name.data()+2, "m_dot_acc_sat", 13) != 0)
          break;
        return std::make_pair(3006, 2);	 // "arm_dot_acc_sat"
      case 't':	 // 2 strings to match.
        if (memcmp(Name.data()+2, "omic_", 5) != 0)
          break;
        switch (Name[7]) {
        default: break;
        case 'e':	 // 1 string to match.
          if (memcmp(Name.data()+8, "xchange", 7) != 0)
            break;
          return std::make_pair(1786, 21);	 // "atomic_exchange"
        case 'f':	 // 1 string to match.
          if (memcmp(Name.data()+8, "etch_or", 7) != 0)
            break;
          return std::make_pair(1921, 12);	 // "atomic_fetch_or"
        }
        break;
      }
      break;
    case 'b':	 // 1 string to match.
      if (memcmp(Name.data()+1, "itfield_insert", 14) != 0)
        break;
      return std::make_pair(2782, 1);	 // "bitfield_insert"
    case 'c':	 // 17 strings to match.
      if (memcmp(Name.data()+1, "onvert_", 7) != 0)
        break;
      switch (Name[8]) {
      default: break;
      case 'd':	 // 4 strings to match.
        if (memcmp(Name.data()+9, "ouble", 5) != 0)
          break;
        switch (Name[14]) {
        default: break;
        case '2':	 // 1 string to match.
          return std::make_pair(78, 11);	 // "convert_double2"
        case '3':	 // 1 string to match.
          return std::make_pair(89, 11);	 // "convert_double3"
        case '4':	 // 1 string to match.
          return std::make_pair(100, 11);	 // "convert_double4"
        case '8':	 // 1 string to match.
          return std::make_pair(111, 11);	 // "convert_double8"
        }
        break;
      case 'f':	 // 1 string to match.
        if (memcmp(Name.data()+9, "loat16", 6) != 0)
          break;
        return std::make_pair(56, 11);	 // "convert_float16"
      case 'i':	 // 5 strings to match.
        if (memcmp(Name.data()+9, "nt_", 3) != 0)
          break;
        switch (Name[12]) {
        default: break;
        case 'r':	 // 4 strings to match.
          if (Name[13] != 't')
            break;
          switch (Name[14]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(463, 11);	 // "convert_int_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(463, 11);	 // "convert_int_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(463, 11);	 // "convert_int_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(463, 11);	 // "convert_int_rtz"
          }
          break;
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+13, "at", 2) != 0)
            break;
          return std::make_pair(463, 11);	 // "convert_int_sat"
        }
        break;
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+9, "hort16", 6) != 0)
          break;
        return std::make_pair(386, 11);	 // "convert_short16"
      case 'u':	 // 6 strings to match.
        switch (Name[9]) {
        default: break;
        case 'c':	 // 1 string to match.
          if (memcmp(Name.data()+10, "har16", 5) != 0)
            break;
          return std::make_pair(320, 11);	 // "convert_uchar16"
        case 'l':	 // 1 string to match.
          if (memcmp(Name.data()+10, "ong16", 5) != 0)
            break;
          return std::make_pair(716, 11);	 // "convert_ulong16"
        case 's':	 // 4 strings to match.
          if (memcmp(Name.data()+10, "hort", 4) != 0)
            break;
          switch (Name[14]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(408, 11);	 // "convert_ushort2"
          case '3':	 // 1 string to match.
            return std::make_pair(419, 11);	 // "convert_ushort3"
          case '4':	 // 1 string to match.
            return std::make_pair(430, 11);	 // "convert_ushort4"
          case '8':	 // 1 string to match.
            return std::make_pair(441, 11);	 // "convert_ushort8"
          }
          break;
        }
        break;
      }
      break;
    case 'g':	 // 3 strings to match.
      if (memcmp(Name.data()+1, "et_", 3) != 0)
        break;
      switch (Name[4]) {
      default: break;
      case 'g':	 // 1 string to match.
        if (memcmp(Name.data()+5, "lobal_size", 10) != 0)
          break;
        return std::make_pair(728, 1);	 // "get_global_size"
      case 'i':	 // 2 strings to match.
        if (memcmp(Name.data()+5, "mage_", 5) != 0)
          break;
        switch (Name[10]) {
        default: break;
        case 'd':	 // 1 string to match.
          if (memcmp(Name.data()+11, "epth", 4) != 0)
            break;
          return std::make_pair(2622, 3);	 // "get_image_depth"
        case 'w':	 // 1 string to match.
          if (memcmp(Name.data()+11, "idth", 4) != 0)
            break;
          return std::make_pair(2559, 36);	 // "get_image_width"
        }
        break;
      }
      break;
    case 's':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ub_group_elect", 14) != 0)
        break;
      return std::make_pair(2763, 1);	 // "sub_group_elect"
    case 'v':	 // 4 strings to match.
      if (memcmp(Name.data()+1, "store_half_rt", 13) != 0)
        break;
      switch (Name[14]) {
      default: break;
      case 'e':	 // 1 string to match.
        return std::make_pair(1376, 8);	 // "vstore_half_rte"
      case 'n':	 // 1 string to match.
        return std::make_pair(1376, 8);	 // "vstore_half_rtn"
      case 'p':	 // 1 string to match.
        return std::make_pair(1376, 8);	 // "vstore_half_rtp"
      case 'z':	 // 1 string to match.
        return std::make_pair(1376, 8);	 // "vstore_half_rtz"
      }
      break;
    case 'w':	 // 1 string to match.
      if (memcmp(Name.data()+1, "rite_mem_fence", 14) != 0)
        break;
      return std::make_pair(1447, 1);	 // "write_mem_fence"
    }
    break;
  case 16:	 // 67 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 6 strings to match.
      if (memcmp(Name.data()+1, "tomic_fetch_", 12) != 0)
        break;
      switch (Name[13]) {
      default: break;
      case 'a':	 // 2 strings to match.
        switch (Name[14]) {
        default: break;
        case 'd':	 // 1 string to match.
          if (Name[15] != 'd')
            break;
          return std::make_pair(1849, 24);	 // "atomic_fetch_add"
        case 'n':	 // 1 string to match.
          if (Name[15] != 'd')
            break;
          return std::make_pair(1921, 12);	 // "atomic_fetch_and"
        }
        break;
      case 'm':	 // 2 strings to match.
        switch (Name[14]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (Name[15] != 'x')
            break;
          return std::make_pair(1786, 21);	 // "atomic_fetch_max"
        case 'i':	 // 1 string to match.
          if (Name[15] != 'n')
            break;
          return std::make_pair(1786, 21);	 // "atomic_fetch_min"
        }
        break;
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+14, "ub", 2) != 0)
          break;
        return std::make_pair(1849, 24);	 // "atomic_fetch_sub"
      case 'x':	 // 1 string to match.
        if (memcmp(Name.data()+14, "or", 2) != 0)
          break;
        return std::make_pair(1921, 12);	 // "atomic_fetch_xor"
      }
      break;
    case 'c':	 // 41 strings to match.
      if (memcmp(Name.data()+1, "onvert_", 7) != 0)
        break;
      switch (Name[8]) {
      default: break;
      case 'c':	 // 5 strings to match.
        if (memcmp(Name.data()+9, "har_", 4) != 0)
          break;
        switch (Name[13]) {
        default: break;
        case 'r':	 // 4 strings to match.
          if (Name[14] != 't')
            break;
          switch (Name[15]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(199, 11);	 // "convert_char_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(199, 11);	 // "convert_char_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(199, 11);	 // "convert_char_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(199, 11);	 // "convert_char_rtz"
          }
          break;
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+14, "at", 2) != 0)
            break;
          return std::make_pair(199, 11);	 // "convert_char_sat"
        }
        break;
      case 'd':	 // 1 string to match.
        if (memcmp(Name.data()+9, "ouble16", 7) != 0)
          break;
        return std::make_pair(122, 11);	 // "convert_double16"
      case 'h':	 // 4 strings to match.
        if (memcmp(Name.data()+9, "alf_rt", 6) != 0)
          break;
        switch (Name[15]) {
        default: break;
        case 'e':	 // 1 string to match.
          return std::make_pair(133, 11);	 // "convert_half_rte"
        case 'n':	 // 1 string to match.
          return std::make_pair(133, 11);	 // "convert_half_rtn"
        case 'p':	 // 1 string to match.
          return std::make_pair(133, 11);	 // "convert_half_rtp"
        case 'z':	 // 1 string to match.
          return std::make_pair(133, 11);	 // "convert_half_rtz"
        }
        break;
      case 'i':	 // 20 strings to match.
        if (memcmp(Name.data()+9, "nt", 2) != 0)
          break;
        switch (Name[11]) {
        default: break;
        case '2':	 // 5 strings to match.
          if (Name[12] != '_')
            break;
          switch (Name[13]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[14] != 't')
              break;
            switch (Name[15]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(474, 11);	 // "convert_int2_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(474, 11);	 // "convert_int2_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(474, 11);	 // "convert_int2_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(474, 11);	 // "convert_int2_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+14, "at", 2) != 0)
              break;
            return std::make_pair(474, 11);	 // "convert_int2_sat"
          }
          break;
        case '3':	 // 5 strings to match.
          if (Name[12] != '_')
            break;
          switch (Name[13]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[14] != 't')
              break;
            switch (Name[15]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(485, 11);	 // "convert_int3_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(485, 11);	 // "convert_int3_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(485, 11);	 // "convert_int3_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(485, 11);	 // "convert_int3_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+14, "at", 2) != 0)
              break;
            return std::make_pair(485, 11);	 // "convert_int3_sat"
          }
          break;
        case '4':	 // 5 strings to match.
          if (Name[12] != '_')
            break;
          switch (Name[13]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[14] != 't')
              break;
            switch (Name[15]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(496, 11);	 // "convert_int4_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(496, 11);	 // "convert_int4_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(496, 11);	 // "convert_int4_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(496, 11);	 // "convert_int4_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+14, "at", 2) != 0)
              break;
            return std::make_pair(496, 11);	 // "convert_int4_sat"
          }
          break;
        case '8':	 // 5 strings to match.
          if (Name[12] != '_')
            break;
          switch (Name[13]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[14] != 't')
              break;
            switch (Name[15]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(507, 11);	 // "convert_int8_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(507, 11);	 // "convert_int8_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(507, 11);	 // "convert_int8_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(507, 11);	 // "convert_int8_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+14, "at", 2) != 0)
              break;
            return std::make_pair(507, 11);	 // "convert_int8_sat"
          }
          break;
        }
        break;
      case 'l':	 // 5 strings to match.
        if (memcmp(Name.data()+9, "ong_", 4) != 0)
          break;
        switch (Name[13]) {
        default: break;
        case 'r':	 // 4 strings to match.
          if (Name[14] != 't')
            break;
          switch (Name[15]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(595, 11);	 // "convert_long_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(595, 11);	 // "convert_long_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(595, 11);	 // "convert_long_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(595, 11);	 // "convert_long_rtz"
          }
          break;
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+14, "at", 2) != 0)
            break;
          return std::make_pair(595, 11);	 // "convert_long_sat"
        }
        break;
      case 'u':	 // 6 strings to match.
        switch (Name[9]) {
        default: break;
        case 'i':	 // 5 strings to match.
          if (memcmp(Name.data()+10, "nt_", 3) != 0)
            break;
          switch (Name[13]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[14] != 't')
              break;
            switch (Name[15]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(529, 11);	 // "convert_uint_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(529, 11);	 // "convert_uint_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(529, 11);	 // "convert_uint_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(529, 11);	 // "convert_uint_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+14, "at", 2) != 0)
              break;
            return std::make_pair(529, 11);	 // "convert_uint_sat"
          }
          break;
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+10, "hort16", 6) != 0)
            break;
          return std::make_pair(452, 11);	 // "convert_ushort16"
        }
        break;
      }
      break;
    case 'g':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "et_", 3) != 0)
        break;
      switch (Name[4]) {
      default: break;
      case 'i':	 // 1 string to match.
        if (memcmp(Name.data()+5, "mage_height", 11) != 0)
          break;
        return std::make_pair(2595, 27);	 // "get_image_height"
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+5, "ub_group_id", 11) != 0)
          break;
        return std::make_pair(2753, 1);	 // "get_sub_group_id"
      }
      break;
    case 's':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "ub_group_", 9) != 0)
        break;
      switch (Name[10]) {
      default: break;
      case 'b':	 // 1 string to match.
        if (memcmp(Name.data()+11, "allot", 5) != 0)
          break;
        return std::make_pair(2768, 1);	 // "sub_group_ballot"
      case 'r':	 // 1 string to match.
        if (memcmp(Name.data()+11, "otate", 5) != 0)
          break;
        return std::make_pair(2796, 1);	 // "sub_group_rotate"
      }
      break;
    case 'v':	 // 16 strings to match.
      if (memcmp(Name.data()+1, "store_half", 10) != 0)
        break;
      switch (Name[11]) {
      default: break;
      case '2':	 // 4 strings to match.
        if (memcmp(Name.data()+12, "_rt", 3) != 0)
          break;
        switch (Name[15]) {
        default: break;
        case 'e':	 // 1 string to match.
          return std::make_pair(1384, 8);	 // "vstore_half2_rte"
        case 'n':	 // 1 string to match.
          return std::make_pair(1384, 8);	 // "vstore_half2_rtn"
        case 'p':	 // 1 string to match.
          return std::make_pair(1384, 8);	 // "vstore_half2_rtp"
        case 'z':	 // 1 string to match.
          return std::make_pair(1384, 8);	 // "vstore_half2_rtz"
        }
        break;
      case '3':	 // 4 strings to match.
        if (memcmp(Name.data()+12, "_rt", 3) != 0)
          break;
        switch (Name[15]) {
        default: break;
        case 'e':	 // 1 string to match.
          return std::make_pair(1412, 8);	 // "vstore_half3_rte"
        case 'n':	 // 1 string to match.
          return std::make_pair(1412, 8);	 // "vstore_half3_rtn"
        case 'p':	 // 1 string to match.
          return std::make_pair(1412, 8);	 // "vstore_half3_rtp"
        case 'z':	 // 1 string to match.
          return std::make_pair(1412, 8);	 // "vstore_half3_rtz"
        }
        break;
      case '4':	 // 4 strings to match.
        if (memcmp(Name.data()+12, "_rt", 3) != 0)
          break;
        switch (Name[15]) {
        default: break;
        case 'e':	 // 1 string to match.
          return std::make_pair(1420, 8);	 // "vstore_half4_rte"
        case 'n':	 // 1 string to match.
          return std::make_pair(1420, 8);	 // "vstore_half4_rtn"
        case 'p':	 // 1 string to match.
          return std::make_pair(1420, 8);	 // "vstore_half4_rtp"
        case 'z':	 // 1 string to match.
          return std::make_pair(1420, 8);	 // "vstore_half4_rtz"
        }
        break;
      case '8':	 // 4 strings to match.
        if (memcmp(Name.data()+12, "_rt", 3) != 0)
          break;
        switch (Name[15]) {
        default: break;
        case 'e':	 // 1 string to match.
          return std::make_pair(1428, 8);	 // "vstore_half8_rte"
        case 'n':	 // 1 string to match.
          return std::make_pair(1428, 8);	 // "vstore_half8_rtn"
        case 'p':	 // 1 string to match.
          return std::make_pair(1428, 8);	 // "vstore_half8_rtp"
        case 'z':	 // 1 string to match.
          return std::make_pair(1428, 8);	 // "vstore_half8_rtz"
        }
        break;
      }
      break;
    }
    break;
  case 17:	 // 128 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 1 string to match.
      if (memcmp(Name.data()+1, "tomic_flag_clear", 16) != 0)
        break;
      return std::make_pair(1957, 3);	 // "atomic_flag_clear"
    case 'c':	 // 102 strings to match.
      switch (Name[1]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+2, "ock_read_device", 15) != 0)
          break;
        return std::make_pair(2798, 1);	 // "clock_read_device"
      case 'o':	 // 100 strings to match.
        if (memcmp(Name.data()+2, "nvert_", 6) != 0)
          break;
        switch (Name[8]) {
        default: break;
        case 'c':	 // 20 strings to match.
          if (memcmp(Name.data()+9, "har", 3) != 0)
            break;
          switch (Name[12]) {
          default: break;
          case '2':	 // 5 strings to match.
            if (Name[13] != '_')
              break;
            switch (Name[14]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[15] != 't')
                break;
              switch (Name[16]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(210, 11);	 // "convert_char2_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(210, 11);	 // "convert_char2_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(210, 11);	 // "convert_char2_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(210, 11);	 // "convert_char2_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+15, "at", 2) != 0)
                break;
              return std::make_pair(210, 11);	 // "convert_char2_sat"
            }
            break;
          case '3':	 // 5 strings to match.
            if (Name[13] != '_')
              break;
            switch (Name[14]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[15] != 't')
                break;
              switch (Name[16]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(221, 11);	 // "convert_char3_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(221, 11);	 // "convert_char3_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(221, 11);	 // "convert_char3_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(221, 11);	 // "convert_char3_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+15, "at", 2) != 0)
                break;
              return std::make_pair(221, 11);	 // "convert_char3_sat"
            }
            break;
          case '4':	 // 5 strings to match.
            if (Name[13] != '_')
              break;
            switch (Name[14]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[15] != 't')
                break;
              switch (Name[16]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(232, 11);	 // "convert_char4_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(232, 11);	 // "convert_char4_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(232, 11);	 // "convert_char4_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(232, 11);	 // "convert_char4_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+15, "at", 2) != 0)
                break;
              return std::make_pair(232, 11);	 // "convert_char4_sat"
            }
            break;
          case '8':	 // 5 strings to match.
            if (Name[13] != '_')
              break;
            switch (Name[14]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[15] != 't')
                break;
              switch (Name[16]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(243, 11);	 // "convert_char8_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(243, 11);	 // "convert_char8_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(243, 11);	 // "convert_char8_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(243, 11);	 // "convert_char8_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+15, "at", 2) != 0)
                break;
              return std::make_pair(243, 11);	 // "convert_char8_sat"
            }
            break;
          }
          break;
        case 'f':	 // 4 strings to match.
          if (memcmp(Name.data()+9, "loat_rt", 7) != 0)
            break;
          switch (Name[16]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(1, 11);	 // "convert_float_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(1, 11);	 // "convert_float_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(1, 11);	 // "convert_float_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(1, 11);	 // "convert_float_rtz"
          }
          break;
        case 'h':	 // 16 strings to match.
          if (memcmp(Name.data()+9, "alf", 3) != 0)
            break;
          switch (Name[12]) {
          default: break;
          case '2':	 // 4 strings to match.
            if (memcmp(Name.data()+13, "_rt", 3) != 0)
              break;
            switch (Name[16]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(144, 11);	 // "convert_half2_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(144, 11);	 // "convert_half2_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(144, 11);	 // "convert_half2_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(144, 11);	 // "convert_half2_rtz"
            }
            break;
          case '3':	 // 4 strings to match.
            if (memcmp(Name.data()+13, "_rt", 3) != 0)
              break;
            switch (Name[16]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(155, 11);	 // "convert_half3_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(155, 11);	 // "convert_half3_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(155, 11);	 // "convert_half3_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(155, 11);	 // "convert_half3_rtz"
            }
            break;
          case '4':	 // 4 strings to match.
            if (memcmp(Name.data()+13, "_rt", 3) != 0)
              break;
            switch (Name[16]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(166, 11);	 // "convert_half4_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(166, 11);	 // "convert_half4_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(166, 11);	 // "convert_half4_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(166, 11);	 // "convert_half4_rtz"
            }
            break;
          case '8':	 // 4 strings to match.
            if (memcmp(Name.data()+13, "_rt", 3) != 0)
              break;
            switch (Name[16]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(177, 11);	 // "convert_half8_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(177, 11);	 // "convert_half8_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(177, 11);	 // "convert_half8_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(177, 11);	 // "convert_half8_rtz"
            }
            break;
          }
          break;
        case 'i':	 // 5 strings to match.
          if (memcmp(Name.data()+9, "nt16_", 5) != 0)
            break;
          switch (Name[14]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[15] != 't')
              break;
            switch (Name[16]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(518, 11);	 // "convert_int16_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(518, 11);	 // "convert_int16_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(518, 11);	 // "convert_int16_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(518, 11);	 // "convert_int16_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+15, "at", 2) != 0)
              break;
            return std::make_pair(518, 11);	 // "convert_int16_sat"
          }
          break;
        case 'l':	 // 20 strings to match.
          if (memcmp(Name.data()+9, "ong", 3) != 0)
            break;
          switch (Name[12]) {
          default: break;
          case '2':	 // 5 strings to match.
            if (Name[13] != '_')
              break;
            switch (Name[14]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[15] != 't')
                break;
              switch (Name[16]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(606, 11);	 // "convert_long2_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(606, 11);	 // "convert_long2_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(606, 11);	 // "convert_long2_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(606, 11);	 // "convert_long2_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+15, "at", 2) != 0)
                break;
              return std::make_pair(606, 11);	 // "convert_long2_sat"
            }
            break;
          case '3':	 // 5 strings to match.
            if (Name[13] != '_')
              break;
            switch (Name[14]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[15] != 't')
                break;
              switch (Name[16]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(617, 11);	 // "convert_long3_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(617, 11);	 // "convert_long3_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(617, 11);	 // "convert_long3_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(617, 11);	 // "convert_long3_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+15, "at", 2) != 0)
                break;
              return std::make_pair(617, 11);	 // "convert_long3_sat"
            }
            break;
          case '4':	 // 5 strings to match.
            if (Name[13] != '_')
              break;
            switch (Name[14]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[15] != 't')
                break;
              switch (Name[16]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(628, 11);	 // "convert_long4_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(628, 11);	 // "convert_long4_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(628, 11);	 // "convert_long4_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(628, 11);	 // "convert_long4_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+15, "at", 2) != 0)
                break;
              return std::make_pair(628, 11);	 // "convert_long4_sat"
            }
            break;
          case '8':	 // 5 strings to match.
            if (Name[13] != '_')
              break;
            switch (Name[14]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[15] != 't')
                break;
              switch (Name[16]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(639, 11);	 // "convert_long8_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(639, 11);	 // "convert_long8_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(639, 11);	 // "convert_long8_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(639, 11);	 // "convert_long8_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+15, "at", 2) != 0)
                break;
              return std::make_pair(639, 11);	 // "convert_long8_sat"
            }
            break;
          }
          break;
        case 's':	 // 5 strings to match.
          if (memcmp(Name.data()+9, "hort_", 5) != 0)
            break;
          switch (Name[14]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[15] != 't')
              break;
            switch (Name[16]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(331, 11);	 // "convert_short_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(331, 11);	 // "convert_short_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(331, 11);	 // "convert_short_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(331, 11);	 // "convert_short_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+15, "at", 2) != 0)
              break;
            return std::make_pair(331, 11);	 // "convert_short_sat"
          }
          break;
        case 'u':	 // 30 strings to match.
          switch (Name[9]) {
          default: break;
          case 'c':	 // 5 strings to match.
            if (memcmp(Name.data()+10, "har_", 4) != 0)
              break;
            switch (Name[14]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[15] != 't')
                break;
              switch (Name[16]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(265, 11);	 // "convert_uchar_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(265, 11);	 // "convert_uchar_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(265, 11);	 // "convert_uchar_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(265, 11);	 // "convert_uchar_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+15, "at", 2) != 0)
                break;
              return std::make_pair(265, 11);	 // "convert_uchar_sat"
            }
            break;
          case 'i':	 // 20 strings to match.
            if (memcmp(Name.data()+10, "nt", 2) != 0)
              break;
            switch (Name[12]) {
            default: break;
            case '2':	 // 5 strings to match.
              if (Name[13] != '_')
                break;
              switch (Name[14]) {
              default: break;
              case 'r':	 // 4 strings to match.
                if (Name[15] != 't')
                  break;
                switch (Name[16]) {
                default: break;
                case 'e':	 // 1 string to match.
                  return std::make_pair(540, 11);	 // "convert_uint2_rte"
                case 'n':	 // 1 string to match.
                  return std::make_pair(540, 11);	 // "convert_uint2_rtn"
                case 'p':	 // 1 string to match.
                  return std::make_pair(540, 11);	 // "convert_uint2_rtp"
                case 'z':	 // 1 string to match.
                  return std::make_pair(540, 11);	 // "convert_uint2_rtz"
                }
                break;
              case 's':	 // 1 string to match.
                if (memcmp(Name.data()+15, "at", 2) != 0)
                  break;
                return std::make_pair(540, 11);	 // "convert_uint2_sat"
              }
              break;
            case '3':	 // 5 strings to match.
              if (Name[13] != '_')
                break;
              switch (Name[14]) {
              default: break;
              case 'r':	 // 4 strings to match.
                if (Name[15] != 't')
                  break;
                switch (Name[16]) {
                default: break;
                case 'e':	 // 1 string to match.
                  return std::make_pair(551, 11);	 // "convert_uint3_rte"
                case 'n':	 // 1 string to match.
                  return std::make_pair(551, 11);	 // "convert_uint3_rtn"
                case 'p':	 // 1 string to match.
                  return std::make_pair(551, 11);	 // "convert_uint3_rtp"
                case 'z':	 // 1 string to match.
                  return std::make_pair(551, 11);	 // "convert_uint3_rtz"
                }
                break;
              case 's':	 // 1 string to match.
                if (memcmp(Name.data()+15, "at", 2) != 0)
                  break;
                return std::make_pair(551, 11);	 // "convert_uint3_sat"
              }
              break;
            case '4':	 // 5 strings to match.
              if (Name[13] != '_')
                break;
              switch (Name[14]) {
              default: break;
              case 'r':	 // 4 strings to match.
                if (Name[15] != 't')
                  break;
                switch (Name[16]) {
                default: break;
                case 'e':	 // 1 string to match.
                  return std::make_pair(562, 11);	 // "convert_uint4_rte"
                case 'n':	 // 1 string to match.
                  return std::make_pair(562, 11);	 // "convert_uint4_rtn"
                case 'p':	 // 1 string to match.
                  return std::make_pair(562, 11);	 // "convert_uint4_rtp"
                case 'z':	 // 1 string to match.
                  return std::make_pair(562, 11);	 // "convert_uint4_rtz"
                }
                break;
              case 's':	 // 1 string to match.
                if (memcmp(Name.data()+15, "at", 2) != 0)
                  break;
                return std::make_pair(562, 11);	 // "convert_uint4_sat"
              }
              break;
            case '8':	 // 5 strings to match.
              if (Name[13] != '_')
                break;
              switch (Name[14]) {
              default: break;
              case 'r':	 // 4 strings to match.
                if (Name[15] != 't')
                  break;
                switch (Name[16]) {
                default: break;
                case 'e':	 // 1 string to match.
                  return std::make_pair(573, 11);	 // "convert_uint8_rte"
                case 'n':	 // 1 string to match.
                  return std::make_pair(573, 11);	 // "convert_uint8_rtn"
                case 'p':	 // 1 string to match.
                  return std::make_pair(573, 11);	 // "convert_uint8_rtp"
                case 'z':	 // 1 string to match.
                  return std::make_pair(573, 11);	 // "convert_uint8_rtz"
                }
                break;
              case 's':	 // 1 string to match.
                if (memcmp(Name.data()+15, "at", 2) != 0)
                  break;
                return std::make_pair(573, 11);	 // "convert_uint8_sat"
              }
              break;
            }
            break;
          case 'l':	 // 5 strings to match.
            if (memcmp(Name.data()+10, "ong_", 4) != 0)
              break;
            switch (Name[14]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[15] != 't')
                break;
              switch (Name[16]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(661, 11);	 // "convert_ulong_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(661, 11);	 // "convert_ulong_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(661, 11);	 // "convert_ulong_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(661, 11);	 // "convert_ulong_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+15, "at", 2) != 0)
                break;
              return std::make_pair(661, 11);	 // "convert_ulong_sat"
            }
            break;
          }
          break;
        }
        break;
      case 'r':	 // 1 string to match.
        if (memcmp(Name.data()+2, "eate_user_event", 15) != 0)
          break;
        return std::make_pair(2709, 1);	 // "create_user_event"
      }
      break;
    case 'g':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "et_", 3) != 0)
        break;
      switch (Name[4]) {
      default: break;
      case 'd':	 // 1 string to match.
        if (memcmp(Name.data()+5, "efault_queue", 12) != 0)
          break;
        return std::make_pair(2713, 1);	 // "get_default_queue"
      case 'g':	 // 1 string to match.
        if (memcmp(Name.data()+5, "lobal_offset", 12) != 0)
          break;
        return std::make_pair(728, 1);	 // "get_global_offset"
      }
      break;
    case 's':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "ub_group_", 9) != 0)
        break;
      switch (Name[10]) {
      default: break;
      case 'b':	 // 1 string to match.
        if (memcmp(Name.data()+11, "arrier", 6) != 0)
          break;
        return std::make_pair(2755, 2);	 // "sub_group_barrier"
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+11, "huffle", 6) != 0)
          break;
        return std::make_pair(2777, 1);	 // "sub_group_shuffle"
      }
      break;
    case 'v':	 // 20 strings to match.
      if (memcmp(Name.data()+1, "store", 5) != 0)
        break;
      switch (Name[6]) {
      default: break;
      case '_':	 // 4 strings to match.
        if (memcmp(Name.data()+7, "half16_rt", 9) != 0)
          break;
        switch (Name[16]) {
        default: break;
        case 'e':	 // 1 string to match.
          return std::make_pair(1436, 8);	 // "vstore_half16_rte"
        case 'n':	 // 1 string to match.
          return std::make_pair(1436, 8);	 // "vstore_half16_rtn"
        case 'p':	 // 1 string to match.
          return std::make_pair(1436, 8);	 // "vstore_half16_rtp"
        case 'z':	 // 1 string to match.
          return std::make_pair(1436, 8);	 // "vstore_half16_rtz"
        }
        break;
      case 'a':	 // 16 strings to match.
        if (memcmp(Name.data()+7, "_half", 5) != 0)
          break;
        switch (Name[12]) {
        default: break;
        case '2':	 // 4 strings to match.
          if (memcmp(Name.data()+13, "_rt", 3) != 0)
            break;
          switch (Name[16]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(1384, 8);	 // "vstorea_half2_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(1384, 8);	 // "vstorea_half2_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(1384, 8);	 // "vstorea_half2_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(1384, 8);	 // "vstorea_half2_rtz"
          }
          break;
        case '3':	 // 4 strings to match.
          if (memcmp(Name.data()+13, "_rt", 3) != 0)
            break;
          switch (Name[16]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(1412, 8);	 // "vstorea_half3_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(1412, 8);	 // "vstorea_half3_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(1412, 8);	 // "vstorea_half3_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(1412, 8);	 // "vstorea_half3_rtz"
          }
          break;
        case '4':	 // 4 strings to match.
          if (memcmp(Name.data()+13, "_rt", 3) != 0)
            break;
          switch (Name[16]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(1420, 8);	 // "vstorea_half4_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(1420, 8);	 // "vstorea_half4_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(1420, 8);	 // "vstorea_half4_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(1420, 8);	 // "vstorea_half4_rtz"
          }
          break;
        case '8':	 // 4 strings to match.
          if (memcmp(Name.data()+13, "_rt", 3) != 0)
            break;
          switch (Name[16]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(1428, 8);	 // "vstorea_half8_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(1428, 8);	 // "vstorea_half8_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(1428, 8);	 // "vstorea_half8_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(1428, 8);	 // "vstorea_half8_rtz"
          }
          break;
        }
        break;
      }
      break;
    case 'w':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ait_group_events", 16) != 0)
        break;
      return std::make_pair(1455, 2);	 // "wait_group_events"
    }
    break;
  case 18:	 // 111 strings to match.
    switch (Name[0]) {
    default: break;
    case 'c':	 // 104 strings to match.
      if (memcmp(Name.data()+1, "onvert_", 7) != 0)
        break;
      switch (Name[8]) {
      default: break;
      case 'c':	 // 5 strings to match.
        if (memcmp(Name.data()+9, "har16_", 6) != 0)
          break;
        switch (Name[15]) {
        default: break;
        case 'r':	 // 4 strings to match.
          if (Name[16] != 't')
            break;
          switch (Name[17]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(254, 11);	 // "convert_char16_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(254, 11);	 // "convert_char16_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(254, 11);	 // "convert_char16_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(254, 11);	 // "convert_char16_rtz"
          }
          break;
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+16, "at", 2) != 0)
            break;
          return std::make_pair(254, 11);	 // "convert_char16_sat"
        }
        break;
      case 'd':	 // 4 strings to match.
        if (memcmp(Name.data()+9, "ouble_rt", 8) != 0)
          break;
        switch (Name[17]) {
        default: break;
        case 'e':	 // 1 string to match.
          return std::make_pair(67, 11);	 // "convert_double_rte"
        case 'n':	 // 1 string to match.
          return std::make_pair(67, 11);	 // "convert_double_rtn"
        case 'p':	 // 1 string to match.
          return std::make_pair(67, 11);	 // "convert_double_rtp"
        case 'z':	 // 1 string to match.
          return std::make_pair(67, 11);	 // "convert_double_rtz"
        }
        break;
      case 'f':	 // 16 strings to match.
        if (memcmp(Name.data()+9, "loat", 4) != 0)
          break;
        switch (Name[13]) {
        default: break;
        case '2':	 // 4 strings to match.
          if (memcmp(Name.data()+14, "_rt", 3) != 0)
            break;
          switch (Name[17]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(12, 11);	 // "convert_float2_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(12, 11);	 // "convert_float2_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(12, 11);	 // "convert_float2_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(12, 11);	 // "convert_float2_rtz"
          }
          break;
        case '3':	 // 4 strings to match.
          if (memcmp(Name.data()+14, "_rt", 3) != 0)
            break;
          switch (Name[17]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(23, 11);	 // "convert_float3_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(23, 11);	 // "convert_float3_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(23, 11);	 // "convert_float3_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(23, 11);	 // "convert_float3_rtz"
          }
          break;
        case '4':	 // 4 strings to match.
          if (memcmp(Name.data()+14, "_rt", 3) != 0)
            break;
          switch (Name[17]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(34, 11);	 // "convert_float4_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(34, 11);	 // "convert_float4_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(34, 11);	 // "convert_float4_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(34, 11);	 // "convert_float4_rtz"
          }
          break;
        case '8':	 // 4 strings to match.
          if (memcmp(Name.data()+14, "_rt", 3) != 0)
            break;
          switch (Name[17]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(45, 11);	 // "convert_float8_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(45, 11);	 // "convert_float8_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(45, 11);	 // "convert_float8_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(45, 11);	 // "convert_float8_rtz"
          }
          break;
        }
        break;
      case 'h':	 // 4 strings to match.
        if (memcmp(Name.data()+9, "alf16_rt", 8) != 0)
          break;
        switch (Name[17]) {
        default: break;
        case 'e':	 // 1 string to match.
          return std::make_pair(188, 11);	 // "convert_half16_rte"
        case 'n':	 // 1 string to match.
          return std::make_pair(188, 11);	 // "convert_half16_rtn"
        case 'p':	 // 1 string to match.
          return std::make_pair(188, 11);	 // "convert_half16_rtp"
        case 'z':	 // 1 string to match.
          return std::make_pair(188, 11);	 // "convert_half16_rtz"
        }
        break;
      case 'l':	 // 5 strings to match.
        if (memcmp(Name.data()+9, "ong16_", 6) != 0)
          break;
        switch (Name[15]) {
        default: break;
        case 'r':	 // 4 strings to match.
          if (Name[16] != 't')
            break;
          switch (Name[17]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(650, 11);	 // "convert_long16_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(650, 11);	 // "convert_long16_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(650, 11);	 // "convert_long16_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(650, 11);	 // "convert_long16_rtz"
          }
          break;
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+16, "at", 2) != 0)
            break;
          return std::make_pair(650, 11);	 // "convert_long16_sat"
        }
        break;
      case 's':	 // 20 strings to match.
        if (memcmp(Name.data()+9, "hort", 4) != 0)
          break;
        switch (Name[13]) {
        default: break;
        case '2':	 // 5 strings to match.
          if (Name[14] != '_')
            break;
          switch (Name[15]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[16] != 't')
              break;
            switch (Name[17]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(342, 11);	 // "convert_short2_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(342, 11);	 // "convert_short2_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(342, 11);	 // "convert_short2_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(342, 11);	 // "convert_short2_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+16, "at", 2) != 0)
              break;
            return std::make_pair(342, 11);	 // "convert_short2_sat"
          }
          break;
        case '3':	 // 5 strings to match.
          if (Name[14] != '_')
            break;
          switch (Name[15]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[16] != 't')
              break;
            switch (Name[17]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(353, 11);	 // "convert_short3_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(353, 11);	 // "convert_short3_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(353, 11);	 // "convert_short3_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(353, 11);	 // "convert_short3_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+16, "at", 2) != 0)
              break;
            return std::make_pair(353, 11);	 // "convert_short3_sat"
          }
          break;
        case '4':	 // 5 strings to match.
          if (Name[14] != '_')
            break;
          switch (Name[15]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[16] != 't')
              break;
            switch (Name[17]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(364, 11);	 // "convert_short4_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(364, 11);	 // "convert_short4_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(364, 11);	 // "convert_short4_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(364, 11);	 // "convert_short4_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+16, "at", 2) != 0)
              break;
            return std::make_pair(364, 11);	 // "convert_short4_sat"
          }
          break;
        case '8':	 // 5 strings to match.
          if (Name[14] != '_')
            break;
          switch (Name[15]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[16] != 't')
              break;
            switch (Name[17]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(375, 11);	 // "convert_short8_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(375, 11);	 // "convert_short8_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(375, 11);	 // "convert_short8_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(375, 11);	 // "convert_short8_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+16, "at", 2) != 0)
              break;
            return std::make_pair(375, 11);	 // "convert_short8_sat"
          }
          break;
        }
        break;
      case 'u':	 // 50 strings to match.
        switch (Name[9]) {
        default: break;
        case 'c':	 // 20 strings to match.
          if (memcmp(Name.data()+10, "har", 3) != 0)
            break;
          switch (Name[13]) {
          default: break;
          case '2':	 // 5 strings to match.
            if (Name[14] != '_')
              break;
            switch (Name[15]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[16] != 't')
                break;
              switch (Name[17]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(276, 11);	 // "convert_uchar2_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(276, 11);	 // "convert_uchar2_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(276, 11);	 // "convert_uchar2_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(276, 11);	 // "convert_uchar2_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+16, "at", 2) != 0)
                break;
              return std::make_pair(276, 11);	 // "convert_uchar2_sat"
            }
            break;
          case '3':	 // 5 strings to match.
            if (Name[14] != '_')
              break;
            switch (Name[15]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[16] != 't')
                break;
              switch (Name[17]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(287, 11);	 // "convert_uchar3_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(287, 11);	 // "convert_uchar3_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(287, 11);	 // "convert_uchar3_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(287, 11);	 // "convert_uchar3_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+16, "at", 2) != 0)
                break;
              return std::make_pair(287, 11);	 // "convert_uchar3_sat"
            }
            break;
          case '4':	 // 5 strings to match.
            if (Name[14] != '_')
              break;
            switch (Name[15]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[16] != 't')
                break;
              switch (Name[17]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(298, 11);	 // "convert_uchar4_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(298, 11);	 // "convert_uchar4_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(298, 11);	 // "convert_uchar4_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(298, 11);	 // "convert_uchar4_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+16, "at", 2) != 0)
                break;
              return std::make_pair(298, 11);	 // "convert_uchar4_sat"
            }
            break;
          case '8':	 // 5 strings to match.
            if (Name[14] != '_')
              break;
            switch (Name[15]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[16] != 't')
                break;
              switch (Name[17]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(309, 11);	 // "convert_uchar8_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(309, 11);	 // "convert_uchar8_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(309, 11);	 // "convert_uchar8_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(309, 11);	 // "convert_uchar8_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+16, "at", 2) != 0)
                break;
              return std::make_pair(309, 11);	 // "convert_uchar8_sat"
            }
            break;
          }
          break;
        case 'i':	 // 5 strings to match.
          if (memcmp(Name.data()+10, "nt16_", 5) != 0)
            break;
          switch (Name[15]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[16] != 't')
              break;
            switch (Name[17]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(584, 11);	 // "convert_uint16_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(584, 11);	 // "convert_uint16_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(584, 11);	 // "convert_uint16_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(584, 11);	 // "convert_uint16_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+16, "at", 2) != 0)
              break;
            return std::make_pair(584, 11);	 // "convert_uint16_sat"
          }
          break;
        case 'l':	 // 20 strings to match.
          if (memcmp(Name.data()+10, "ong", 3) != 0)
            break;
          switch (Name[13]) {
          default: break;
          case '2':	 // 5 strings to match.
            if (Name[14] != '_')
              break;
            switch (Name[15]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[16] != 't')
                break;
              switch (Name[17]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(672, 11);	 // "convert_ulong2_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(672, 11);	 // "convert_ulong2_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(672, 11);	 // "convert_ulong2_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(672, 11);	 // "convert_ulong2_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+16, "at", 2) != 0)
                break;
              return std::make_pair(672, 11);	 // "convert_ulong2_sat"
            }
            break;
          case '3':	 // 5 strings to match.
            if (Name[14] != '_')
              break;
            switch (Name[15]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[16] != 't')
                break;
              switch (Name[17]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(683, 11);	 // "convert_ulong3_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(683, 11);	 // "convert_ulong3_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(683, 11);	 // "convert_ulong3_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(683, 11);	 // "convert_ulong3_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+16, "at", 2) != 0)
                break;
              return std::make_pair(683, 11);	 // "convert_ulong3_sat"
            }
            break;
          case '4':	 // 5 strings to match.
            if (Name[14] != '_')
              break;
            switch (Name[15]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[16] != 't')
                break;
              switch (Name[17]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(694, 11);	 // "convert_ulong4_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(694, 11);	 // "convert_ulong4_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(694, 11);	 // "convert_ulong4_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(694, 11);	 // "convert_ulong4_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+16, "at", 2) != 0)
                break;
              return std::make_pair(694, 11);	 // "convert_ulong4_sat"
            }
            break;
          case '8':	 // 5 strings to match.
            if (Name[14] != '_')
              break;
            switch (Name[15]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[16] != 't')
                break;
              switch (Name[17]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(705, 11);	 // "convert_ulong8_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(705, 11);	 // "convert_ulong8_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(705, 11);	 // "convert_ulong8_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(705, 11);	 // "convert_ulong8_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+16, "at", 2) != 0)
                break;
              return std::make_pair(705, 11);	 // "convert_ulong8_sat"
            }
            break;
          }
          break;
        case 's':	 // 5 strings to match.
          if (memcmp(Name.data()+10, "hort_", 5) != 0)
            break;
          switch (Name[15]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[16] != 't')
              break;
            switch (Name[17]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(397, 11);	 // "convert_ushort_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(397, 11);	 // "convert_ushort_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(397, 11);	 // "convert_ushort_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(397, 11);	 // "convert_ushort_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+16, "at", 2) != 0)
              break;
            return std::make_pair(397, 11);	 // "convert_ushort_sat"
          }
          break;
        }
        break;
      }
      break;
    case 'g':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "et_", 3) != 0)
        break;
      switch (Name[4]) {
      default: break;
      case 'n':	 // 1 string to match.
        if (memcmp(Name.data()+5, "um_sub_groups", 13) != 0)
          break;
        return std::make_pair(2753, 1);	 // "get_num_sub_groups"
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+5, "ub_group_size", 13) != 0)
          break;
        return std::make_pair(2753, 1);	 // "get_sub_group_size"
      }
      break;
    case 'v':	 // 4 strings to match.
      if (memcmp(Name.data()+1, "storea_half16_rt", 16) != 0)
        break;
      switch (Name[17]) {
      default: break;
      case 'e':	 // 1 string to match.
        return std::make_pair(1436, 8);	 // "vstorea_half16_rte"
      case 'n':	 // 1 string to match.
        return std::make_pair(1436, 8);	 // "vstorea_half16_rtn"
      case 'p':	 // 1 string to match.
        return std::make_pair(1436, 8);	 // "vstorea_half16_rtp"
      case 'z':	 // 1 string to match.
        return std::make_pair(1436, 8);	 // "vstorea_half16_rtz"
      }
      break;
    case 'w':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ork_group_barrier", 17) != 0)
        break;
      return std::make_pair(1445, 2);	 // "work_group_barrier"
    }
    break;
  case 19:	 // 62 strings to match.
    switch (Name[0]) {
    default: break;
    case 'c':	 // 59 strings to match.
      if (memcmp(Name.data()+1, "onvert_", 7) != 0)
        break;
      switch (Name[8]) {
      default: break;
      case 'd':	 // 16 strings to match.
        if (memcmp(Name.data()+9, "ouble", 5) != 0)
          break;
        switch (Name[14]) {
        default: break;
        case '2':	 // 4 strings to match.
          if (memcmp(Name.data()+15, "_rt", 3) != 0)
            break;
          switch (Name[18]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(78, 11);	 // "convert_double2_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(78, 11);	 // "convert_double2_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(78, 11);	 // "convert_double2_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(78, 11);	 // "convert_double2_rtz"
          }
          break;
        case '3':	 // 4 strings to match.
          if (memcmp(Name.data()+15, "_rt", 3) != 0)
            break;
          switch (Name[18]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(89, 11);	 // "convert_double3_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(89, 11);	 // "convert_double3_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(89, 11);	 // "convert_double3_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(89, 11);	 // "convert_double3_rtz"
          }
          break;
        case '4':	 // 4 strings to match.
          if (memcmp(Name.data()+15, "_rt", 3) != 0)
            break;
          switch (Name[18]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(100, 11);	 // "convert_double4_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(100, 11);	 // "convert_double4_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(100, 11);	 // "convert_double4_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(100, 11);	 // "convert_double4_rtz"
          }
          break;
        case '8':	 // 4 strings to match.
          if (memcmp(Name.data()+15, "_rt", 3) != 0)
            break;
          switch (Name[18]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(111, 11);	 // "convert_double8_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(111, 11);	 // "convert_double8_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(111, 11);	 // "convert_double8_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(111, 11);	 // "convert_double8_rtz"
          }
          break;
        }
        break;
      case 'f':	 // 4 strings to match.
        if (memcmp(Name.data()+9, "loat16_rt", 9) != 0)
          break;
        switch (Name[18]) {
        default: break;
        case 'e':	 // 1 string to match.
          return std::make_pair(56, 11);	 // "convert_float16_rte"
        case 'n':	 // 1 string to match.
          return std::make_pair(56, 11);	 // "convert_float16_rtn"
        case 'p':	 // 1 string to match.
          return std::make_pair(56, 11);	 // "convert_float16_rtp"
        case 'z':	 // 1 string to match.
          return std::make_pair(56, 11);	 // "convert_float16_rtz"
        }
        break;
      case 'i':	 // 4 strings to match.
        if (memcmp(Name.data()+9, "nt_sat_rt", 9) != 0)
          break;
        switch (Name[18]) {
        default: break;
        case 'e':	 // 1 string to match.
          return std::make_pair(463, 11);	 // "convert_int_sat_rte"
        case 'n':	 // 1 string to match.
          return std::make_pair(463, 11);	 // "convert_int_sat_rtn"
        case 'p':	 // 1 string to match.
          return std::make_pair(463, 11);	 // "convert_int_sat_rtp"
        case 'z':	 // 1 string to match.
          return std::make_pair(463, 11);	 // "convert_int_sat_rtz"
        }
        break;
      case 's':	 // 5 strings to match.
        if (memcmp(Name.data()+9, "hort16_", 7) != 0)
          break;
        switch (Name[16]) {
        default: break;
        case 'r':	 // 4 strings to match.
          if (Name[17] != 't')
            break;
          switch (Name[18]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(386, 11);	 // "convert_short16_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(386, 11);	 // "convert_short16_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(386, 11);	 // "convert_short16_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(386, 11);	 // "convert_short16_rtz"
          }
          break;
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+17, "at", 2) != 0)
            break;
          return std::make_pair(386, 11);	 // "convert_short16_sat"
        }
        break;
      case 'u':	 // 30 strings to match.
        switch (Name[9]) {
        default: break;
        case 'c':	 // 5 strings to match.
          if (memcmp(Name.data()+10, "har16_", 6) != 0)
            break;
          switch (Name[16]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[17] != 't')
              break;
            switch (Name[18]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(320, 11);	 // "convert_uchar16_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(320, 11);	 // "convert_uchar16_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(320, 11);	 // "convert_uchar16_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(320, 11);	 // "convert_uchar16_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+17, "at", 2) != 0)
              break;
            return std::make_pair(320, 11);	 // "convert_uchar16_sat"
          }
          break;
        case 'l':	 // 5 strings to match.
          if (memcmp(Name.data()+10, "ong16_", 6) != 0)
            break;
          switch (Name[16]) {
          default: break;
          case 'r':	 // 4 strings to match.
            if (Name[17] != 't')
              break;
            switch (Name[18]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(716, 11);	 // "convert_ulong16_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(716, 11);	 // "convert_ulong16_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(716, 11);	 // "convert_ulong16_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(716, 11);	 // "convert_ulong16_rtz"
            }
            break;
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+17, "at", 2) != 0)
              break;
            return std::make_pair(716, 11);	 // "convert_ulong16_sat"
          }
          break;
        case 's':	 // 20 strings to match.
          if (memcmp(Name.data()+10, "hort", 4) != 0)
            break;
          switch (Name[14]) {
          default: break;
          case '2':	 // 5 strings to match.
            if (Name[15] != '_')
              break;
            switch (Name[16]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[17] != 't')
                break;
              switch (Name[18]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(408, 11);	 // "convert_ushort2_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(408, 11);	 // "convert_ushort2_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(408, 11);	 // "convert_ushort2_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(408, 11);	 // "convert_ushort2_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+17, "at", 2) != 0)
                break;
              return std::make_pair(408, 11);	 // "convert_ushort2_sat"
            }
            break;
          case '3':	 // 5 strings to match.
            if (Name[15] != '_')
              break;
            switch (Name[16]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[17] != 't')
                break;
              switch (Name[18]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(419, 11);	 // "convert_ushort3_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(419, 11);	 // "convert_ushort3_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(419, 11);	 // "convert_ushort3_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(419, 11);	 // "convert_ushort3_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+17, "at", 2) != 0)
                break;
              return std::make_pair(419, 11);	 // "convert_ushort3_sat"
            }
            break;
          case '4':	 // 5 strings to match.
            if (Name[15] != '_')
              break;
            switch (Name[16]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[17] != 't')
                break;
              switch (Name[18]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(430, 11);	 // "convert_ushort4_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(430, 11);	 // "convert_ushort4_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(430, 11);	 // "convert_ushort4_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(430, 11);	 // "convert_ushort4_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+17, "at", 2) != 0)
                break;
              return std::make_pair(430, 11);	 // "convert_ushort4_sat"
            }
            break;
          case '8':	 // 5 strings to match.
            if (Name[15] != '_')
              break;
            switch (Name[16]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[17] != 't')
                break;
              switch (Name[18]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(441, 11);	 // "convert_ushort8_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(441, 11);	 // "convert_ushort8_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(441, 11);	 // "convert_ushort8_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(441, 11);	 // "convert_ushort8_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+17, "at", 2) != 0)
                break;
              return std::make_pair(441, 11);	 // "convert_ushort8_sat"
            }
            break;
          }
          break;
        }
        break;
      }
      break;
    case 'g':	 // 1 string to match.
      if (memcmp(Name.data()+1, "et_local_linear_id", 18) != 0)
        break;
      return std::make_pair(730, 1);	 // "get_local_linear_id"
    case 'i':	 // 1 string to match.
      if (memcmp(Name.data()+1, "s_valid_reserve_id", 18) != 0)
        break;
      return std::make_pair(2706, 1);	 // "is_valid_reserve_id"
    case 's':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ub_group_broadcast", 18) != 0)
        break;
      return std::make_pair(2758, 3);	 // "sub_group_broadcast"
    }
    break;
  case 20:	 // 49 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 1 string to match.
      if (memcmp(Name.data()+1, "tomic_load_explicit", 19) != 0)
        break;
      return std::make_pair(1744, 42);	 // "atomic_load_explicit"
    case 'c':	 // 38 strings to match.
      switch (Name[1]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+2, "ock_read_sub_group", 18) != 0)
          break;
        return std::make_pair(2802, 1);	 // "clock_read_sub_group"
      case 'o':	 // 37 strings to match.
        if (memcmp(Name.data()+2, "nvert_", 6) != 0)
          break;
        switch (Name[8]) {
        default: break;
        case 'c':	 // 4 strings to match.
          if (memcmp(Name.data()+9, "har_sat_rt", 10) != 0)
            break;
          switch (Name[19]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(199, 11);	 // "convert_char_sat_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(199, 11);	 // "convert_char_sat_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(199, 11);	 // "convert_char_sat_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(199, 11);	 // "convert_char_sat_rtz"
          }
          break;
        case 'd':	 // 4 strings to match.
          if (memcmp(Name.data()+9, "ouble16_rt", 10) != 0)
            break;
          switch (Name[19]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(122, 11);	 // "convert_double16_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(122, 11);	 // "convert_double16_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(122, 11);	 // "convert_double16_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(122, 11);	 // "convert_double16_rtz"
          }
          break;
        case 'i':	 // 16 strings to match.
          if (memcmp(Name.data()+9, "nt", 2) != 0)
            break;
          switch (Name[11]) {
          default: break;
          case '2':	 // 4 strings to match.
            if (memcmp(Name.data()+12, "_sat_rt", 7) != 0)
              break;
            switch (Name[19]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(474, 11);	 // "convert_int2_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(474, 11);	 // "convert_int2_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(474, 11);	 // "convert_int2_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(474, 11);	 // "convert_int2_sat_rtz"
            }
            break;
          case '3':	 // 4 strings to match.
            if (memcmp(Name.data()+12, "_sat_rt", 7) != 0)
              break;
            switch (Name[19]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(485, 11);	 // "convert_int3_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(485, 11);	 // "convert_int3_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(485, 11);	 // "convert_int3_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(485, 11);	 // "convert_int3_sat_rtz"
            }
            break;
          case '4':	 // 4 strings to match.
            if (memcmp(Name.data()+12, "_sat_rt", 7) != 0)
              break;
            switch (Name[19]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(496, 11);	 // "convert_int4_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(496, 11);	 // "convert_int4_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(496, 11);	 // "convert_int4_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(496, 11);	 // "convert_int4_sat_rtz"
            }
            break;
          case '8':	 // 4 strings to match.
            if (memcmp(Name.data()+12, "_sat_rt", 7) != 0)
              break;
            switch (Name[19]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(507, 11);	 // "convert_int8_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(507, 11);	 // "convert_int8_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(507, 11);	 // "convert_int8_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(507, 11);	 // "convert_int8_sat_rtz"
            }
            break;
          }
          break;
        case 'l':	 // 4 strings to match.
          if (memcmp(Name.data()+9, "ong_sat_rt", 10) != 0)
            break;
          switch (Name[19]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(595, 11);	 // "convert_long_sat_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(595, 11);	 // "convert_long_sat_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(595, 11);	 // "convert_long_sat_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(595, 11);	 // "convert_long_sat_rtz"
          }
          break;
        case 'u':	 // 9 strings to match.
          switch (Name[9]) {
          default: break;
          case 'i':	 // 4 strings to match.
            if (memcmp(Name.data()+10, "nt_sat_rt", 9) != 0)
              break;
            switch (Name[19]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(529, 11);	 // "convert_uint_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(529, 11);	 // "convert_uint_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(529, 11);	 // "convert_uint_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(529, 11);	 // "convert_uint_sat_rtz"
            }
            break;
          case 's':	 // 5 strings to match.
            if (memcmp(Name.data()+10, "hort16_", 7) != 0)
              break;
            switch (Name[17]) {
            default: break;
            case 'r':	 // 4 strings to match.
              if (Name[18] != 't')
                break;
              switch (Name[19]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(452, 11);	 // "convert_ushort16_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(452, 11);	 // "convert_ushort16_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(452, 11);	 // "convert_ushort16_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(452, 11);	 // "convert_ushort16_rtz"
              }
              break;
            case 's':	 // 1 string to match.
              if (memcmp(Name.data()+18, "at", 2) != 0)
                break;
              return std::make_pair(452, 11);	 // "convert_ushort16_sat"
            }
            break;
          }
          break;
        }
        break;
      }
      break;
    case 'd':	 // 3 strings to match.
      if (memcmp(Name.data()+1, "ot_4x8packed_", 13) != 0)
        break;
      switch (Name[14]) {
      default: break;
      case 's':	 // 2 strings to match.
        switch (Name[15]) {
        default: break;
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+16, "_int", 4) != 0)
            break;
          return std::make_pair(2793, 1);	 // "dot_4x8packed_ss_int"
        case 'u':	 // 1 string to match.
          if (memcmp(Name.data()+16, "_int", 4) != 0)
            break;
          return std::make_pair(2793, 1);	 // "dot_4x8packed_su_int"
        }
        break;
      case 'u':	 // 1 string to match.
        if (memcmp(Name.data()+15, "s_int", 5) != 0)
          break;
        return std::make_pair(2793, 1);	 // "dot_4x8packed_us_int"
      }
      break;
    case 'g':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "et_", 3) != 0)
        break;
      switch (Name[4]) {
      default: break;
      case 'g':	 // 1 string to match.
        if (memcmp(Name.data()+5, "lobal_linear_id", 15) != 0)
          break;
        return std::make_pair(730, 1);	 // "get_global_linear_id"
      case 'i':	 // 1 string to match.
        if (memcmp(Name.data()+5, "mage_array_size", 15) != 0)
          break;
        return std::make_pair(2652, 15);	 // "get_image_array_size"
      }
      break;
    case 's':	 // 4 strings to match.
      if (memcmp(Name.data()+1, "ub_group_", 9) != 0)
        break;
      switch (Name[10]) {
      default: break;
      case 'r':	 // 3 strings to match.
        if (memcmp(Name.data()+11, "educe_", 6) != 0)
          break;
        switch (Name[17]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (memcmp(Name.data()+18, "dd", 2) != 0)
            break;
          return std::make_pair(2761, 2);	 // "sub_group_reduce_add"
        case 'm':	 // 2 strings to match.
          switch (Name[18]) {
          default: break;
          case 'a':	 // 1 string to match.
            if (Name[19] != 'x')
              break;
            return std::make_pair(2761, 2);	 // "sub_group_reduce_max"
          case 'i':	 // 1 string to match.
            if (Name[19] != 'n')
              break;
            return std::make_pair(2761, 2);	 // "sub_group_reduce_min"
          }
          break;
        }
        break;
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+11, "huffle_up", 9) != 0)
          break;
        return std::make_pair(2778, 1);	 // "sub_group_shuffle_up"
      }
      break;
    case 'w':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ork_group_broadcast", 19) != 0)
        break;
      return std::make_pair(2702, 3);	 // "work_group_broadcast"
    }
    break;
  case 21:	 // 79 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+2, "ync_work_group_copy", 19) != 0)
          break;
        return std::make_pair(1450, 2);	 // "async_work_group_copy"
      case 't':	 // 1 string to match.
        if (memcmp(Name.data()+2, "omic_store_explicit", 19) != 0)
          break;
        return std::make_pair(1536, 42);	 // "atomic_store_explicit"
      }
      break;
    case 'c':	 // 65 strings to match.
      switch (Name[1]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+2, "ock_read_work_group", 19) != 0)
          break;
        return std::make_pair(2800, 1);	 // "clock_read_work_group"
      case 'o':	 // 64 strings to match.
        if (memcmp(Name.data()+2, "nvert_", 6) != 0)
          break;
        switch (Name[8]) {
        default: break;
        case 'c':	 // 16 strings to match.
          if (memcmp(Name.data()+9, "har", 3) != 0)
            break;
          switch (Name[12]) {
          default: break;
          case '2':	 // 4 strings to match.
            if (memcmp(Name.data()+13, "_sat_rt", 7) != 0)
              break;
            switch (Name[20]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(210, 11);	 // "convert_char2_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(210, 11);	 // "convert_char2_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(210, 11);	 // "convert_char2_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(210, 11);	 // "convert_char2_sat_rtz"
            }
            break;
          case '3':	 // 4 strings to match.
            if (memcmp(Name.data()+13, "_sat_rt", 7) != 0)
              break;
            switch (Name[20]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(221, 11);	 // "convert_char3_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(221, 11);	 // "convert_char3_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(221, 11);	 // "convert_char3_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(221, 11);	 // "convert_char3_sat_rtz"
            }
            break;
          case '4':	 // 4 strings to match.
            if (memcmp(Name.data()+13, "_sat_rt", 7) != 0)
              break;
            switch (Name[20]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(232, 11);	 // "convert_char4_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(232, 11);	 // "convert_char4_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(232, 11);	 // "convert_char4_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(232, 11);	 // "convert_char4_sat_rtz"
            }
            break;
          case '8':	 // 4 strings to match.
            if (memcmp(Name.data()+13, "_sat_rt", 7) != 0)
              break;
            switch (Name[20]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(243, 11);	 // "convert_char8_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(243, 11);	 // "convert_char8_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(243, 11);	 // "convert_char8_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(243, 11);	 // "convert_char8_sat_rtz"
            }
            break;
          }
          break;
        case 'i':	 // 4 strings to match.
          if (memcmp(Name.data()+9, "nt16_sat_rt", 11) != 0)
            break;
          switch (Name[20]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(518, 11);	 // "convert_int16_sat_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(518, 11);	 // "convert_int16_sat_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(518, 11);	 // "convert_int16_sat_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(518, 11);	 // "convert_int16_sat_rtz"
          }
          break;
        case 'l':	 // 16 strings to match.
          if (memcmp(Name.data()+9, "ong", 3) != 0)
            break;
          switch (Name[12]) {
          default: break;
          case '2':	 // 4 strings to match.
            if (memcmp(Name.data()+13, "_sat_rt", 7) != 0)
              break;
            switch (Name[20]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(606, 11);	 // "convert_long2_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(606, 11);	 // "convert_long2_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(606, 11);	 // "convert_long2_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(606, 11);	 // "convert_long2_sat_rtz"
            }
            break;
          case '3':	 // 4 strings to match.
            if (memcmp(Name.data()+13, "_sat_rt", 7) != 0)
              break;
            switch (Name[20]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(617, 11);	 // "convert_long3_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(617, 11);	 // "convert_long3_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(617, 11);	 // "convert_long3_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(617, 11);	 // "convert_long3_sat_rtz"
            }
            break;
          case '4':	 // 4 strings to match.
            if (memcmp(Name.data()+13, "_sat_rt", 7) != 0)
              break;
            switch (Name[20]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(628, 11);	 // "convert_long4_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(628, 11);	 // "convert_long4_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(628, 11);	 // "convert_long4_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(628, 11);	 // "convert_long4_sat_rtz"
            }
            break;
          case '8':	 // 4 strings to match.
            if (memcmp(Name.data()+13, "_sat_rt", 7) != 0)
              break;
            switch (Name[20]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(639, 11);	 // "convert_long8_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(639, 11);	 // "convert_long8_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(639, 11);	 // "convert_long8_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(639, 11);	 // "convert_long8_sat_rtz"
            }
            break;
          }
          break;
        case 's':	 // 4 strings to match.
          if (memcmp(Name.data()+9, "hort_sat_rt", 11) != 0)
            break;
          switch (Name[20]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(331, 11);	 // "convert_short_sat_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(331, 11);	 // "convert_short_sat_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(331, 11);	 // "convert_short_sat_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(331, 11);	 // "convert_short_sat_rtz"
          }
          break;
        case 'u':	 // 24 strings to match.
          switch (Name[9]) {
          default: break;
          case 'c':	 // 4 strings to match.
            if (memcmp(Name.data()+10, "har_sat_rt", 10) != 0)
              break;
            switch (Name[20]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(265, 11);	 // "convert_uchar_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(265, 11);	 // "convert_uchar_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(265, 11);	 // "convert_uchar_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(265, 11);	 // "convert_uchar_sat_rtz"
            }
            break;
          case 'i':	 // 16 strings to match.
            if (memcmp(Name.data()+10, "nt", 2) != 0)
              break;
            switch (Name[12]) {
            default: break;
            case '2':	 // 4 strings to match.
              if (memcmp(Name.data()+13, "_sat_rt", 7) != 0)
                break;
              switch (Name[20]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(540, 11);	 // "convert_uint2_sat_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(540, 11);	 // "convert_uint2_sat_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(540, 11);	 // "convert_uint2_sat_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(540, 11);	 // "convert_uint2_sat_rtz"
              }
              break;
            case '3':	 // 4 strings to match.
              if (memcmp(Name.data()+13, "_sat_rt", 7) != 0)
                break;
              switch (Name[20]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(551, 11);	 // "convert_uint3_sat_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(551, 11);	 // "convert_uint3_sat_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(551, 11);	 // "convert_uint3_sat_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(551, 11);	 // "convert_uint3_sat_rtz"
              }
              break;
            case '4':	 // 4 strings to match.
              if (memcmp(Name.data()+13, "_sat_rt", 7) != 0)
                break;
              switch (Name[20]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(562, 11);	 // "convert_uint4_sat_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(562, 11);	 // "convert_uint4_sat_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(562, 11);	 // "convert_uint4_sat_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(562, 11);	 // "convert_uint4_sat_rtz"
              }
              break;
            case '8':	 // 4 strings to match.
              if (memcmp(Name.data()+13, "_sat_rt", 7) != 0)
                break;
              switch (Name[20]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(573, 11);	 // "convert_uint8_sat_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(573, 11);	 // "convert_uint8_sat_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(573, 11);	 // "convert_uint8_sat_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(573, 11);	 // "convert_uint8_sat_rtz"
              }
              break;
            }
            break;
          case 'l':	 // 4 strings to match.
            if (memcmp(Name.data()+10, "ong_sat_rt", 10) != 0)
              break;
            switch (Name[20]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(661, 11);	 // "convert_ulong_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(661, 11);	 // "convert_ulong_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(661, 11);	 // "convert_ulong_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(661, 11);	 // "convert_ulong_sat_rtz"
            }
            break;
          }
          break;
        }
        break;
      }
      break;
    case 'd':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ot_4x8packed_uu_uint", 20) != 0)
        break;
      return std::make_pair(2792, 1);	 // "dot_4x8packed_uu_uint"
    case 'g':	 // 6 strings to match.
      if (memcmp(Name.data()+1, "et_", 3) != 0)
        break;
      switch (Name[4]) {
      default: break;
      case 'i':	 // 1 string to match.
        if (memcmp(Name.data()+5, "mage_num_samples", 16) != 0)
          break;
        return std::make_pair(2741, 12);	 // "get_image_num_samples"
      case 's':	 // 5 strings to match.
        if (memcmp(Name.data()+5, "ub_group_", 9) != 0)
          break;
        switch (Name[14]) {
        default: break;
        case 'e':	 // 1 string to match.
          if (memcmp(Name.data()+15, "q_mask", 6) != 0)
            break;
          return std::make_pair(2773, 1);	 // "get_sub_group_eq_mask"
        case 'g':	 // 2 strings to match.
          switch (Name[15]) {
          default: break;
          case 'e':	 // 1 string to match.
            if (memcmp(Name.data()+16, "_mask", 5) != 0)
              break;
            return std::make_pair(2773, 1);	 // "get_sub_group_ge_mask"
          case 't':	 // 1 string to match.
            if (memcmp(Name.data()+16, "_mask", 5) != 0)
              break;
            return std::make_pair(2773, 1);	 // "get_sub_group_gt_mask"
          }
          break;
        case 'l':	 // 2 strings to match.
          switch (Name[15]) {
          default: break;
          case 'e':	 // 1 string to match.
            if (memcmp(Name.data()+16, "_mask", 5) != 0)
              break;
            return std::make_pair(2773, 1);	 // "get_sub_group_le_mask"
          case 't':	 // 1 string to match.
            if (memcmp(Name.data()+16, "_mask", 5) != 0)
              break;
            return std::make_pair(2773, 1);	 // "get_sub_group_lt_mask"
          }
          break;
        }
        break;
      }
      break;
    case 's':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+2, "t_user_event_status", 19) != 0)
          break;
        return std::make_pair(2711, 1);	 // "set_user_event_status"
      case 'u':	 // 1 string to match.
        if (memcmp(Name.data()+2, "b_group_shuffle_xor", 19) != 0)
          break;
        return std::make_pair(2777, 1);	 // "sub_group_shuffle_xor"
      }
      break;
    case 'w':	 // 3 strings to match.
      if (memcmp(Name.data()+1, "ork_group_reduce_", 17) != 0)
        break;
      switch (Name[18]) {
      default: break;
      case 'a':	 // 1 string to match.
        if (memcmp(Name.data()+19, "dd", 2) != 0)
          break;
        return std::make_pair(2705, 1);	 // "work_group_reduce_add"
      case 'm':	 // 2 strings to match.
        switch (Name[19]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (Name[20] != 'x')
            break;
          return std::make_pair(2705, 1);	 // "work_group_reduce_max"
        case 'i':	 // 1 string to match.
          if (Name[20] != 'n')
            break;
          return std::make_pair(2705, 1);	 // "work_group_reduce_min"
        }
        break;
      }
      break;
    }
    break;
  case 22:	 // 69 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 1 string to match.
      if (memcmp(Name.data()+1, "tomic_work_item_fence", 21) != 0)
        break;
      return std::make_pair(1722, 1);	 // "atomic_work_item_fence"
    case 'c':	 // 65 strings to match.
      switch (Name[1]) {
      default: break;
      case 'l':	 // 1 string to match.
        if (memcmp(Name.data()+2, "ock_read_hilo_device", 20) != 0)
          break;
        return std::make_pair(2799, 1);	 // "clock_read_hilo_device"
      case 'o':	 // 64 strings to match.
        if (memcmp(Name.data()+2, "nvert_", 6) != 0)
          break;
        switch (Name[8]) {
        default: break;
        case 'c':	 // 4 strings to match.
          if (memcmp(Name.data()+9, "har16_sat_rt", 12) != 0)
            break;
          switch (Name[21]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(254, 11);	 // "convert_char16_sat_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(254, 11);	 // "convert_char16_sat_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(254, 11);	 // "convert_char16_sat_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(254, 11);	 // "convert_char16_sat_rtz"
          }
          break;
        case 'l':	 // 4 strings to match.
          if (memcmp(Name.data()+9, "ong16_sat_rt", 12) != 0)
            break;
          switch (Name[21]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(650, 11);	 // "convert_long16_sat_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(650, 11);	 // "convert_long16_sat_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(650, 11);	 // "convert_long16_sat_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(650, 11);	 // "convert_long16_sat_rtz"
          }
          break;
        case 's':	 // 16 strings to match.
          if (memcmp(Name.data()+9, "hort", 4) != 0)
            break;
          switch (Name[13]) {
          default: break;
          case '2':	 // 4 strings to match.
            if (memcmp(Name.data()+14, "_sat_rt", 7) != 0)
              break;
            switch (Name[21]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(342, 11);	 // "convert_short2_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(342, 11);	 // "convert_short2_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(342, 11);	 // "convert_short2_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(342, 11);	 // "convert_short2_sat_rtz"
            }
            break;
          case '3':	 // 4 strings to match.
            if (memcmp(Name.data()+14, "_sat_rt", 7) != 0)
              break;
            switch (Name[21]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(353, 11);	 // "convert_short3_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(353, 11);	 // "convert_short3_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(353, 11);	 // "convert_short3_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(353, 11);	 // "convert_short3_sat_rtz"
            }
            break;
          case '4':	 // 4 strings to match.
            if (memcmp(Name.data()+14, "_sat_rt", 7) != 0)
              break;
            switch (Name[21]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(364, 11);	 // "convert_short4_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(364, 11);	 // "convert_short4_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(364, 11);	 // "convert_short4_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(364, 11);	 // "convert_short4_sat_rtz"
            }
            break;
          case '8':	 // 4 strings to match.
            if (memcmp(Name.data()+14, "_sat_rt", 7) != 0)
              break;
            switch (Name[21]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(375, 11);	 // "convert_short8_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(375, 11);	 // "convert_short8_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(375, 11);	 // "convert_short8_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(375, 11);	 // "convert_short8_sat_rtz"
            }
            break;
          }
          break;
        case 'u':	 // 40 strings to match.
          switch (Name[9]) {
          default: break;
          case 'c':	 // 16 strings to match.
            if (memcmp(Name.data()+10, "har", 3) != 0)
              break;
            switch (Name[13]) {
            default: break;
            case '2':	 // 4 strings to match.
              if (memcmp(Name.data()+14, "_sat_rt", 7) != 0)
                break;
              switch (Name[21]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(276, 11);	 // "convert_uchar2_sat_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(276, 11);	 // "convert_uchar2_sat_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(276, 11);	 // "convert_uchar2_sat_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(276, 11);	 // "convert_uchar2_sat_rtz"
              }
              break;
            case '3':	 // 4 strings to match.
              if (memcmp(Name.data()+14, "_sat_rt", 7) != 0)
                break;
              switch (Name[21]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(287, 11);	 // "convert_uchar3_sat_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(287, 11);	 // "convert_uchar3_sat_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(287, 11);	 // "convert_uchar3_sat_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(287, 11);	 // "convert_uchar3_sat_rtz"
              }
              break;
            case '4':	 // 4 strings to match.
              if (memcmp(Name.data()+14, "_sat_rt", 7) != 0)
                break;
              switch (Name[21]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(298, 11);	 // "convert_uchar4_sat_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(298, 11);	 // "convert_uchar4_sat_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(298, 11);	 // "convert_uchar4_sat_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(298, 11);	 // "convert_uchar4_sat_rtz"
              }
              break;
            case '8':	 // 4 strings to match.
              if (memcmp(Name.data()+14, "_sat_rt", 7) != 0)
                break;
              switch (Name[21]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(309, 11);	 // "convert_uchar8_sat_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(309, 11);	 // "convert_uchar8_sat_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(309, 11);	 // "convert_uchar8_sat_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(309, 11);	 // "convert_uchar8_sat_rtz"
              }
              break;
            }
            break;
          case 'i':	 // 4 strings to match.
            if (memcmp(Name.data()+10, "nt16_sat_rt", 11) != 0)
              break;
            switch (Name[21]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(584, 11);	 // "convert_uint16_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(584, 11);	 // "convert_uint16_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(584, 11);	 // "convert_uint16_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(584, 11);	 // "convert_uint16_sat_rtz"
            }
            break;
          case 'l':	 // 16 strings to match.
            if (memcmp(Name.data()+10, "ong", 3) != 0)
              break;
            switch (Name[13]) {
            default: break;
            case '2':	 // 4 strings to match.
              if (memcmp(Name.data()+14, "_sat_rt", 7) != 0)
                break;
              switch (Name[21]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(672, 11);	 // "convert_ulong2_sat_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(672, 11);	 // "convert_ulong2_sat_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(672, 11);	 // "convert_ulong2_sat_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(672, 11);	 // "convert_ulong2_sat_rtz"
              }
              break;
            case '3':	 // 4 strings to match.
              if (memcmp(Name.data()+14, "_sat_rt", 7) != 0)
                break;
              switch (Name[21]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(683, 11);	 // "convert_ulong3_sat_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(683, 11);	 // "convert_ulong3_sat_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(683, 11);	 // "convert_ulong3_sat_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(683, 11);	 // "convert_ulong3_sat_rtz"
              }
              break;
            case '4':	 // 4 strings to match.
              if (memcmp(Name.data()+14, "_sat_rt", 7) != 0)
                break;
              switch (Name[21]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(694, 11);	 // "convert_ulong4_sat_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(694, 11);	 // "convert_ulong4_sat_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(694, 11);	 // "convert_ulong4_sat_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(694, 11);	 // "convert_ulong4_sat_rtz"
              }
              break;
            case '8':	 // 4 strings to match.
              if (memcmp(Name.data()+14, "_sat_rt", 7) != 0)
                break;
              switch (Name[21]) {
              default: break;
              case 'e':	 // 1 string to match.
                return std::make_pair(705, 11);	 // "convert_ulong8_sat_rte"
              case 'n':	 // 1 string to match.
                return std::make_pair(705, 11);	 // "convert_ulong8_sat_rtn"
              case 'p':	 // 1 string to match.
                return std::make_pair(705, 11);	 // "convert_ulong8_sat_rtp"
              case 'z':	 // 1 string to match.
                return std::make_pair(705, 11);	 // "convert_ulong8_sat_rtz"
              }
              break;
            }
            break;
          case 's':	 // 4 strings to match.
            if (memcmp(Name.data()+10, "hort_sat_rt", 11) != 0)
              break;
            switch (Name[21]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(397, 11);	 // "convert_ushort_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(397, 11);	 // "convert_ushort_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(397, 11);	 // "convert_ushort_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(397, 11);	 // "convert_ushort_sat_rtz"
            }
            break;
          }
          break;
        }
        break;
      }
      break;
    case 'g':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "et_", 3) != 0)
        break;
      switch (Name[4]) {
      default: break;
      case 'm':	 // 1 string to match.
        if (memcmp(Name.data()+5, "ax_sub_group_size", 17) != 0)
          break;
        return std::make_pair(2753, 1);	 // "get_max_sub_group_size"
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+5, "ub_group_local_id", 17) != 0)
          break;
        return std::make_pair(2753, 1);	 // "get_sub_group_local_id"
      }
      break;
    case 's':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ub_group_shuffle_down", 21) != 0)
        break;
      return std::make_pair(2778, 1);	 // "sub_group_shuffle_down"
    }
    break;
  case 23:	 // 32 strings to match.
    switch (Name[0]) {
    default: break;
    case 'b':	 // 1 string to match.
      if (memcmp(Name.data()+1, "itfield_extract_signed", 22) != 0)
        break;
      return std::make_pair(2783, 2);	 // "bitfield_extract_signed"
    case 'c':	 // 28 strings to match.
      if (memcmp(Name.data()+1, "onvert_", 7) != 0)
        break;
      switch (Name[8]) {
      default: break;
      case 's':	 // 4 strings to match.
        if (memcmp(Name.data()+9, "hort16_sat_rt", 13) != 0)
          break;
        switch (Name[22]) {
        default: break;
        case 'e':	 // 1 string to match.
          return std::make_pair(386, 11);	 // "convert_short16_sat_rte"
        case 'n':	 // 1 string to match.
          return std::make_pair(386, 11);	 // "convert_short16_sat_rtn"
        case 'p':	 // 1 string to match.
          return std::make_pair(386, 11);	 // "convert_short16_sat_rtp"
        case 'z':	 // 1 string to match.
          return std::make_pair(386, 11);	 // "convert_short16_sat_rtz"
        }
        break;
      case 'u':	 // 24 strings to match.
        switch (Name[9]) {
        default: break;
        case 'c':	 // 4 strings to match.
          if (memcmp(Name.data()+10, "har16_sat_rt", 12) != 0)
            break;
          switch (Name[22]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(320, 11);	 // "convert_uchar16_sat_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(320, 11);	 // "convert_uchar16_sat_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(320, 11);	 // "convert_uchar16_sat_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(320, 11);	 // "convert_uchar16_sat_rtz"
          }
          break;
        case 'l':	 // 4 strings to match.
          if (memcmp(Name.data()+10, "ong16_sat_rt", 12) != 0)
            break;
          switch (Name[22]) {
          default: break;
          case 'e':	 // 1 string to match.
            return std::make_pair(716, 11);	 // "convert_ulong16_sat_rte"
          case 'n':	 // 1 string to match.
            return std::make_pair(716, 11);	 // "convert_ulong16_sat_rtn"
          case 'p':	 // 1 string to match.
            return std::make_pair(716, 11);	 // "convert_ulong16_sat_rtp"
          case 'z':	 // 1 string to match.
            return std::make_pair(716, 11);	 // "convert_ulong16_sat_rtz"
          }
          break;
        case 's':	 // 16 strings to match.
          if (memcmp(Name.data()+10, "hort", 4) != 0)
            break;
          switch (Name[14]) {
          default: break;
          case '2':	 // 4 strings to match.
            if (memcmp(Name.data()+15, "_sat_rt", 7) != 0)
              break;
            switch (Name[22]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(408, 11);	 // "convert_ushort2_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(408, 11);	 // "convert_ushort2_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(408, 11);	 // "convert_ushort2_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(408, 11);	 // "convert_ushort2_sat_rtz"
            }
            break;
          case '3':	 // 4 strings to match.
            if (memcmp(Name.data()+15, "_sat_rt", 7) != 0)
              break;
            switch (Name[22]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(419, 11);	 // "convert_ushort3_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(419, 11);	 // "convert_ushort3_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(419, 11);	 // "convert_ushort3_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(419, 11);	 // "convert_ushort3_sat_rtz"
            }
            break;
          case '4':	 // 4 strings to match.
            if (memcmp(Name.data()+15, "_sat_rt", 7) != 0)
              break;
            switch (Name[22]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(430, 11);	 // "convert_ushort4_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(430, 11);	 // "convert_ushort4_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(430, 11);	 // "convert_ushort4_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(430, 11);	 // "convert_ushort4_sat_rtz"
            }
            break;
          case '8':	 // 4 strings to match.
            if (memcmp(Name.data()+15, "_sat_rt", 7) != 0)
              break;
            switch (Name[22]) {
            default: break;
            case 'e':	 // 1 string to match.
              return std::make_pair(441, 11);	 // "convert_ushort8_sat_rte"
            case 'n':	 // 1 string to match.
              return std::make_pair(441, 11);	 // "convert_ushort8_sat_rtn"
            case 'p':	 // 1 string to match.
              return std::make_pair(441, 11);	 // "convert_ushort8_sat_rtp"
            case 'z':	 // 1 string to match.
              return std::make_pair(441, 11);	 // "convert_ushort8_sat_rtz"
            }
            break;
          }
          break;
        }
        break;
      }
      break;
    case 'g':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "et_", 3) != 0)
        break;
      switch (Name[4]) {
      default: break;
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+5, "nqueued_local_size", 18) != 0)
          break;
        return std::make_pair(729, 1);	 // "get_enqueued_local_size"
      case 'i':	 // 1 string to match.
        if (memcmp(Name.data()+5, "mage_channel_order", 18) != 0)
          break;
        return std::make_pair(2559, 36);	 // "get_image_channel_order"
      }
      break;
    case 'i':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ntel_sub_group_shuffle", 22) != 0)
        break;
      return std::make_pair(2804, 11);	 // "intel_sub_group_shuffle"
    }
    break;
  case 24:	 // 9 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 3 strings to match.
      if (memcmp(Name.data()+1, "tomic_", 6) != 0)
        break;
      switch (Name[7]) {
      default: break;
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+8, "xchange_explicit", 16) != 0)
          break;
        return std::make_pair(1807, 42);	 // "atomic_exchange_explicit"
      case 'f':	 // 2 strings to match.
        switch (Name[8]) {
        default: break;
        case 'e':	 // 1 string to match.
          if (memcmp(Name.data()+9, "tch_or_explicit", 15) != 0)
            break;
          return std::make_pair(1933, 24);	 // "atomic_fetch_or_explicit"
        case 'l':	 // 1 string to match.
          if (memcmp(Name.data()+9, "ag_test_and_set", 15) != 0)
            break;
          return std::make_pair(1966, 3);	 // "atomic_flag_test_and_set"
        }
        break;
      }
      break;
    case 'c':	 // 4 strings to match.
      if (memcmp(Name.data()+1, "onvert_ushort16_sat_rt", 22) != 0)
        break;
      switch (Name[23]) {
      default: break;
      case 'e':	 // 1 string to match.
        return std::make_pair(452, 11);	 // "convert_ushort16_sat_rte"
      case 'n':	 // 1 string to match.
        return std::make_pair(452, 11);	 // "convert_ushort16_sat_rtn"
      case 'p':	 // 1 string to match.
        return std::make_pair(452, 11);	 // "convert_ushort16_sat_rtp"
      case 'z':	 // 1 string to match.
        return std::make_pair(452, 11);	 // "convert_ushort16_sat_rtz"
      }
      break;
    case 'g':	 // 1 string to match.
      if (memcmp(Name.data()+1, "et_image_num_mip_levels", 23) != 0)
        break;
      return std::make_pair(2720, 21);	 // "get_image_num_mip_levels"
    case 's':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ub_group_inverse_ballot", 23) != 0)
        break;
      return std::make_pair(2769, 1);	 // "sub_group_inverse_ballot"
    }
    break;
  case 25:	 // 14 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 6 strings to match.
      if (memcmp(Name.data()+1, "tomic_fetch_", 12) != 0)
        break;
      switch (Name[13]) {
      default: break;
      case 'a':	 // 2 strings to match.
        switch (Name[14]) {
        default: break;
        case 'd':	 // 1 string to match.
          if (memcmp(Name.data()+15, "d_explicit", 10) != 0)
            break;
          return std::make_pair(1873, 48);	 // "atomic_fetch_add_explicit"
        case 'n':	 // 1 string to match.
          if (memcmp(Name.data()+15, "d_explicit", 10) != 0)
            break;
          return std::make_pair(1933, 24);	 // "atomic_fetch_and_explicit"
        }
        break;
      case 'm':	 // 2 strings to match.
        switch (Name[14]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (memcmp(Name.data()+15, "x_explicit", 10) != 0)
            break;
          return std::make_pair(1807, 42);	 // "atomic_fetch_max_explicit"
        case 'i':	 // 1 string to match.
          if (memcmp(Name.data()+15, "n_explicit", 10) != 0)
            break;
          return std::make_pair(1807, 42);	 // "atomic_fetch_min_explicit"
        }
        break;
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+14, "ub_explicit", 11) != 0)
          break;
        return std::make_pair(1873, 48);	 // "atomic_fetch_sub_explicit"
      case 'x':	 // 1 string to match.
        if (memcmp(Name.data()+14, "or_explicit", 11) != 0)
          break;
        return std::make_pair(1933, 24);	 // "atomic_fetch_xor_explicit"
      }
      break;
    case 'b':	 // 1 string to match.
      if (memcmp(Name.data()+1, "itfield_extract_unsigned", 24) != 0)
        break;
      return std::make_pair(2785, 2);	 // "bitfield_extract_unsigned"
    case 'c':	 // 1 string to match.
      if (memcmp(Name.data()+1, "lock_read_hilo_sub_group", 24) != 0)
        break;
      return std::make_pair(2803, 1);	 // "clock_read_hilo_sub_group"
    case 'i':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ntel_sub_group_broadcast", 24) != 0)
        break;
      return std::make_pair(2858, 4);	 // "intel_sub_group_broadcast"
    case 's':	 // 5 strings to match.
      if (memcmp(Name.data()+1, "ub_group_", 9) != 0)
        break;
      switch (Name[10]) {
      default: break;
      case 'b':	 // 3 strings to match.
        switch (Name[11]) {
        default: break;
        case 'a':	 // 2 strings to match.
          if (memcmp(Name.data()+12, "llot_find_", 10) != 0)
            break;
          switch (Name[22]) {
          default: break;
          case 'l':	 // 1 string to match.
            if (memcmp(Name.data()+23, "sb", 2) != 0)
              break;
            return std::make_pair(2772, 1);	 // "sub_group_ballot_find_lsb"
          case 'm':	 // 1 string to match.
            if (memcmp(Name.data()+23, "sb", 2) != 0)
              break;
            return std::make_pair(2772, 1);	 // "sub_group_ballot_find_msb"
          }
          break;
        case 'r':	 // 1 string to match.
          if (memcmp(Name.data()+12, "oadcast_first", 13) != 0)
            break;
          return std::make_pair(2767, 1);	 // "sub_group_broadcast_first"
        }
        break;
      case 'n':	 // 2 strings to match.
        if (memcmp(Name.data()+11, "on_uniform_a", 12) != 0)
          break;
        switch (Name[23]) {
        default: break;
        case 'l':	 // 1 string to match.
          if (Name[24] != 'l')
            break;
          return std::make_pair(2764, 1);	 // "sub_group_non_uniform_all"
        case 'n':	 // 1 string to match.
          if (Name[24] != 'y')
            break;
          return std::make_pair(2764, 1);	 // "sub_group_non_uniform_any"
        }
        break;
      }
      break;
    }
    break;
  case 26:	 // 9 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 1 string to match.
      if (memcmp(Name.data()+1, "tomic_flag_clear_explicit", 25) != 0)
        break;
      return std::make_pair(1960, 6);	 // "atomic_flag_clear_explicit"
    case 'c':	 // 1 string to match.
      if (memcmp(Name.data()+1, "lock_read_hilo_work_group", 25) != 0)
        break;
      return std::make_pair(2801, 1);	 // "clock_read_hilo_work_group"
    case 'i':	 // 5 strings to match.
      if (memcmp(Name.data()+1, "ntel_sub_group_", 15) != 0)
        break;
      switch (Name[16]) {
      default: break;
      case 'b':	 // 1 string to match.
        if (memcmp(Name.data()+17, "lock_read", 9) != 0)
          break;
        return std::make_pair(2826, 4);	 // "intel_sub_group_block_read"
      case 'r':	 // 3 strings to match.
        if (memcmp(Name.data()+17, "educe_", 6) != 0)
          break;
        switch (Name[23]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (memcmp(Name.data()+24, "dd", 2) != 0)
            break;
          return std::make_pair(2862, 4);	 // "intel_sub_group_reduce_add"
        case 'm':	 // 2 strings to match.
          switch (Name[24]) {
          default: break;
          case 'a':	 // 1 string to match.
            if (Name[25] != 'x')
              break;
            return std::make_pair(2862, 4);	 // "intel_sub_group_reduce_max"
          case 'i':	 // 1 string to match.
            if (Name[25] != 'n')
              break;
            return std::make_pair(2862, 4);	 // "intel_sub_group_reduce_min"
          }
          break;
        }
        break;
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+17, "huffle_up", 9) != 0)
          break;
        return std::make_pair(2815, 11);	 // "intel_sub_group_shuffle_up"
      }
      break;
    case 's':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "ub_group_", 9) != 0)
        break;
      switch (Name[10]) {
      default: break;
      case 'b':	 // 1 string to match.
        if (memcmp(Name.data()+11, "allot_bit_count", 15) != 0)
          break;
        return std::make_pair(2771, 1);	 // "sub_group_ballot_bit_count"
      case 'c':	 // 1 string to match.
        if (memcmp(Name.data()+11, "lustered_rotate", 15) != 0)
          break;
        return std::make_pair(2797, 1);	 // "sub_group_clustered_rotate"
      }
      break;
    }
    break;
  case 27:	 // 7 strings to match.
    switch (Name[0]) {
    default: break;
    case 'g':	 // 2 strings to match.
      if (memcmp(Name.data()+1, "et_", 3) != 0)
        break;
      switch (Name[4]) {
      default: break;
      case 'e':	 // 1 string to match.
        if (memcmp(Name.data()+5, "nqueued_num_sub_groups", 22) != 0)
          break;
        return std::make_pair(2754, 1);	 // "get_enqueued_num_sub_groups"
      case 'i':	 // 1 string to match.
        if (memcmp(Name.data()+5, "mage_channel_data_type", 22) != 0)
          break;
        return std::make_pair(2559, 36);	 // "get_image_channel_data_type"
      }
      break;
    case 'i':	 // 5 strings to match.
      if (memcmp(Name.data()+1, "ntel_sub_group_", 15) != 0)
        break;
      switch (Name[16]) {
      default: break;
      case 'b':	 // 4 strings to match.
        if (memcmp(Name.data()+17, "lock_", 5) != 0)
          break;
        switch (Name[22]) {
        default: break;
        case 'r':	 // 3 strings to match.
          if (memcmp(Name.data()+23, "ead", 3) != 0)
            break;
          switch (Name[26]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(2830, 4);	 // "intel_sub_group_block_read2"
          case '4':	 // 1 string to match.
            return std::make_pair(2834, 4);	 // "intel_sub_group_block_read4"
          case '8':	 // 1 string to match.
            return std::make_pair(2838, 4);	 // "intel_sub_group_block_read8"
          }
          break;
        case 'w':	 // 1 string to match.
          if (memcmp(Name.data()+23, "rite", 4) != 0)
            break;
          return std::make_pair(2842, 4);	 // "intel_sub_group_block_write"
        }
        break;
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+17, "huffle_xor", 10) != 0)
          break;
        return std::make_pair(2804, 11);	 // "intel_sub_group_shuffle_xor"
      }
      break;
    }
    break;
  case 28:	 // 16 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 1 string to match.
      if (memcmp(Name.data()+1, "tomic_compare_exchange_weak", 27) != 0)
        break;
      return std::make_pair(1596, 42);	 // "atomic_compare_exchange_weak"
    case 'c':	 // 1 string to match.
      if (memcmp(Name.data()+1, "apture_event_profiling_info", 27) != 0)
        break;
      return std::make_pair(2712, 1);	 // "capture_event_profiling_info"
    case 'd':	 // 3 strings to match.
      if (memcmp(Name.data()+1, "ot_acc_sat_4x8packed_", 21) != 0)
        break;
      switch (Name[22]) {
      default: break;
      case 's':	 // 2 strings to match.
        switch (Name[23]) {
        default: break;
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+24, "_int", 4) != 0)
            break;
          return std::make_pair(2795, 1);	 // "dot_acc_sat_4x8packed_ss_int"
        case 'u':	 // 1 string to match.
          if (memcmp(Name.data()+24, "_int", 4) != 0)
            break;
          return std::make_pair(2795, 1);	 // "dot_acc_sat_4x8packed_su_int"
        }
        break;
      case 'u':	 // 1 string to match.
        if (memcmp(Name.data()+23, "s_int", 5) != 0)
          break;
        return std::make_pair(2795, 1);	 // "dot_acc_sat_4x8packed_us_int"
      }
      break;
    case 'i':	 // 4 strings to match.
      if (memcmp(Name.data()+1, "ntel_sub_group_", 15) != 0)
        break;
      switch (Name[16]) {
      default: break;
      case 'b':	 // 3 strings to match.
        if (memcmp(Name.data()+17, "lock_write", 10) != 0)
          break;
        switch (Name[27]) {
        default: break;
        case '2':	 // 1 string to match.
          return std::make_pair(2846, 4);	 // "intel_sub_group_block_write2"
        case '4':	 // 1 string to match.
          return std::make_pair(2850, 4);	 // "intel_sub_group_block_write4"
        case '8':	 // 1 string to match.
          return std::make_pair(2854, 4);	 // "intel_sub_group_block_write8"
        }
        break;
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+17, "huffle_down", 11) != 0)
          break;
        return std::make_pair(2815, 11);	 // "intel_sub_group_shuffle_down"
      }
      break;
    case 's':	 // 7 strings to match.
      if (memcmp(Name.data()+1, "ub_group_", 9) != 0)
        break;
      switch (Name[10]) {
      default: break;
      case 'b':	 // 1 string to match.
        if (memcmp(Name.data()+11, "allot_bit_extract", 17) != 0)
          break;
        return std::make_pair(2770, 1);	 // "sub_group_ballot_bit_extract"
      case 's':	 // 6 strings to match.
        if (memcmp(Name.data()+11, "can_", 4) != 0)
          break;
        switch (Name[15]) {
        default: break;
        case 'e':	 // 3 strings to match.
          if (memcmp(Name.data()+16, "xclusive_", 9) != 0)
            break;
          switch (Name[25]) {
          default: break;
          case 'a':	 // 1 string to match.
            if (memcmp(Name.data()+26, "dd", 2) != 0)
              break;
            return std::make_pair(2761, 2);	 // "sub_group_scan_exclusive_add"
          case 'm':	 // 2 strings to match.
            switch (Name[26]) {
            default: break;
            case 'a':	 // 1 string to match.
              if (Name[27] != 'x')
                break;
              return std::make_pair(2761, 2);	 // "sub_group_scan_exclusive_max"
            case 'i':	 // 1 string to match.
              if (Name[27] != 'n')
                break;
              return std::make_pair(2761, 2);	 // "sub_group_scan_exclusive_min"
            }
            break;
          }
          break;
        case 'i':	 // 3 strings to match.
          if (memcmp(Name.data()+16, "nclusive_", 9) != 0)
            break;
          switch (Name[25]) {
          default: break;
          case 'a':	 // 1 string to match.
            if (memcmp(Name.data()+26, "dd", 2) != 0)
              break;
            return std::make_pair(2761, 2);	 // "sub_group_scan_inclusive_add"
          case 'm':	 // 2 strings to match.
            switch (Name[26]) {
            default: break;
            case 'a':	 // 1 string to match.
              if (Name[27] != 'x')
                break;
              return std::make_pair(2761, 2);	 // "sub_group_scan_inclusive_max"
            case 'i':	 // 1 string to match.
              if (Name[27] != 'n')
                break;
              return std::make_pair(2761, 2);	 // "sub_group_scan_inclusive_min"
            }
            break;
          }
          break;
        }
        break;
      }
      break;
    }
    break;
  case 29:	 // 14 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 1 string to match.
      if (memcmp(Name.data()+1, "sync_work_group_strided_copy", 28) != 0)
        break;
      return std::make_pair(1452, 2);	 // "async_work_group_strided_copy"
    case 'd':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ot_acc_sat_4x8packed_uu_uint", 28) != 0)
        break;
      return std::make_pair(2794, 1);	 // "dot_acc_sat_4x8packed_uu_uint"
    case 'i':	 // 5 strings to match.
      if (memcmp(Name.data()+1, "ntel_", 5) != 0)
        break;
      switch (Name[6]) {
      default: break;
      case 's':	 // 4 strings to match.
        if (memcmp(Name.data()+7, "ub_group_block_read_u", 21) != 0)
          break;
        switch (Name[28]) {
        default: break;
        case 'c':	 // 1 string to match.
          return std::make_pair(2906, 4);	 // "intel_sub_group_block_read_uc"
        case 'i':	 // 1 string to match.
          return std::make_pair(2826, 4);	 // "intel_sub_group_block_read_ui"
        case 'l':	 // 1 string to match.
          return std::make_pair(2946, 4);	 // "intel_sub_group_block_read_ul"
        case 's':	 // 1 string to match.
          return std::make_pair(2866, 4);	 // "intel_sub_group_block_read_us"
        }
        break;
      case 'w':	 // 1 string to match.
        if (memcmp(Name.data()+7, "ork_group_barrier_wait", 22) != 0)
          break;
        return std::make_pair(2998, 2);	 // "intel_work_group_barrier_wait"
      }
      break;
    case 's':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ub_group_clustered_reduce_or", 28) != 0)
        break;
      return std::make_pair(2780, 1);	 // "sub_group_clustered_reduce_or"
    case 'w':	 // 6 strings to match.
      if (memcmp(Name.data()+1, "ork_group_scan_", 15) != 0)
        break;
      switch (Name[16]) {
      default: break;
      case 'e':	 // 3 strings to match.
        if (memcmp(Name.data()+17, "xclusive_", 9) != 0)
          break;
        switch (Name[26]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (memcmp(Name.data()+27, "dd", 2) != 0)
            break;
          return std::make_pair(2705, 1);	 // "work_group_scan_exclusive_add"
        case 'm':	 // 2 strings to match.
          switch (Name[27]) {
          default: break;
          case 'a':	 // 1 string to match.
            if (Name[28] != 'x')
              break;
            return std::make_pair(2705, 1);	 // "work_group_scan_exclusive_max"
          case 'i':	 // 1 string to match.
            if (Name[28] != 'n')
              break;
            return std::make_pair(2705, 1);	 // "work_group_scan_exclusive_min"
          }
          break;
        }
        break;
      case 'i':	 // 3 strings to match.
        if (memcmp(Name.data()+17, "nclusive_", 9) != 0)
          break;
        switch (Name[26]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (memcmp(Name.data()+27, "dd", 2) != 0)
            break;
          return std::make_pair(2705, 1);	 // "work_group_scan_inclusive_add"
        case 'm':	 // 2 strings to match.
          switch (Name[27]) {
          default: break;
          case 'a':	 // 1 string to match.
            if (Name[28] != 'x')
              break;
            return std::make_pair(2705, 1);	 // "work_group_scan_inclusive_max"
          case 'i':	 // 1 string to match.
            if (Name[28] != 'n')
              break;
            return std::make_pair(2705, 1);	 // "work_group_scan_inclusive_min"
          }
          break;
        }
        break;
      }
      break;
    }
    break;
  case 30:	 // 23 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 1 string to match.
      if (memcmp(Name.data()+1, "tomic_compare_exchange_strong", 29) != 0)
        break;
      return std::make_pair(1596, 42);	 // "atomic_compare_exchange_strong"
    case 'i':	 // 16 strings to match.
      if (memcmp(Name.data()+1, "ntel_sub_group_block_", 21) != 0)
        break;
      switch (Name[22]) {
      default: break;
      case 'r':	 // 12 strings to match.
        if (memcmp(Name.data()+23, "ead_u", 5) != 0)
          break;
        switch (Name[28]) {
        default: break;
        case 'c':	 // 3 strings to match.
          switch (Name[29]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(2910, 4);	 // "intel_sub_group_block_read_uc2"
          case '4':	 // 1 string to match.
            return std::make_pair(2914, 4);	 // "intel_sub_group_block_read_uc4"
          case '8':	 // 1 string to match.
            return std::make_pair(2918, 4);	 // "intel_sub_group_block_read_uc8"
          }
          break;
        case 'i':	 // 3 strings to match.
          switch (Name[29]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(2830, 4);	 // "intel_sub_group_block_read_ui2"
          case '4':	 // 1 string to match.
            return std::make_pair(2834, 4);	 // "intel_sub_group_block_read_ui4"
          case '8':	 // 1 string to match.
            return std::make_pair(2838, 4);	 // "intel_sub_group_block_read_ui8"
          }
          break;
        case 'l':	 // 3 strings to match.
          switch (Name[29]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(2950, 4);	 // "intel_sub_group_block_read_ul2"
          case '4':	 // 1 string to match.
            return std::make_pair(2954, 4);	 // "intel_sub_group_block_read_ul4"
          case '8':	 // 1 string to match.
            return std::make_pair(2958, 4);	 // "intel_sub_group_block_read_ul8"
          }
          break;
        case 's':	 // 3 strings to match.
          switch (Name[29]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(2870, 4);	 // "intel_sub_group_block_read_us2"
          case '4':	 // 1 string to match.
            return std::make_pair(2874, 4);	 // "intel_sub_group_block_read_us4"
          case '8':	 // 1 string to match.
            return std::make_pair(2878, 4);	 // "intel_sub_group_block_read_us8"
          }
          break;
        }
        break;
      case 'w':	 // 4 strings to match.
        if (memcmp(Name.data()+23, "rite_u", 6) != 0)
          break;
        switch (Name[29]) {
        default: break;
        case 'c':	 // 1 string to match.
          return std::make_pair(2926, 4);	 // "intel_sub_group_block_write_uc"
        case 'i':	 // 1 string to match.
          return std::make_pair(2842, 4);	 // "intel_sub_group_block_write_ui"
        case 'l':	 // 1 string to match.
          return std::make_pair(2964, 4);	 // "intel_sub_group_block_write_ul"
        case 's':	 // 1 string to match.
          return std::make_pair(2886, 4);	 // "intel_sub_group_block_write_us"
        }
        break;
      }
      break;
    case 's':	 // 6 strings to match.
      if (memcmp(Name.data()+1, "ub_group_clustered_reduce_", 26) != 0)
        break;
      switch (Name[27]) {
      default: break;
      case 'a':	 // 2 strings to match.
        switch (Name[28]) {
        default: break;
        case 'd':	 // 1 string to match.
          if (Name[29] != 'd')
            break;
          return std::make_pair(2779, 1);	 // "sub_group_clustered_reduce_add"
        case 'n':	 // 1 string to match.
          if (Name[29] != 'd')
            break;
          return std::make_pair(2780, 1);	 // "sub_group_clustered_reduce_and"
        }
        break;
      case 'm':	 // 3 strings to match.
        switch (Name[28]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (Name[29] != 'x')
            break;
          return std::make_pair(2779, 1);	 // "sub_group_clustered_reduce_max"
        case 'i':	 // 1 string to match.
          if (Name[29] != 'n')
            break;
          return std::make_pair(2779, 1);	 // "sub_group_clustered_reduce_min"
        case 'u':	 // 1 string to match.
          if (Name[29] != 'l')
            break;
          return std::make_pair(2779, 1);	 // "sub_group_clustered_reduce_mul"
        }
        break;
      case 'x':	 // 1 string to match.
        if (memcmp(Name.data()+28, "or", 2) != 0)
          break;
        return std::make_pair(2780, 1);	 // "sub_group_clustered_reduce_xor"
      }
      break;
    }
    break;
  case 31:	 // 22 strings to match.
    switch (Name[0]) {
    default: break;
    case 'i':	 // 17 strings to match.
      if (memcmp(Name.data()+1, "ntel_", 5) != 0)
        break;
      switch (Name[6]) {
      default: break;
      case 'c':	 // 1 string to match.
        if (memcmp(Name.data()+7, "onvert_as_bfloat16_float", 24) != 0)
          break;
        return std::make_pair(2992, 1);	 // "intel_convert_as_bfloat16_float"
      case 's':	 // 15 strings to match.
        if (memcmp(Name.data()+7, "ub_group_block_", 15) != 0)
          break;
        switch (Name[22]) {
        default: break;
        case 'r':	 // 3 strings to match.
          if (memcmp(Name.data()+23, "ead_u", 5) != 0)
            break;
          switch (Name[28]) {
          default: break;
          case 'c':	 // 1 string to match.
            if (memcmp(Name.data()+29, "16", 2) != 0)
              break;
            return std::make_pair(2922, 4);	 // "intel_sub_group_block_read_uc16"
          case 'l':	 // 1 string to match.
            if (memcmp(Name.data()+29, "16", 2) != 0)
              break;
            return std::make_pair(2962, 2);	 // "intel_sub_group_block_read_ul16"
          case 's':	 // 1 string to match.
            if (memcmp(Name.data()+29, "16", 2) != 0)
              break;
            return std::make_pair(2882, 4);	 // "intel_sub_group_block_read_us16"
          }
          break;
        case 'w':	 // 12 strings to match.
          if (memcmp(Name.data()+23, "rite_u", 6) != 0)
            break;
          switch (Name[29]) {
          default: break;
          case 'c':	 // 3 strings to match.
            switch (Name[30]) {
            default: break;
            case '2':	 // 1 string to match.
              return std::make_pair(2930, 4);	 // "intel_sub_group_block_write_uc2"
            case '4':	 // 1 string to match.
              return std::make_pair(2934, 4);	 // "intel_sub_group_block_write_uc4"
            case '8':	 // 1 string to match.
              return std::make_pair(2938, 4);	 // "intel_sub_group_block_write_uc8"
            }
            break;
          case 'i':	 // 3 strings to match.
            switch (Name[30]) {
            default: break;
            case '2':	 // 1 string to match.
              return std::make_pair(2846, 4);	 // "intel_sub_group_block_write_ui2"
            case '4':	 // 1 string to match.
              return std::make_pair(2850, 4);	 // "intel_sub_group_block_write_ui4"
            case '8':	 // 1 string to match.
              return std::make_pair(2854, 4);	 // "intel_sub_group_block_write_ui8"
            }
            break;
          case 'l':	 // 3 strings to match.
            switch (Name[30]) {
            default: break;
            case '2':	 // 1 string to match.
              return std::make_pair(2968, 4);	 // "intel_sub_group_block_write_ul2"
            case '4':	 // 1 string to match.
              return std::make_pair(2972, 4);	 // "intel_sub_group_block_write_ul4"
            case '8':	 // 1 string to match.
              return std::make_pair(2976, 4);	 // "intel_sub_group_block_write_ul8"
            }
            break;
          case 's':	 // 3 strings to match.
            switch (Name[30]) {
            default: break;
            case '2':	 // 1 string to match.
              return std::make_pair(2890, 4);	 // "intel_sub_group_block_write_us2"
            case '4':	 // 1 string to match.
              return std::make_pair(2894, 4);	 // "intel_sub_group_block_write_us4"
            case '8':	 // 1 string to match.
              return std::make_pair(2898, 4);	 // "intel_sub_group_block_write_us8"
            }
            break;
          }
          break;
        }
        break;
      case 'w':	 // 1 string to match.
        if (memcmp(Name.data()+7, "ork_group_barrier_arrive", 24) != 0)
          break;
        return std::make_pair(2998, 2);	 // "intel_work_group_barrier_arrive"
      }
      break;
    case 's':	 // 5 strings to match.
      if (memcmp(Name.data()+1, "ub_group_", 9) != 0)
        break;
      switch (Name[10]) {
      default: break;
      case 'b':	 // 2 strings to match.
        if (memcmp(Name.data()+11, "allot_", 6) != 0)
          break;
        switch (Name[17]) {
        default: break;
        case 'e':	 // 1 string to match.
          if (memcmp(Name.data()+18, "xclusive_scan", 13) != 0)
            break;
          return std::make_pair(2772, 1);	 // "sub_group_ballot_exclusive_scan"
        case 'i':	 // 1 string to match.
          if (memcmp(Name.data()+18, "nclusive_scan", 13) != 0)
            break;
          return std::make_pair(2772, 1);	 // "sub_group_ballot_inclusive_scan"
        }
        break;
      case 'n':	 // 3 strings to match.
        if (memcmp(Name.data()+11, "on_uniform_", 11) != 0)
          break;
        switch (Name[22]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (memcmp(Name.data()+23, "ll_equal", 8) != 0)
            break;
          return std::make_pair(2765, 1);	 // "sub_group_non_uniform_all_equal"
        case 'b':	 // 1 string to match.
          if (memcmp(Name.data()+23, "roadcast", 8) != 0)
            break;
          return std::make_pair(2766, 1);	 // "sub_group_non_uniform_broadcast"
        case 'r':	 // 1 string to match.
          if (memcmp(Name.data()+23, "educe_or", 8) != 0)
            break;
          return std::make_pair(2775, 1);	 // "sub_group_non_uniform_reduce_or"
        }
        break;
      }
      break;
    }
    break;
  case 32:	 // 10 strings to match.
    switch (Name[0]) {
    default: break;
    case 'i':	 // 4 strings to match.
      if (memcmp(Name.data()+1, "ntel_", 5) != 0)
        break;
      switch (Name[6]) {
      default: break;
      case 'c':	 // 1 string to match.
        if (memcmp(Name.data()+7, "onvert_bfloat16_as_ushort", 25) != 0)
          break;
        return std::make_pair(2986, 1);	 // "intel_convert_bfloat16_as_ushort"
      case 's':	 // 3 strings to match.
        if (memcmp(Name.data()+7, "ub_group_block_write_u", 22) != 0)
          break;
        switch (Name[29]) {
        default: break;
        case 'c':	 // 1 string to match.
          if (memcmp(Name.data()+30, "16", 2) != 0)
            break;
          return std::make_pair(2942, 4);	 // "intel_sub_group_block_write_uc16"
        case 'l':	 // 1 string to match.
          if (memcmp(Name.data()+30, "16", 2) != 0)
            break;
          return std::make_pair(2980, 2);	 // "intel_sub_group_block_write_ul16"
        case 's':	 // 1 string to match.
          if (memcmp(Name.data()+30, "16", 2) != 0)
            break;
          return std::make_pair(2902, 4);	 // "intel_sub_group_block_write_us16"
        }
        break;
      }
      break;
    case 's':	 // 6 strings to match.
      if (memcmp(Name.data()+1, "ub_group_non_uniform_reduce_", 28) != 0)
        break;
      switch (Name[29]) {
      default: break;
      case 'a':	 // 2 strings to match.
        switch (Name[30]) {
        default: break;
        case 'd':	 // 1 string to match.
          if (Name[31] != 'd')
            break;
          return std::make_pair(2774, 1);	 // "sub_group_non_uniform_reduce_add"
        case 'n':	 // 1 string to match.
          if (Name[31] != 'd')
            break;
          return std::make_pair(2775, 1);	 // "sub_group_non_uniform_reduce_and"
        }
        break;
      case 'm':	 // 3 strings to match.
        switch (Name[30]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (Name[31] != 'x')
            break;
          return std::make_pair(2774, 1);	 // "sub_group_non_uniform_reduce_max"
        case 'i':	 // 1 string to match.
          if (Name[31] != 'n')
            break;
          return std::make_pair(2774, 1);	 // "sub_group_non_uniform_reduce_min"
        case 'u':	 // 1 string to match.
          if (Name[31] != 'l')
            break;
          return std::make_pair(2774, 1);	 // "sub_group_non_uniform_reduce_mul"
        }
        break;
      case 'x':	 // 1 string to match.
        if (memcmp(Name.data()+30, "or", 2) != 0)
          break;
        return std::make_pair(2775, 1);	 // "sub_group_non_uniform_reduce_xor"
      }
      break;
    }
    break;
  case 33:	 // 9 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 1 string to match.
      if (memcmp(Name.data()+1, "tomic_flag_test_and_set_explicit", 32) != 0)
        break;
      return std::make_pair(1969, 6);	 // "atomic_flag_test_and_set_explicit"
    case 'i':	 // 8 strings to match.
      if (memcmp(Name.data()+1, "ntel_", 5) != 0)
        break;
      switch (Name[6]) {
      default: break;
      case 'c':	 // 4 strings to match.
        if (memcmp(Name.data()+7, "onvert_as_bfloat16", 18) != 0)
          break;
        switch (Name[25]) {
        default: break;
        case '2':	 // 1 string to match.
          if (memcmp(Name.data()+26, "_float2", 7) != 0)
            break;
          return std::make_pair(2993, 1);	 // "intel_convert_as_bfloat162_float2"
        case '3':	 // 1 string to match.
          if (memcmp(Name.data()+26, "_float3", 7) != 0)
            break;
          return std::make_pair(2994, 1);	 // "intel_convert_as_bfloat163_float3"
        case '4':	 // 1 string to match.
          if (memcmp(Name.data()+26, "_float4", 7) != 0)
            break;
          return std::make_pair(2995, 1);	 // "intel_convert_as_bfloat164_float4"
        case '8':	 // 1 string to match.
          if (memcmp(Name.data()+26, "_float8", 7) != 0)
            break;
          return std::make_pair(2996, 1);	 // "intel_convert_as_bfloat168_float8"
        }
        break;
      case 's':	 // 4 strings to match.
        if (memcmp(Name.data()+7, "ub_group_block_prefetch_u", 25) != 0)
          break;
        switch (Name[32]) {
        default: break;
        case 'c':	 // 1 string to match.
          return std::make_pair(2984, 1);	 // "intel_sub_group_block_prefetch_uc"
        case 'i':	 // 1 string to match.
          return std::make_pair(2982, 1);	 // "intel_sub_group_block_prefetch_ui"
        case 'l':	 // 1 string to match.
          return std::make_pair(2985, 1);	 // "intel_sub_group_block_prefetch_ul"
        case 's':	 // 1 string to match.
          return std::make_pair(2983, 1);	 // "intel_sub_group_block_prefetch_us"
        }
        break;
      }
      break;
    }
    break;
  case 34:	 // 22 strings to match.
    if (memcmp(Name.data()+0, "intel_", 6) != 0)
      break;
    switch (Name[6]) {
    default: break;
    case 'c':	 // 4 strings to match.
      if (memcmp(Name.data()+7, "onvert_bfloat16", 15) != 0)
        break;
      switch (Name[22]) {
      default: break;
      case '2':	 // 1 string to match.
        if (memcmp(Name.data()+23, "_as_ushort2", 11) != 0)
          break;
        return std::make_pair(2987, 1);	 // "intel_convert_bfloat162_as_ushort2"
      case '3':	 // 1 string to match.
        if (memcmp(Name.data()+23, "_as_ushort3", 11) != 0)
          break;
        return std::make_pair(2988, 1);	 // "intel_convert_bfloat163_as_ushort3"
      case '4':	 // 1 string to match.
        if (memcmp(Name.data()+23, "_as_ushort4", 11) != 0)
          break;
        return std::make_pair(2989, 1);	 // "intel_convert_bfloat164_as_ushort4"
      case '8':	 // 1 string to match.
        if (memcmp(Name.data()+23, "_as_ushort8", 11) != 0)
          break;
        return std::make_pair(2990, 1);	 // "intel_convert_bfloat168_as_ushort8"
      }
      break;
    case 's':	 // 18 strings to match.
      if (memcmp(Name.data()+7, "ub_group_", 9) != 0)
        break;
      switch (Name[16]) {
      default: break;
      case 'b':	 // 12 strings to match.
        if (memcmp(Name.data()+17, "lock_prefetch_u", 15) != 0)
          break;
        switch (Name[32]) {
        default: break;
        case 'c':	 // 3 strings to match.
          switch (Name[33]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(2984, 1);	 // "intel_sub_group_block_prefetch_uc2"
          case '4':	 // 1 string to match.
            return std::make_pair(2984, 1);	 // "intel_sub_group_block_prefetch_uc4"
          case '8':	 // 1 string to match.
            return std::make_pair(2984, 1);	 // "intel_sub_group_block_prefetch_uc8"
          }
          break;
        case 'i':	 // 3 strings to match.
          switch (Name[33]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(2982, 1);	 // "intel_sub_group_block_prefetch_ui2"
          case '4':	 // 1 string to match.
            return std::make_pair(2982, 1);	 // "intel_sub_group_block_prefetch_ui4"
          case '8':	 // 1 string to match.
            return std::make_pair(2982, 1);	 // "intel_sub_group_block_prefetch_ui8"
          }
          break;
        case 'l':	 // 3 strings to match.
          switch (Name[33]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(2985, 1);	 // "intel_sub_group_block_prefetch_ul2"
          case '4':	 // 1 string to match.
            return std::make_pair(2985, 1);	 // "intel_sub_group_block_prefetch_ul4"
          case '8':	 // 1 string to match.
            return std::make_pair(2985, 1);	 // "intel_sub_group_block_prefetch_ul8"
          }
          break;
        case 's':	 // 3 strings to match.
          switch (Name[33]) {
          default: break;
          case '2':	 // 1 string to match.
            return std::make_pair(2983, 1);	 // "intel_sub_group_block_prefetch_us2"
          case '4':	 // 1 string to match.
            return std::make_pair(2983, 1);	 // "intel_sub_group_block_prefetch_us4"
          case '8':	 // 1 string to match.
            return std::make_pair(2983, 1);	 // "intel_sub_group_block_prefetch_us8"
          }
          break;
        }
        break;
      case 's':	 // 6 strings to match.
        if (memcmp(Name.data()+17, "can_", 4) != 0)
          break;
        switch (Name[21]) {
        default: break;
        case 'e':	 // 3 strings to match.
          if (memcmp(Name.data()+22, "xclusive_", 9) != 0)
            break;
          switch (Name[31]) {
          default: break;
          case 'a':	 // 1 string to match.
            if (memcmp(Name.data()+32, "dd", 2) != 0)
              break;
            return std::make_pair(2862, 4);	 // "intel_sub_group_scan_exclusive_add"
          case 'm':	 // 2 strings to match.
            switch (Name[32]) {
            default: break;
            case 'a':	 // 1 string to match.
              if (Name[33] != 'x')
                break;
              return std::make_pair(2862, 4);	 // "intel_sub_group_scan_exclusive_max"
            case 'i':	 // 1 string to match.
              if (Name[33] != 'n')
                break;
              return std::make_pair(2862, 4);	 // "intel_sub_group_scan_exclusive_min"
            }
            break;
          }
          break;
        case 'i':	 // 3 strings to match.
          if (memcmp(Name.data()+22, "nclusive_", 9) != 0)
            break;
          switch (Name[31]) {
          default: break;
          case 'a':	 // 1 string to match.
            if (memcmp(Name.data()+32, "dd", 2) != 0)
              break;
            return std::make_pair(2862, 4);	 // "intel_sub_group_scan_inclusive_add"
          case 'm':	 // 2 strings to match.
            switch (Name[32]) {
            default: break;
            case 'a':	 // 1 string to match.
              if (Name[33] != 'x')
                break;
              return std::make_pair(2862, 4);	 // "intel_sub_group_scan_inclusive_max"
            case 'i':	 // 1 string to match.
              if (Name[33] != 'n')
                break;
              return std::make_pair(2862, 4);	 // "intel_sub_group_scan_inclusive_min"
            }
            break;
          }
          break;
        }
        break;
      }
      break;
    }
    break;
  case 35:	 // 3 strings to match.
    if (memcmp(Name.data()+0, "intel_", 6) != 0)
      break;
    switch (Name[6]) {
    default: break;
    case 'c':	 // 1 string to match.
      if (memcmp(Name.data()+7, "onvert_as_bfloat1616_float16", 28) != 0)
        break;
      return std::make_pair(2997, 1);	 // "intel_convert_as_bfloat1616_float16"
    case 's':	 // 2 strings to match.
      if (memcmp(Name.data()+7, "ub_group_block_prefetch_u", 25) != 0)
        break;
      switch (Name[32]) {
      default: break;
      case 'c':	 // 1 string to match.
        if (memcmp(Name.data()+33, "16", 2) != 0)
          break;
        return std::make_pair(2984, 1);	 // "intel_sub_group_block_prefetch_uc16"
      case 's':	 // 1 string to match.
        if (memcmp(Name.data()+33, "16", 2) != 0)
          break;
        return std::make_pair(2983, 1);	 // "intel_sub_group_block_prefetch_us16"
      }
      break;
    }
    break;
  case 36:	 // 1 string to match.
    if (memcmp(Name.data()+0, "intel_convert_bfloat1616_as_ushort16", 36) != 0)
      break;
    return std::make_pair(2991, 1);	 // "intel_convert_bfloat1616_as_ushort16"
  case 37:	 // 2 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 1 string to match.
      if (memcmp(Name.data()+1, "tomic_compare_exchange_weak_explicit", 36) != 0)
        break;
      return std::make_pair(1638, 84);	 // "atomic_compare_exchange_weak_explicit"
    case 's':	 // 1 string to match.
      if (memcmp(Name.data()+1, "ub_group_clustered_reduce_logical_or", 36) != 0)
        break;
      return std::make_pair(2781, 1);	 // "sub_group_clustered_reduce_logical_or"
    }
    break;
  case 38:	 // 2 strings to match.
    if (memcmp(Name.data()+0, "sub_group_clustered_reduce_logical_", 35) != 0)
      break;
    switch (Name[35]) {
    default: break;
    case 'a':	 // 1 string to match.
      if (memcmp(Name.data()+36, "nd", 2) != 0)
        break;
      return std::make_pair(2781, 1);	 // "sub_group_clustered_reduce_logical_and"
    case 'x':	 // 1 string to match.
      if (memcmp(Name.data()+36, "or", 2) != 0)
        break;
      return std::make_pair(2781, 1);	 // "sub_group_clustered_reduce_logical_xor"
    }
    break;
  case 39:	 // 4 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 1 string to match.
      if (memcmp(Name.data()+1, "tomic_compare_exchange_strong_explicit", 38) != 0)
        break;
      return std::make_pair(1638, 84);	 // "atomic_compare_exchange_strong_explicit"
    case 's':	 // 3 strings to match.
      if (memcmp(Name.data()+1, "ub_group_non_uniform_", 21) != 0)
        break;
      switch (Name[22]) {
      default: break;
      case 'r':	 // 1 string to match.
        if (memcmp(Name.data()+23, "educe_logical_or", 16) != 0)
          break;
        return std::make_pair(2776, 1);	 // "sub_group_non_uniform_reduce_logical_or"
      case 's':	 // 2 strings to match.
        if (memcmp(Name.data()+23, "can_", 4) != 0)
          break;
        switch (Name[27]) {
        default: break;
        case 'e':	 // 1 string to match.
          if (memcmp(Name.data()+28, "xclusive_or", 11) != 0)
            break;
          return std::make_pair(2775, 1);	 // "sub_group_non_uniform_scan_exclusive_or"
        case 'i':	 // 1 string to match.
          if (memcmp(Name.data()+28, "nclusive_or", 11) != 0)
            break;
          return std::make_pair(2775, 1);	 // "sub_group_non_uniform_scan_inclusive_or"
        }
        break;
      }
      break;
    }
    break;
  case 40:	 // 14 strings to match.
    if (memcmp(Name.data()+0, "sub_group_non_uniform_", 22) != 0)
      break;
    switch (Name[22]) {
    default: break;
    case 'r':	 // 2 strings to match.
      if (memcmp(Name.data()+23, "educe_logical_", 14) != 0)
        break;
      switch (Name[37]) {
      default: break;
      case 'a':	 // 1 string to match.
        if (memcmp(Name.data()+38, "nd", 2) != 0)
          break;
        return std::make_pair(2776, 1);	 // "sub_group_non_uniform_reduce_logical_and"
      case 'x':	 // 1 string to match.
        if (memcmp(Name.data()+38, "or", 2) != 0)
          break;
        return std::make_pair(2776, 1);	 // "sub_group_non_uniform_reduce_logical_xor"
      }
      break;
    case 's':	 // 12 strings to match.
      if (memcmp(Name.data()+23, "can_", 4) != 0)
        break;
      switch (Name[27]) {
      default: break;
      case 'e':	 // 6 strings to match.
        if (memcmp(Name.data()+28, "xclusive_", 9) != 0)
          break;
        switch (Name[37]) {
        default: break;
        case 'a':	 // 2 strings to match.
          switch (Name[38]) {
          default: break;
          case 'd':	 // 1 string to match.
            if (Name[39] != 'd')
              break;
            return std::make_pair(2774, 1);	 // "sub_group_non_uniform_scan_exclusive_add"
          case 'n':	 // 1 string to match.
            if (Name[39] != 'd')
              break;
            return std::make_pair(2775, 1);	 // "sub_group_non_uniform_scan_exclusive_and"
          }
          break;
        case 'm':	 // 3 strings to match.
          switch (Name[38]) {
          default: break;
          case 'a':	 // 1 string to match.
            if (Name[39] != 'x')
              break;
            return std::make_pair(2774, 1);	 // "sub_group_non_uniform_scan_exclusive_max"
          case 'i':	 // 1 string to match.
            if (Name[39] != 'n')
              break;
            return std::make_pair(2774, 1);	 // "sub_group_non_uniform_scan_exclusive_min"
          case 'u':	 // 1 string to match.
            if (Name[39] != 'l')
              break;
            return std::make_pair(2774, 1);	 // "sub_group_non_uniform_scan_exclusive_mul"
          }
          break;
        case 'x':	 // 1 string to match.
          if (memcmp(Name.data()+38, "or", 2) != 0)
            break;
          return std::make_pair(2775, 1);	 // "sub_group_non_uniform_scan_exclusive_xor"
        }
        break;
      case 'i':	 // 6 strings to match.
        if (memcmp(Name.data()+28, "nclusive_", 9) != 0)
          break;
        switch (Name[37]) {
        default: break;
        case 'a':	 // 2 strings to match.
          switch (Name[38]) {
          default: break;
          case 'd':	 // 1 string to match.
            if (Name[39] != 'd')
              break;
            return std::make_pair(2774, 1);	 // "sub_group_non_uniform_scan_inclusive_add"
          case 'n':	 // 1 string to match.
            if (Name[39] != 'd')
              break;
            return std::make_pair(2775, 1);	 // "sub_group_non_uniform_scan_inclusive_and"
          }
          break;
        case 'm':	 // 3 strings to match.
          switch (Name[38]) {
          default: break;
          case 'a':	 // 1 string to match.
            if (Name[39] != 'x')
              break;
            return std::make_pair(2774, 1);	 // "sub_group_non_uniform_scan_inclusive_max"
          case 'i':	 // 1 string to match.
            if (Name[39] != 'n')
              break;
            return std::make_pair(2774, 1);	 // "sub_group_non_uniform_scan_inclusive_min"
          case 'u':	 // 1 string to match.
            if (Name[39] != 'l')
              break;
            return std::make_pair(2774, 1);	 // "sub_group_non_uniform_scan_inclusive_mul"
          }
          break;
        case 'x':	 // 1 string to match.
          if (memcmp(Name.data()+38, "or", 2) != 0)
            break;
          return std::make_pair(2775, 1);	 // "sub_group_non_uniform_scan_inclusive_xor"
        }
        break;
      }
      break;
    }
    break;
  case 47:	 // 2 strings to match.
    if (memcmp(Name.data()+0, "sub_group_non_uniform_scan_", 27) != 0)
      break;
    switch (Name[27]) {
    default: break;
    case 'e':	 // 1 string to match.
      if (memcmp(Name.data()+28, "xclusive_logical_or", 19) != 0)
        break;
      return std::make_pair(2776, 1);	 // "sub_group_non_uniform_scan_exclusive_logical_or"
    case 'i':	 // 1 string to match.
      if (memcmp(Name.data()+28, "nclusive_logical_or", 19) != 0)
        break;
      return std::make_pair(2776, 1);	 // "sub_group_non_uniform_scan_inclusive_logical_or"
    }
    break;
  case 48:	 // 4 strings to match.
    if (memcmp(Name.data()+0, "sub_group_non_uniform_scan_", 27) != 0)
      break;
    switch (Name[27]) {
    default: break;
    case 'e':	 // 2 strings to match.
      if (memcmp(Name.data()+28, "xclusive_logical_", 17) != 0)
        break;
      switch (Name[45]) {
      default: break;
      case 'a':	 // 1 string to match.
        if (memcmp(Name.data()+46, "nd", 2) != 0)
          break;
        return std::make_pair(2776, 1);	 // "sub_group_non_uniform_scan_exclusive_logical_and"
      case 'x':	 // 1 string to match.
        if (memcmp(Name.data()+46, "or", 2) != 0)
          break;
        return std::make_pair(2776, 1);	 // "sub_group_non_uniform_scan_exclusive_logical_xor"
      }
      break;
    case 'i':	 // 2 strings to match.
      if (memcmp(Name.data()+28, "nclusive_logical_", 17) != 0)
        break;
      switch (Name[45]) {
      default: break;
      case 'a':	 // 1 string to match.
        if (memcmp(Name.data()+46, "nd", 2) != 0)
          break;
        return std::make_pair(2776, 1);	 // "sub_group_non_uniform_scan_inclusive_logical_and"
      case 'x':	 // 1 string to match.
        if (memcmp(Name.data()+46, "or", 2) != 0)
          break;
        return std::make_pair(2776, 1);	 // "sub_group_non_uniform_scan_inclusive_logical_xor"
      }
      break;
    }
    break;
  }
  return std::make_pair(0, 0);
} // isOpenCLBuiltin


static QualType getOpenCLEnumType(Sema &S, llvm::StringRef Name);
static QualType getOpenCLTypedefType(Sema &S, llvm::StringRef Name);

// Convert an OpenCLTypeStruct type to a list of QualTypes.
// Generic types represent multiple types and vector sizes, thus a vector
// is returned. The conversion is done in two steps:
// Step 1: A switch statement fills a vector with scalar base types for the
//         Cartesian product of (vector sizes) x (types) for generic types,
//         or a single scalar type for non generic types.
// Step 2: Qualifiers and other type properties such as vector size are
//         applied.
static void OCL2Qual(Sema &S, const OpenCLTypeStruct &Ty,
                     llvm::SmallVectorImpl<QualType> &QT) {
  ASTContext &Context = S.Context;
  // Number of scalar types in the GenType.
  unsigned GenTypeNumTypes;
  // Pointer to the list of vector sizes for the GenType.
  llvm::ArrayRef<unsigned> GenVectorSizes;
  constexpr unsigned ListVec1[] = {1, };
  constexpr unsigned ListVec1234[] = {1, 2, 3, 4, };
  constexpr unsigned ListVecAndScalar[] = {1, 2, 3, 4, 8, 16, };
  constexpr unsigned ListVecAndScalarTo8[] = {1, 2, 3, 4, 8, };
  constexpr unsigned ListVecNoScalar[] = {2, 3, 4, 8, 16, };

  switch (Ty.ID) {
    case OCLT_image2d_array_msaa_depth_t:
      switch (Ty.AccessQualifier) {
        case OCLAQ_None:
          llvm_unreachable("Image without access qualifier");
        case OCLAQ_ReadOnly:
          QT.push_back(Context.OCLImage2dArrayMSAADepthROTy);
          break;
        case OCLAQ_WriteOnly:
          QT.push_back(Context.OCLImage2dArrayMSAADepthWOTy);
          break;
        case OCLAQ_ReadWrite:
          if (S.getPreprocessor().isMacroDefined("__opencl_c_read_write_images")) {
            QT.push_back(Context.OCLImage2dArrayMSAADepthRWTy);
          }
          break;
      }
      break;
    case OCLT_image1d_t:
      switch (Ty.AccessQualifier) {
        case OCLAQ_None:
          llvm_unreachable("Image without access qualifier");
        case OCLAQ_ReadOnly:
          QT.push_back(Context.OCLImage1dROTy);
          break;
        case OCLAQ_ReadWrite:
          if (S.getPreprocessor().isMacroDefined("__opencl_c_read_write_images")) {
            QT.push_back(Context.OCLImage1dRWTy);
          }
          break;
        case OCLAQ_WriteOnly:
          QT.push_back(Context.OCLImage1dWOTy);
          break;
      }
      break;
    case OCLT_image2d_msaa_t:
      switch (Ty.AccessQualifier) {
        case OCLAQ_None:
          llvm_unreachable("Image without access qualifier");
        case OCLAQ_ReadOnly:
          QT.push_back(Context.OCLImage2dMSAAROTy);
          break;
        case OCLAQ_WriteOnly:
          QT.push_back(Context.OCLImage2dMSAAWOTy);
          break;
        case OCLAQ_ReadWrite:
          if (S.getPreprocessor().isMacroDefined("__opencl_c_read_write_images")) {
            QT.push_back(Context.OCLImage2dMSAARWTy);
          }
          break;
      }
      break;
    case OCLT_image2d_array_t:
      switch (Ty.AccessQualifier) {
        case OCLAQ_None:
          llvm_unreachable("Image without access qualifier");
        case OCLAQ_ReadOnly:
          QT.push_back(Context.OCLImage2dArrayROTy);
          break;
        case OCLAQ_ReadWrite:
          if (S.getPreprocessor().isMacroDefined("__opencl_c_read_write_images")) {
            QT.push_back(Context.OCLImage2dArrayRWTy);
          }
          break;
        case OCLAQ_WriteOnly:
          QT.push_back(Context.OCLImage2dArrayWOTy);
          break;
      }
      break;
    case OCLT_image2d_array_depth_t:
      switch (Ty.AccessQualifier) {
        case OCLAQ_None:
          llvm_unreachable("Image without access qualifier");
        case OCLAQ_ReadOnly:
          if (S.getPreprocessor().isMacroDefined("__opencl_depth_image_builtins")) {
            QT.push_back(Context.OCLImage2dArrayDepthROTy);
          }
          break;
        case OCLAQ_ReadWrite:
          if (S.getPreprocessor().isMacroDefined("__opencl_depth_image_builtins")) {
            QT.push_back(Context.OCLImage2dArrayDepthRWTy);
          }
          break;
        case OCLAQ_WriteOnly:
          if (S.getPreprocessor().isMacroDefined("__opencl_depth_image_builtins")) {
            QT.push_back(Context.OCLImage2dArrayDepthWOTy);
          }
          break;
      }
      break;
    case OCLT_image2d_t:
      switch (Ty.AccessQualifier) {
        case OCLAQ_None:
          llvm_unreachable("Image without access qualifier");
        case OCLAQ_ReadOnly:
          QT.push_back(Context.OCLImage2dROTy);
          break;
        case OCLAQ_ReadWrite:
          if (S.getPreprocessor().isMacroDefined("__opencl_c_read_write_images")) {
            QT.push_back(Context.OCLImage2dRWTy);
          }
          break;
        case OCLAQ_WriteOnly:
          QT.push_back(Context.OCLImage2dWOTy);
          break;
      }
      break;
    case OCLT_image3d_t:
      switch (Ty.AccessQualifier) {
        case OCLAQ_None:
          llvm_unreachable("Image without access qualifier");
        case OCLAQ_ReadOnly:
          QT.push_back(Context.OCLImage3dROTy);
          break;
        case OCLAQ_ReadWrite:
          if (S.getPreprocessor().isMacroDefined("cl_khr_3d_image_writes") && S.getPreprocessor().isMacroDefined("__opencl_c_read_write_images")) {
            QT.push_back(Context.OCLImage3dRWTy);
          }
          break;
        case OCLAQ_WriteOnly:
          if (S.getPreprocessor().isMacroDefined("cl_khr_3d_image_writes")) {
            QT.push_back(Context.OCLImage3dWOTy);
          }
          break;
      }
      break;
    case OCLT_image1d_array_t:
      switch (Ty.AccessQualifier) {
        case OCLAQ_None:
          llvm_unreachable("Image without access qualifier");
        case OCLAQ_ReadOnly:
          QT.push_back(Context.OCLImage1dArrayROTy);
          break;
        case OCLAQ_ReadWrite:
          if (S.getPreprocessor().isMacroDefined("__opencl_c_read_write_images")) {
            QT.push_back(Context.OCLImage1dArrayRWTy);
          }
          break;
        case OCLAQ_WriteOnly:
          QT.push_back(Context.OCLImage1dArrayWOTy);
          break;
      }
      break;
    case OCLT_image1d_buffer_t:
      switch (Ty.AccessQualifier) {
        case OCLAQ_None:
          llvm_unreachable("Image without access qualifier");
        case OCLAQ_ReadOnly:
          QT.push_back(Context.OCLImage1dBufferROTy);
          break;
        case OCLAQ_ReadWrite:
          if (S.getPreprocessor().isMacroDefined("__opencl_c_read_write_images")) {
            QT.push_back(Context.OCLImage1dBufferRWTy);
          }
          break;
        case OCLAQ_WriteOnly:
          QT.push_back(Context.OCLImage1dBufferWOTy);
          break;
      }
      break;
    case OCLT_image2d_depth_t:
      switch (Ty.AccessQualifier) {
        case OCLAQ_None:
          llvm_unreachable("Image without access qualifier");
        case OCLAQ_ReadOnly:
          if (S.getPreprocessor().isMacroDefined("__opencl_depth_image_builtins")) {
            QT.push_back(Context.OCLImage2dDepthROTy);
          }
          break;
        case OCLAQ_ReadWrite:
          if (S.getPreprocessor().isMacroDefined("__opencl_depth_image_builtins")) {
            QT.push_back(Context.OCLImage2dDepthRWTy);
          }
          break;
        case OCLAQ_WriteOnly:
          if (S.getPreprocessor().isMacroDefined("__opencl_depth_image_builtins")) {
            QT.push_back(Context.OCLImage2dDepthWOTy);
          }
          break;
      }
      break;
    case OCLT_image2d_array_msaa_t:
      switch (Ty.AccessQualifier) {
        case OCLAQ_None:
          llvm_unreachable("Image without access qualifier");
        case OCLAQ_ReadOnly:
          QT.push_back(Context.OCLImage2dArrayMSAAROTy);
          break;
        case OCLAQ_WriteOnly:
          QT.push_back(Context.OCLImage2dArrayMSAAWOTy);
          break;
        case OCLAQ_ReadWrite:
          if (S.getPreprocessor().isMacroDefined("__opencl_c_read_write_images")) {
            QT.push_back(Context.OCLImage2dArrayMSAARWTy);
          }
          break;
      }
      break;
    case OCLT_image2d_msaa_depth_t:
      switch (Ty.AccessQualifier) {
        case OCLAQ_None:
          llvm_unreachable("Image without access qualifier");
        case OCLAQ_ReadOnly:
          QT.push_back(Context.OCLImage2dMSAADepthROTy);
          break;
        case OCLAQ_WriteOnly:
          QT.push_back(Context.OCLImage2dMSAADepthWOTy);
          break;
        case OCLAQ_ReadWrite:
          if (S.getPreprocessor().isMacroDefined("__opencl_c_read_write_images")) {
            QT.push_back(Context.OCLImage2dMSAADepthRWTy);
          }
          break;
      }
      break;
    case OCLT_AGenType1: {
      SmallVector<QualType, 11> TypeList;
      TypeList.push_back(Context.CharTy);
      TypeList.push_back(Context.UnsignedCharTy);
      TypeList.push_back(Context.ShortTy);
      TypeList.push_back(Context.UnsignedShortTy);
      TypeList.push_back(Context.IntTy);
      TypeList.push_back(Context.UnsignedIntTy);
      TypeList.push_back(Context.LongTy);
      TypeList.push_back(Context.UnsignedLongTy);
      TypeList.push_back(Context.FloatTy);
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp64")) {
        TypeList.push_back(Context.DoubleTy);
      }
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp16")) {
        TypeList.push_back(Context.HalfTy);
      }
      GenTypeNumTypes = TypeList.size();
      QT.reserve(11);
      for (unsigned I = 0; I < 1; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVec1;
      break;
    }
    case OCLT_AGenTypeN: {
      SmallVector<QualType, 11> TypeList;
      TypeList.push_back(Context.CharTy);
      TypeList.push_back(Context.UnsignedCharTy);
      TypeList.push_back(Context.ShortTy);
      TypeList.push_back(Context.UnsignedShortTy);
      TypeList.push_back(Context.IntTy);
      TypeList.push_back(Context.UnsignedIntTy);
      TypeList.push_back(Context.LongTy);
      TypeList.push_back(Context.UnsignedLongTy);
      TypeList.push_back(Context.FloatTy);
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp64")) {
        TypeList.push_back(Context.DoubleTy);
      }
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp16")) {
        TypeList.push_back(Context.HalfTy);
      }
      GenTypeNumTypes = TypeList.size();
      QT.reserve(66);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_AGenTypeNNoScalar: {
      SmallVector<QualType, 11> TypeList;
      TypeList.push_back(Context.CharTy);
      TypeList.push_back(Context.UnsignedCharTy);
      TypeList.push_back(Context.ShortTy);
      TypeList.push_back(Context.UnsignedShortTy);
      TypeList.push_back(Context.IntTy);
      TypeList.push_back(Context.UnsignedIntTy);
      TypeList.push_back(Context.LongTy);
      TypeList.push_back(Context.UnsignedLongTy);
      TypeList.push_back(Context.FloatTy);
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp64")) {
        TypeList.push_back(Context.DoubleTy);
      }
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp16")) {
        TypeList.push_back(Context.HalfTy);
      }
      GenTypeNumTypes = TypeList.size();
      QT.reserve(55);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecNoScalar;
      break;
    }
    case OCLT_AI2UGenTypeN: {
      SmallVector<QualType, 8> TypeList;
      TypeList.push_back(Context.UnsignedCharTy);
      TypeList.push_back(Context.UnsignedCharTy);
      TypeList.push_back(Context.UnsignedShortTy);
      TypeList.push_back(Context.UnsignedShortTy);
      TypeList.push_back(Context.UnsignedIntTy);
      TypeList.push_back(Context.UnsignedIntTy);
      TypeList.push_back(Context.UnsignedLongTy);
      TypeList.push_back(Context.UnsignedLongTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(48);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_AIGenType1: {
      SmallVector<QualType, 8> TypeList;
      TypeList.push_back(Context.CharTy);
      TypeList.push_back(Context.UnsignedCharTy);
      TypeList.push_back(Context.ShortTy);
      TypeList.push_back(Context.UnsignedShortTy);
      TypeList.push_back(Context.IntTy);
      TypeList.push_back(Context.UnsignedIntTy);
      TypeList.push_back(Context.LongTy);
      TypeList.push_back(Context.UnsignedLongTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(8);
      for (unsigned I = 0; I < 1; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVec1;
      break;
    }
    case OCLT_AIGenTypeN: {
      SmallVector<QualType, 8> TypeList;
      TypeList.push_back(Context.CharTy);
      TypeList.push_back(Context.UnsignedCharTy);
      TypeList.push_back(Context.ShortTy);
      TypeList.push_back(Context.UnsignedShortTy);
      TypeList.push_back(Context.IntTy);
      TypeList.push_back(Context.UnsignedIntTy);
      TypeList.push_back(Context.LongTy);
      TypeList.push_back(Context.UnsignedLongTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(48);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_AIGenTypeNNoScalar: {
      SmallVector<QualType, 8> TypeList;
      TypeList.push_back(Context.CharTy);
      TypeList.push_back(Context.UnsignedCharTy);
      TypeList.push_back(Context.ShortTy);
      TypeList.push_back(Context.UnsignedShortTy);
      TypeList.push_back(Context.IntTy);
      TypeList.push_back(Context.UnsignedIntTy);
      TypeList.push_back(Context.LongTy);
      TypeList.push_back(Context.UnsignedLongTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(40);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecNoScalar;
      break;
    }
    case OCLT_CharShortGenType1: {
      SmallVector<QualType, 4> TypeList;
      TypeList.push_back(Context.CharTy);
      TypeList.push_back(Context.UnsignedCharTy);
      TypeList.push_back(Context.ShortTy);
      TypeList.push_back(Context.UnsignedShortTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(4);
      for (unsigned I = 0; I < 1; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVec1;
      break;
    }
    case OCLT_FGenTypeN: {
      SmallVector<QualType, 3> TypeList;
      TypeList.push_back(Context.FloatTy);
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp64")) {
        TypeList.push_back(Context.DoubleTy);
      }
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp16")) {
        TypeList.push_back(Context.HalfTy);
      }
      GenTypeNumTypes = TypeList.size();
      QT.reserve(18);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_GenTypeCharVecAndScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.CharTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(6);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_GenTypeCharVecAndScalarTo8: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.CharTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalarTo8;
      break;
    }
    case OCLT_GenTypeCharVecNoScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.CharTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecNoScalar;
      break;
    }
    case OCLT_GenTypeDoubleVec1234: {
      SmallVector<QualType, 1> TypeList;
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp64")) {
        TypeList.push_back(Context.DoubleTy);
      }
      GenTypeNumTypes = TypeList.size();
      QT.reserve(4);
      for (unsigned I = 0; I < 4; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVec1234;
      break;
    }
    case OCLT_GenTypeDoubleVecAndScalar: {
      SmallVector<QualType, 1> TypeList;
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp64")) {
        TypeList.push_back(Context.DoubleTy);
      }
      GenTypeNumTypes = TypeList.size();
      QT.reserve(6);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_GenTypeDoubleVecNoScalar: {
      SmallVector<QualType, 1> TypeList;
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp64")) {
        TypeList.push_back(Context.DoubleTy);
      }
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecNoScalar;
      break;
    }
    case OCLT_GenTypeFloatVec1234: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.FloatTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(4);
      for (unsigned I = 0; I < 4; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVec1234;
      break;
    }
    case OCLT_GenTypeFloatVecAndScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.FloatTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(6);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_GenTypeFloatVecNoScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.FloatTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecNoScalar;
      break;
    }
    case OCLT_GenTypeHalfVec1234: {
      SmallVector<QualType, 1> TypeList;
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp16")) {
        TypeList.push_back(Context.HalfTy);
      }
      GenTypeNumTypes = TypeList.size();
      QT.reserve(4);
      for (unsigned I = 0; I < 4; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVec1234;
      break;
    }
    case OCLT_GenTypeHalfVecAndScalar: {
      SmallVector<QualType, 1> TypeList;
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp16")) {
        TypeList.push_back(Context.HalfTy);
      }
      GenTypeNumTypes = TypeList.size();
      QT.reserve(6);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_GenTypeHalfVecNoScalar: {
      SmallVector<QualType, 1> TypeList;
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp16")) {
        TypeList.push_back(Context.HalfTy);
      }
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecNoScalar;
      break;
    }
    case OCLT_GenTypeIntVecAndScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.IntTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(6);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_GenTypeIntVecNoScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.IntTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecNoScalar;
      break;
    }
    case OCLT_GenTypeLongVecAndScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.LongTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(6);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_GenTypeLongVecNoScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.LongTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecNoScalar;
      break;
    }
    case OCLT_GenTypeShortVecAndScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.ShortTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(6);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_GenTypeShortVecAndScalarTo8: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.ShortTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalarTo8;
      break;
    }
    case OCLT_GenTypeShortVecNoScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.ShortTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecNoScalar;
      break;
    }
    case OCLT_GenTypeUCharVecAndScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.UnsignedCharTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(6);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_GenTypeUCharVecAndScalarTo8: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.UnsignedCharTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalarTo8;
      break;
    }
    case OCLT_GenTypeUCharVecNoScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.UnsignedCharTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecNoScalar;
      break;
    }
    case OCLT_GenTypeUIntVecAndScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.UnsignedIntTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(6);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_GenTypeUIntVecNoScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.UnsignedIntTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecNoScalar;
      break;
    }
    case OCLT_GenTypeULongVecAndScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.UnsignedLongTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(6);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_GenTypeULongVecNoScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.UnsignedLongTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecNoScalar;
      break;
    }
    case OCLT_GenTypeUShortVecAndScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.UnsignedShortTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(6);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_GenTypeUShortVecAndScalarTo8: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.UnsignedShortTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalarTo8;
      break;
    }
    case OCLT_GenTypeUShortVecNoScalar: {
      SmallVector<QualType, 1> TypeList;
      TypeList.push_back(Context.UnsignedShortTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(5);
      for (unsigned I = 0; I < 5; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecNoScalar;
      break;
    }
    case OCLT_IntLongFloatGenType1: {
      SmallVector<QualType, 7> TypeList;
      TypeList.push_back(Context.IntTy);
      TypeList.push_back(Context.UnsignedIntTy);
      TypeList.push_back(Context.LongTy);
      TypeList.push_back(Context.UnsignedLongTy);
      TypeList.push_back(Context.FloatTy);
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp64")) {
        TypeList.push_back(Context.DoubleTy);
      }
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp16")) {
        TypeList.push_back(Context.HalfTy);
      }
      GenTypeNumTypes = TypeList.size();
      QT.reserve(7);
      for (unsigned I = 0; I < 1; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVec1;
      break;
    }
    case OCLT_SGenTypeN: {
      SmallVector<QualType, 4> TypeList;
      TypeList.push_back(Context.CharTy);
      TypeList.push_back(Context.ShortTy);
      TypeList.push_back(Context.IntTy);
      TypeList.push_back(Context.LongTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(24);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_UGenTypeN: {
      SmallVector<QualType, 4> TypeList;
      TypeList.push_back(Context.UnsignedCharTy);
      TypeList.push_back(Context.UnsignedShortTy);
      TypeList.push_back(Context.UnsignedIntTy);
      TypeList.push_back(Context.UnsignedLongTy);
      GenTypeNumTypes = TypeList.size();
      QT.reserve(24);
      for (unsigned I = 0; I < 6; I++) {
        QT.append(TypeList);
      }
      GenVectorSizes = ListVecAndScalar;
      break;
    }
    case OCLT_atomic_double:
      if (S.getPreprocessor().isMacroDefined("cl_khr_int64_base_atomics") && S.getPreprocessor().isMacroDefined("cl_khr_int64_extended_atomics") && S.getPreprocessor().isMacroDefined("cl_khr_fp64")) {
        QT.push_back(Context.getAtomicType(Context.DoubleTy));
      }
      break;
    case OCLT_atomic_flag:
      QT.push_back(getOpenCLTypedefType(S, "atomic_flag"));
      break;
    case OCLT_atomic_float:
      QT.push_back(Context.getAtomicType(Context.FloatTy));
      break;
    case OCLT_atomic_half:
      QT.push_back(Context.getAtomicType(Context.HalfTy));
      break;
    case OCLT_atomic_int:
      QT.push_back(Context.getAtomicType(Context.IntTy));
      break;
    case OCLT_atomic_intptr_t:
      QT.push_back(Context.getAtomicType(Context.getIntPtrType()));
      break;
    case OCLT_atomic_long:
      if (S.getPreprocessor().isMacroDefined("cl_khr_int64_base_atomics") && S.getPreprocessor().isMacroDefined("cl_khr_int64_extended_atomics")) {
        QT.push_back(Context.getAtomicType(Context.LongTy));
      }
      break;
    case OCLT_atomic_ptrdiff_t:
      QT.push_back(Context.getAtomicType(Context.getPointerDiffType()));
      break;
    case OCLT_atomic_size_t:
      QT.push_back(Context.getAtomicType(Context.getSizeType()));
      break;
    case OCLT_atomic_uint:
      QT.push_back(Context.getAtomicType(Context.UnsignedIntTy));
      break;
    case OCLT_atomic_uintptr_t:
      QT.push_back(Context.getAtomicType(Context.getUIntPtrType()));
      break;
    case OCLT_atomic_ulong:
      if (S.getPreprocessor().isMacroDefined("cl_khr_int64_base_atomics") && S.getPreprocessor().isMacroDefined("cl_khr_int64_extended_atomics")) {
        QT.push_back(Context.getAtomicType(Context.UnsignedLongTy));
      }
      break;
    case OCLT_bool:
      QT.push_back(Context.BoolTy);
      break;
    case OCLT_char:
      QT.push_back(Context.CharTy);
      break;
    case OCLT_clk_event_t:
      QT.push_back(Context.OCLClkEventTy);
      break;
    case OCLT_clk_profiling_info:
      QT.push_back(getOpenCLTypedefType(S, "clk_profiling_info"));
      break;
    case OCLT_double:
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp64")) {
        QT.push_back(Context.DoubleTy);
      }
      break;
    case OCLT_event_t:
      QT.push_back(Context.OCLEventTy);
      break;
    case OCLT_float:
      QT.push_back(Context.FloatTy);
      break;
    case OCLT_half:
      if (S.getPreprocessor().isMacroDefined("cl_khr_fp16")) {
        QT.push_back(Context.HalfTy);
      }
      break;
    case OCLT___half:
      QT.push_back(Context.HalfTy);
      break;
    case OCLT_int:
      QT.push_back(Context.IntTy);
      break;
    case OCLT_intptr_t:
      QT.push_back(Context.getIntPtrType());
      break;
    case OCLT_long:
      QT.push_back(Context.LongTy);
      break;
    case OCLT_cl_mem_fence_flags:
      QT.push_back(getOpenCLTypedefType(S, "cl_mem_fence_flags"));
      break;
    case OCLT_memory_order:
      QT.push_back(getOpenCLEnumType(S, "memory_order"));
      break;
    case OCLT_memory_scope:
      QT.push_back(getOpenCLEnumType(S, "memory_scope"));
      break;
    case OCLT_ndrange_t:
      QT.push_back(getOpenCLTypedefType(S, "ndrange_t"));
      break;
    case OCLT_ptrdiff_t:
      QT.push_back(Context.getPointerDiffType());
      break;
    case OCLT_queue_t:
      QT.push_back(Context.OCLQueueTy);
      break;
    case OCLT_reserve_id_t:
      QT.push_back(Context.OCLReserveIDTy);
      break;
    case OCLT_sampler_t:
      QT.push_back(Context.OCLSamplerTy);
      break;
    case OCLT_short:
      QT.push_back(Context.ShortTy);
      break;
    case OCLT_size_t:
      QT.push_back(Context.getSizeType());
      break;
    case OCLT_uchar:
      QT.push_back(Context.UnsignedCharTy);
      break;
    case OCLT_uint:
      QT.push_back(Context.UnsignedIntTy);
      break;
    case OCLT_uintptr_t:
      QT.push_back(Context.getUIntPtrType());
      break;
    case OCLT_ulong:
      QT.push_back(Context.UnsignedLongTy);
      break;
    case OCLT_ushort:
      QT.push_back(Context.UnsignedShortTy);
      break;
    case OCLT_void:
      QT.push_back(Context.VoidTy);
      break;
  } // end of switch (Ty.ID)

  // Construct the different vector types for each generic type.
  if (Ty.ID >= 52) {
    for (unsigned I = 0; I < QT.size(); I++) {
      // For scalars, size is 1.
      if (GenVectorSizes[I / GenTypeNumTypes] != 1) {
        QT[I] = Context.getExtVectorType(QT[I],
                          GenVectorSizes[I / GenTypeNumTypes]);
      }
    }
  }

  // Set vector size for non-generic vector types.
  if (Ty.VectorWidth > 1) {
    for (unsigned Index = 0; Index < QT.size(); Index++) {
      QT[Index] = Context.getExtVectorType(QT[Index], Ty.VectorWidth);
    }
  }

  if (Ty.IsVolatile != 0) {
    for (unsigned Index = 0; Index < QT.size(); Index++) {
      QT[Index] = Context.getVolatileType(QT[Index]);
    }
  }

  if (Ty.IsConst != 0) {
    for (unsigned Index = 0; Index < QT.size(); Index++) {
      QT[Index] = Context.getConstType(QT[Index]);
    }
  }

  // Transform the type to a pointer as the last step, if necessary.
  // Builtin functions only have pointers on [const|volatile], no
  // [const|volatile] pointers, so this is ok to do it as a last step.
  if (Ty.IsPointer != 0) {
    for (unsigned Index = 0; Index < QT.size(); Index++) {
      QT[Index] = Context.getAddrSpaceQualType(QT[Index], Ty.AS);
      QT[Index] = Context.getPointerType(QT[Index]);
    }
  }

} // OCL2Qual
