#!/usr/bin/env python3
"""Fill LLVM's and Clang's CMake configuration templates for openkal (x86_64, musl, libc++).

What CMake's configure step would write, written once here instead: the templates are upstream's
own (`llvm/**/Config/*.cmake`, `*.def.in`, `Version.inc.in`), the answers are this port's, and the
output goes to `llvm-generated/include`, which comes before the upstream include directories.

    tools/gen_config.py            regenerate llvm-generated/include/{llvm,clang}/...

Every HAVE_* answer below was checked against openkal-musl 0.19.2 (the C library a program built
on openkal links): its sources, its port layer and its `[c-abi-absent]` table. An answer that was
not checked is the conservative one (0), so a missing facility is never assumed.
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
UP = ROOT / "llvm"
OUT = ROOT / "llvm-generated" / "include"

VERSION = (23, 1, 0)
TRIPLE = "x86_64-unknown-linux-musl"

values = {
    # Version and identity.
    "LLVM_VERSION_MAJOR": VERSION[0], "LLVM_VERSION_MINOR": VERSION[1], "LLVM_VERSION_PATCH": VERSION[2],
    "PACKAGE_VERSION": "%d.%d.%d" % VERSION, "PACKAGE_NAME": "LLVM", "PACKAGE_STRING": "LLVM %d.%d.%d" % VERSION,
    "PACKAGE_BUGREPORT": "https://github.com/llvm/llvm-project/issues/", "PACKAGE_VENDOR": "",
    "BUG_REPORT_URL": "https://github.com/llvm/llvm-project/issues/",
    "LLVM_DEFAULT_TARGET_TRIPLE": TRIPLE, "LLVM_HOST_TRIPLE": TRIPLE, "LLVM_TARGET_TRIPLE_ENV": "",
    "LLVM_NATIVE_ARCH": "X86", "LLVM_ON_UNIX": 1, "LLVM_PLUGIN_EXT": ".so", "LTDL_SHLIB_EXT": ".so",
    "HOST_LINK_VERSION": "", "LLVM_GISEL_COV_PREFIX": "",
    # Features this port turns off: no compression libraries, no network, no JIT helpers.
    "LLVM_ENABLE_THREADS": 1, "LLVM_HAS_ATOMICS": 1, "LLVM_ENABLE_ZLIB": 0, "LLVM_ENABLE_ZSTD": 0,
    "LLVM_ENABLE_LIBXML2": 0, "LLVM_ENABLE_CURL": 0, "LLVM_ENABLE_HTTPLIB": 0, "LLVM_WITH_Z3": 0,
    "LLVM_ENABLE_ABI_BREAKING_CHECKS": 0, "LLVM_ENABLE_REVERSE_ITERATION": 0, "LLVM_FORCE_ENABLE_STATS": 0,
    "LLVM_ENABLE_CRASH_DUMPS": 0, "LLVM_ENABLE_DIA_SDK": 0, "LLVM_ENABLE_TELEMETRY": 0, "LLVM_ENABLE_ONDISK_CAS": 0,
    "LLVM_ENABLE_IO_SANDBOX": 0, "LLVM_ENABLE_DEBUGLOC_TRACKING_COVERAGE": 0, "LLVM_ENABLE_DEBUGLOC_TRACKING_ORIGIN": 0,
    "LLVM_UNREACHABLE_OPTIMIZE": 1, "LLVM_USE_INTEL_JITEVENTS": 0, "LLVM_USE_OPROFILE": 0, "LLVM_USE_PERF": 0,
    "LLVM_SUPPORT_XCODE_SIGNPOSTS": 0, "LLVM_WINDOWS_PREFER_FORWARD_SLASH": 0, "LLVM_GISEL_COV_ENABLED": 0,
    "LLVM_VERSION_PRINTER_SHOW_BUILD_CONFIG": 0, "LLVM_VERSION_PRINTER_SHOW_HOST_TARGET_INFO": 1,
    "LLVM_FORCE_USE_OLD_TOOLCHAIN": 0, "ENABLE_BACKTRACES": 0, "ENABLE_CRASH_OVERRIDES": 0,
    "HAVE_CRASHREPORTER_INFO": 0, "HAVE_CRASHREPORTERCLIENT_H": 0,
    # The C library (openkal-musl 0.19.2).
    "HAVE_BACKTRACE": 0, "BACKTRACE_HEADER": "execinfo.h",   # no <execinfo.h> in musl
    "HAVE_DLOPEN": 0,                                        # static programs: dlopen is a stub that always fails
    "HAVE_REGISTER_FRAME": 0, "HAVE_DEREGISTER_FRAME": 0, "HAVE_UNW_ADD_DYNAMIC_FDE": 0,
    "HAVE__UNWIND_BACKTRACE": 1,                             # libunwind (openkal-llvm-runtime)
    "HAVE_FFI_CALL": 0, "HAVE_FFI_FFI_H": 0, "HAVE_FFI_H": 0,
    "HAVE_FUTIMENS": 1, "HAVE_FUTIMES": 1, "HAVE_GETPAGESIZE": 1, "HAVE_GETRUSAGE": 1, "HAVE_GETAUXVAL": 1,
    "HAVE_LIBEDIT": 0, "HAVE_LIBPFM": 0, "LIBPFM_HAS_FIELD_CYCLES": 0, "HAVE_LIBPSAPI": 0, "HAVE_LIBPTHREAD": 1,
    "HAVE_PTHREAD_GETNAME_NP": 1, "HAVE_PTHREAD_SETNAME_NP": 1, "HAVE_PTHREAD_GET_NAME_NP": 0, "HAVE_PTHREAD_SET_NAME_NP": 0,
    "HAVE_MACH_MACH_H": 0, "HAVE_MALLCTL": 0, "HAVE_MALLINFO": 0, "HAVE_MALLINFO2": 0, "HAVE_MALLOC_MALLOC_H": 0,
    "HAVE_MALLOC_ZONE_STATISTICS": 0,
    "HAVE_POSIX_SPAWN": 1,                                   # port/src/okm_spawn.c
    "HAVE_PREAD": 0,                                         # no pread64 in openkal-musl's dispatcher (ENOSYS)
    "HAVE_PTHREAD_H": 1, "HAVE_PTHREAD_MUTEX_LOCK": 1, "HAVE_PTHREAD_RWLOCK_INIT": 1,
    "HAVE_SBRK": 0, "HAVE_SETENV": 1, "HAVE_SIGALTSTACK": 1, "HAVE_STRERROR_R": 1, "HAVE_SYSCONF": 1,
    "HAVE_SYS_MMAN_H": 1, "HAVE_SYS_IOCTL_H": 1, "HAVE_SYSEXITS_H": 1, "HAVE_UNISTD_H": 1,
    "HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC": 0, "HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC": 1,
    "HAVE_VALGRIND_VALGRIND_H": 0, "HAVE__CHSIZE_S": 0, "HAVE_BUILTIN_THREAD_POINTER": 1,
    "HAVE_DECL_ARC4RANDOM": 0, "HAVE_DECL_FE_ALL_EXCEPT": 1, "HAVE_DECL_FE_INEXACT": 1, "HAVE_DECL_STRERROR_S": 0,
    "HAVE_ICONV": 0, "HAVE_ICU": 0, "HAVE_WINDOWS_ICU": 0,
    "HAVE_ISATTY": 1, "HAVE_PROC_PID_RUSAGE": 0,
    "strdup": None, "stricmp": None,   # Windows-only spellings
    # Static libraries, no plugins; no native code generation target in this package.
    "LLVM_BUILD_LLVM_DYLIB": 0, "LLVM_BUILD_SHARED_LIBS": 0, "LLVM_ENABLE_DUMP": 0, "LLVM_ENABLE_PLUGINS": 0,
    "LLVM_ENABLE_LLVM_C_EXPORT_ANNOTATIONS": 0, "LLVM_ENABLE_LLVM_EXPORT_ANNOTATIONS": 0, "LLVM_ENABLE_PROFCHECK": 0,
    "LLVM_HAS_LOGF128": 0, "LLVM_HAVE_TFLITE": 0,
    **{k: 0 for k in ["LLVM_NATIVE_ASMPARSER", "LLVM_NATIVE_ASMPRINTER", "LLVM_NATIVE_DISASSEMBLER", "LLVM_NATIVE_TARGET",
                      "LLVM_NATIVE_TARGETINFO", "LLVM_NATIVE_TARGETMC", "LLVM_NATIVE_TARGETMCA"]},
    # Compiler-runtime probes used only by the JIT: none.
    **{k: 0 for k in ["HAVE__ALLOCA", "HAVE___ALLOCA", "HAVE___ASHLDI3", "HAVE___ASHRDI3", "HAVE____CHKSTK",
                      "HAVE___CHKSTK", "HAVE____CHKSTK_MS", "HAVE___CHKSTK_MS", "HAVE___CMPDI2", "HAVE___DIVDI3",
                      "HAVE___FIXDFDI", "HAVE___FIXSFDI", "HAVE___FLOATDIDF", "HAVE___LSHRDI3", "HAVE___MAIN",
                      "HAVE___MODDI3", "HAVE___UDIVDI3", "HAVE___UMODDI3"]},
    # No LLVM code generation targets in this package: the frontend libraries need none.
    **{"LLVM_HAS_%s_TARGET" % t: 0 for t in ["AARCH64", "AMDGPU", "ARC", "ARM", "AVR", "BPF", "CSKY", "DIRECTX",
                                               "HEXAGON", "LANAI", "LOONGARCH", "M68K", "MIPS", "MSP430", "NVPTX",
                                               "POWERPC", "RISCV", "SPARC", "SPIRV", "SYSTEMZ", "VE", "WEBASSEMBLY",
                                               "X86", "XCORE", "XTENSA"]},
    "LLVM_ENUM_TARGETS": "", "LLVM_ENUM_ASM_PRINTERS": "", "LLVM_ENUM_ASM_PARSERS": "", "LLVM_ENUM_DISASSEMBLERS": "",
    "LLVM_ENUM_TARGETMCAS": "", "LLVM_ENUM_EXEGESIS": "",
    "PPC_LINUX_DEFAULT_IEEELONGDOUBLE": 0, "ENABLE_X86_RELAX_RELOCATIONS": 1, "ENABLE_LINKER_BUILD_ID": 0,
    # Clang.
    "CLANG_VERSION": "%d.%d.%d" % VERSION, "CLANG_VERSION_MAJOR": VERSION[0], "CLANG_VERSION_MINOR": VERSION[1],
    "CLANG_VERSION_PATCHLEVEL": VERSION[2], "MAX_CLANG_ABI_COMPAT_VERSION": VERSION[0],
    "CLANG_DEFAULT_PIE_ON_LINUX": 1, "CLANG_ENABLE_CIR": 0, "CLANG_ENABLE_OBJC_REWRITER": 0,
    "CLANG_ENABLE_STATIC_ANALYZER": 0, "CLANG_SPAWN_CC1": 0, "CLANG_USE_EXPERIMENTAL_CONST_INTERP": 0,
    "CLANG_DEFAULT_CXX_STDLIB": "", "CLANG_DEFAULT_LINKER": "", "CLANG_DEFAULT_OBJCOPY": "objcopy",
    "CLANG_DEFAULT_OPENMP_RUNTIME": "libomp", "CLANG_DEFAULT_RTLIB": "", "CLANG_DEFAULT_UNWINDLIB": "",
    "CLANG_INSTALL_LIBDIR_BASENAME": "lib", "CLANG_RESOURCE_DIR": "", "CLANG_SYSTEMZ_DEFAULT_ARCH": "z10",
    "C_INCLUDE_DIRS": "", "DEFAULT_SYSROOT": "", "GCC_INSTALL_PREFIX": "",
    "CLANG_HAVE_DLADDR": 0, "CLANG_HAVE_DLFCN_H": 1, "CLANG_HAVE_LIBXML": 0, "CLANG_HAVE_RLIMITS": 1,
    "CLANG_CONFIG_FILE_SYSTEM_DIR": None, "CLANG_CONFIG_FILE_USER_DIR": None, "CLANG_USE_XCSELECT": None,
    "CLANG_XCSELECT_HOST_SDK_POLICY": None,
}


def render(text: str, path: str) -> str:
    missing = set()

    def value(name):
        if name not in values:
            missing.add(name)
            return None
        return values[name]

    def subst(s):
        s = re.sub(r"\$\{(\w+)\}", lambda m: "" if value(m.group(1)) is None else str(value(m.group(1))), s)
        return re.sub(r"@(\w+)@", lambda m: "" if value(m.group(1)) is None else str(value(m.group(1))), s)

    out = []
    for line in text.splitlines():
        m = re.match(r"^#cmakedefine01\s+(\w+)\s*$", line)
        if m:
            v = value(m.group(1))
            out.append("#define %s %d" % (m.group(1), 1 if v else 0))
            continue
        m = re.match(r"^#cmakedefine\s+(\w+)(.*)$", line)
        if m:
            v = value(m.group(1))
            if v in (None, 0, "", False):
                out.append("/* #undef %s */" % m.group(1))
            else:
                rest = subst(m.group(2))
                out.append("#define %s%s" % (m.group(1), rest if rest.strip() else ""))
            continue
        out.append(subst(line))
    if missing:
        sys.exit("%s: no answer for %s" % (path, ", ".join(sorted(missing))))
    return "\n".join(out) + "\n"


def emit(template: pathlib.Path, output: pathlib.Path):
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(render(template.read_text(), str(template.relative_to(ROOT))))
    print("wrote", output.relative_to(ROOT))


cfg = UP / "llvm/include/llvm/Config"
for name in ["config.h", "llvm-config.h", "abi-breaking.h", "Targets.h"]:
    emit(cfg / (name + ".cmake"), OUT / "llvm/Config" / name)
for name in ["Targets.def", "AsmPrinters.def", "AsmParsers.def", "Disassemblers.def", "TargetMCAs.def", "TargetExegesis.def"]:
    emit(cfg / (name + ".in"), OUT / "llvm/Config" / name)
emit(UP / "clang/include/clang/Config/config.h.cmake", OUT / "clang/Config/config.h")
emit(UP / "clang/include/clang/Basic/Version.inc.in", OUT / "clang/Basic/Version.inc")

rev = (ROOT / "UPSTREAM-REV").read_text().split()[0] if (ROOT / "UPSTREAM-REV").exists() else "llvmorg-%d.%d.%d" % VERSION
(OUT / "llvm/Support").mkdir(parents=True, exist_ok=True)
(OUT / "llvm/Support/VCSRevision.h").write_text('#define LLVM_REVISION "%s"\n#define LLVM_REPOSITORY "https://github.com/llvm/llvm-project"\n' % rev)
(OUT / "llvm/Support/Extension.def").write_text("// No statically registered extensions.\n#undef HANDLE_EXTENSION\n")
(ROOT / "llvm-generated/clang-lib/Basic").mkdir(parents=True, exist_ok=True)
(ROOT / "llvm-generated/clang-lib/Basic/VCSVersion.inc").write_text('#define LLVM_REVISION "%s"\n#define LLVM_REPOSITORY "https://github.com/llvm/llvm-project"\n#define CLANG_REVISION "%s"\n#define CLANG_REPOSITORY "https://github.com/llvm/llvm-project"\n' % (rev, rev))
print("wrote VCSRevision.h, Extension.def, VCSVersion.inc")
