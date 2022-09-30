#undef LINK_SPEC
#define LINK_SPEC "%{h*}			\
   %{!qabs:--relocatable -d}			\
   %{static:-Bstatic}				\
   %{shared:-shared}				\
   %{symbolic:-Bsymbolic}			\
   %{!static:%{rdynamic:-export-dynamic}}	\
   %{mbig-endian:-EB} %{mlittle-endian:-EL} -X	\
  -maarch64elf%{mabi=ilp32*:32}%{mbig-endian:b}" \
  AARCH64_ERRATA_LINK_SPEC

#define ADDITIONAL_DRIVER_SELF_SPECS "-B%R/bros -B%R/libc"

