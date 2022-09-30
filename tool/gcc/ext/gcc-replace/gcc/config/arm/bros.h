#undef LINK_SPEC
#define LINK_SPEC "%{!qabs:--relocatable -d}" BPABI_LINK_SPEC

#define ADDITIONAL_DRIVER_SELF_SPECS "-B%R/bros -B%R/libc"

