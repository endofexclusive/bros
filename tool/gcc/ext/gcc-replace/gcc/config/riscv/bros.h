#define LINK_SPEC "\
-melf" XLEN_SPEC DEFAULT_ENDIAN_SPEC "riscv \
%{mno-relax:--no-relax} \
%{mbig-endian:-EB} \
%{mlittle-endian:-EL} \
%{!qabs:--relocatable -d} \
"

#define ADDITIONAL_DRIVER_SELF_SPECS "-B%R/bros -B%R/libc"

