cmd_/home/build/lustre-release/build/conftest.o := gcc -Wp,-MD,/home/build/lustre-release/build/.conftest.o.d  -nostdinc -isystem /usr/lib/gcc/x86_64-redhat-linux/4.4.7/include -I/home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include -I/home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include -Iinclude -Iarch/x86/include/generated -I/home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include -Iinclude2 -I/home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/uapi -Iinclude/generated -I/home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/uapi -Iarch/x86/include/generated/uapi -I/home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/uapi -Iinclude/generated/uapi     -include /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/kconfig.h -D__KERNEL__ -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -m64 -mtune=generic -mno-red-zone -mcmodel=kernel -funit-at-a-time -maccumulate-outgoing-args -fstack-protector -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1 -DCONFIG_AS_CFI_SECTIONS=1 -DCONFIG_AS_AVX=1 -pipe -Wno-sign-compare -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -Wframe-larger-than=2048 -Wno-unused-but-set-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -pg -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fno-dwarf2-cfi-asm -fconserve-stack -Werror-implicit-function-declaration -g -I/home/build/lustre-release/libcfs/include -I/home/build/lustre-release/lnet/include -I/home/build/lustre-release/lustre/include  -DMODULE -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(conftest)"  -D"KBUILD_MODNAME=KBUILD_STR(conftest)" -D"DEBUG_HASH=61" -D"DEBUG_HASH2=33" -c -o /home/build/lustre-release/build/.tmp_conftest.o /home/build/lustre-release/build/conftest.c

deps_/home/build/lustre-release/build/conftest.o := \
  /home/build/lustre-release/build/conftest.c \
    $(wildcard include/config/quota.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/kconfig.h \
    $(wildcard include/config/h.h) \
    $(wildcard include/config/.h) \
    $(wildcard include/config/booger.h) \
    $(wildcard include/config/foo.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/kernel.h \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/spinlock/sleep.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/dynamic/debug.h) \
    $(wildcard include/config/ring/buffer.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/numa.h) \
    $(wildcard include/config/compaction.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /usr/lib/gcc/x86_64-redhat-linux/4.4.7/include/stdarg.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/linkage.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/compiler-gcc4.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/linkage.h \
    $(wildcard include/config/x86/32.h) \
    $(wildcard include/config/x86/64.h) \
    $(wildcard include/config/x86/alignment/16.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/stringify.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/stddef.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/types.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/asm-generic/types.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/asm-generic/int-ll64.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/bitsperlong.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/asm-generic/bitsperlong.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/posix_types.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/posix_types.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/posix_types_64.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/bitops.h \
    $(wildcard include/config/generic/find/first/bit.h) \
    $(wildcard include/config/generic/find/last/bit.h) \
    $(wildcard include/config/generic/find/next/bit.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/bitops.h \
    $(wildcard include/config/x86/cmov.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/alternative.h \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/paravirt.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/asm.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/cpufeature.h \
    $(wildcard include/config/x86/invlpg.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/required-features.h \
    $(wildcard include/config/x86/minimum/cpu/family.h) \
    $(wildcard include/config/math/emulation.h) \
    $(wildcard include/config/x86/pae.h) \
    $(wildcard include/config/x86/cmpxchg64.h) \
    $(wildcard include/config/x86/use/3dnow.h) \
    $(wildcard include/config/x86/p6/nop.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/asm-generic/bitops/sched.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/asm-generic/bitops/hweight.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/asm-generic/bitops/fls64.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/asm-generic/bitops/ext2-non-atomic.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/asm-generic/bitops/le.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/byteorder.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/byteorder/little_endian.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/swab.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/swab.h \
    $(wildcard include/config/x86/bswap.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/byteorder/generic.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/asm-generic/bitops/minix.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/typecheck.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/ratelimit.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/param.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/param.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/asm-generic/param.h \
    $(wildcard include/config/hz.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/dynamic_debug.h \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/linux/init.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/hotplug.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/arch/x86/include/asm/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/debug/bugverbose.h) \
  /home/build/kernel/rpmbuild/BUILD/kernel-2.6.32.431.29.2.el6_lustre/include/asm-generic/bug.h \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/generic/bug/relative/pointers.h) \

/home/build/lustre-release/build/conftest.o: $(deps_/home/build/lustre-release/build/conftest.o)

$(deps_/home/build/lustre-release/build/conftest.o):
