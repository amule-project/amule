# DownloadBandwidthThrottler holds an std::atomic<int64_t> for the
# shared byte budget. On 64-bit targets the compiler emits native
# CAS / load / store; on 32-bit targets without a hardware 8-byte
# atomic (PPC32, ARMv5/v6, MIPS32, some old x86 toolchains) the
# atomic operations expand to __atomic_{load,store,compare_exchange,
# fetch_add}_8 calls that live in libatomic. Without -latomic on
# the link line the build dies at link time with
# "Undefined symbols: ___atomic_compare_exchange_8" etc.
#
# Try compiling + linking a tiny std::atomic<int64_t> probe twice:
# first bare, then with -latomic on the link line. If the bare
# build links, leave LIBATOMIC empty (the most common 64-bit case
# is a no-op). If only the -latomic version links, export atomic
# in LIBATOMIC for the muleappcore target to inherit publicly.

include (CheckCXXSourceCompiles)
include (CMakePushCheckState)

set (_atomic_probe "
#include <atomic>
#include <cstdint>
int main() {
    std::atomic<int64_t> x{0};
    x.store(1);
    int64_t v = x.load();
    x.fetch_add(1);
    int64_t expected = 2;
    x.compare_exchange_strong(expected, 3);
    return (int)v;
}
")

cmake_push_check_state (RESET)
check_cxx_source_compiles ("${_atomic_probe}" amule_HAVE_NATIVE_ATOMIC64)

set (LIBATOMIC "")
if (NOT amule_HAVE_NATIVE_ATOMIC64)
	set (CMAKE_REQUIRED_LIBRARIES atomic)
	check_cxx_source_compiles ("${_atomic_probe}" amule_HAVE_LIBATOMIC_ATOMIC64)
	if (amule_HAVE_LIBATOMIC_ATOMIC64)
		set (LIBATOMIC atomic)
		message (STATUS "std::atomic<int64_t> needs -latomic (32-bit target)")
	else()
		message (FATAL_ERROR
			"std::atomic<int64_t> doesn't link bare or against "
			"libatomic on this target. aMule's download bandwidth "
			"throttler relies on a lock-free 64-bit atomic byte "
			"budget; without libatomic the link fails with "
			"\"Undefined symbols: ___atomic_*_8\". Install the "
			"toolchain's libatomic runtime (libatomic1-dev on "
			"Debian/Ubuntu/Mint, libatomic on Fedora/RHEL/Rocky/Arch, "
			"part of libgcc on most others) and re-run cmake.")
	endif()
endif()
cmake_pop_check_state()
unset (_atomic_probe)
