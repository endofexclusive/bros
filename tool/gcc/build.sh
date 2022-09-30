#!/usr/bin/env bash

# SPDX-License-Identifier: GPL-2.0
# Copyright 2020-2022 Martin Åberg

set -o errexit
set -o nounset

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ASDFTOP="$( readlink -f "${SCRIPTDIR}")"

PROJNAME=bros
HOSTOS=`uname -s | tr '[:upper:]' '[:lower:]'`
HOSTDESC=$HOSTOS
pushd $ASDFTOP > /dev/null
GITDESC=`git describe --tags --dirty | cut -c 2-`
TOOLVER="0.2.11"
popd > /dev/null
NCPU=`getconf _NPROCESSORS_ONLN 2>/dev/null || getconf NPROCESSORS_ONLN 2>/dev/null || echo 1`
PROGPREFIX=${PROJNAME}-
CFGHOST=
CFGHOST="$CFGHOST --disable-nls"
CFGLANG="c,c++"

if [ $HOSTOS = freebsd ]; then
  CFGHOST="$CFGHOST --with-mpc=/usr/local"
  CFGHOST="$CFGHOST --with-isl=/usr/local"
  CFGHOST="$CFGHOST --with-gmp=/usr/local"
  CFGHOST="$CFGHOST --with-mpfr=/usr/local"
fi

if type gmake > /dev/null 2>&1 ; then
  gmake=gmake
else
  gmake=make
fi

if type md5 > /dev/null 2>&1 ; then
  md5=md5
else
  md5=md5sum
fi

build_for_one_arch() {
  THEARCH=$1
  PKGVERSION="${PROJNAME}-${THEARCH}-$TOOLVER"
  ARCHBASE=${PKGVERSION}-${HOSTDESC}
  PREFIX=$ASDFTOP/opt/${PKGVERSION}
  TARGET=${THEARCH}-all-bros
  CFGMULTI=
  if [ $THEARCH = arm ]; then
    CFGMULTI=--with-multilib-list=rmprofile
    # CFGMULTI="$CFGMULTI --disable-multilib"
    # CFGMULTI="$CFGMULTI --with-cpu=cortex-m0"
  fi
  if [ $THEARCH = avr ]; then
    CFGMULTI="$CFGMULTI --with-avrlibc=no"
  fi

  CFGDEF=
  CFGDEF="$CFGDEF --target=$TARGET"
  CFGDEF="$CFGDEF --prefix=$PREFIX"
  CFGDEF="$CFGDEF --with-sysroot=$PREFIX/sdk"
  CFGDEF="$CFGDEF --with-build-sysroot=${ASDFTOP}/build-${THEARCH}-sdk/dist"
  CFGDEF="$CFGDEF --with-pkgversion=$PKGVERSION"
  CFGDEF="$CFGDEF --docdir=$PREFIX/doc"
  CFGDEF="$CFGDEF --disable-shared"
  # CFGDEF="$CFGDEF --enable-shared"
  # CFGDEF="$CFGDEF --disable-plugin"

  prevPATH=${PATH}
  PATH="${PREFIX}/bin/:${PATH}"
  # export PATH

  log=log-${THEARCH}
  mkdir -p $log

  if [ $do_pre -ne 0 ]; then
    pre > >(tee $log/pre.stdout) 2> >(tee $log/pre.stderr >&2)
  fi

  if [ $do_binutils -ne 0 ]; then
    build_binutils > >(tee $log/binutils.stdout) 2> >(tee $log/binutils.stderr >&2)
  fi

  if [ $do_gcc1 -ne 0 ]; then
    build_gcc1 > >(tee $log/gcc1.stdout) 2> >(tee $log/gcc1.stderr >&2)
  fi

  if [ $do_gcc2 -ne 0 ]; then
    build_gcc2 > >(tee $log/gcc2.stdout) 2> >(tee $log/gcc2.stderr >&2)
  fi

  if [ $do_dist -ne 0 ]; then
    dist > >(tee $log/dist.stdout) 2> >(tee $log/dist.stderr >&2)
  fi

  tar czf $ASDFTOP/$ARCHBASE.log.tar.gz -C $ASDFTOP $log
  PATH=$prevPATH
}

verb() {
  echo VERB "$@"
  eval "$@"
}

genlinks() {
  verb pushd $PREFIX/bin
  for file in $TARGET-*; do
    verb ln -s -f $file `echo $file | sed s/${TARGET}-/${PROGPREFIX}/g`
  done
  verb popd
}

# Bootstrap prefix directory
pre() {
  verb $gmake -C $ASDFTOP/ext clean extdep
}

build_binutils() {
  verb mkdir -p $ASDFTOP/build-${THEARCH}-binutils

  verb pushd $ASDFTOP/build-${THEARCH}-binutils
  verb $ASDFTOP/ext/binutils/configure \
    $CFGDEF \
    $CFGHOST \
    --disable-gdb \
    --disable-sim
  verb $gmake -j $NCPU all
  verb $gmake install-strip
  if [ $op_pdf -ne 0 ]; then
    verb $gmake -j $NCPU pdf
    verb $gmake install-pdf
  fi
  verb popd
  verb genlinks
}

build_gcc1() {
  verb mkdir -p $ASDFTOP/build-${THEARCH}-gcc

  verb pushd $ASDFTOP/build-${THEARCH}-gcc

  verb mkdir -p ${ASDFTOP}/build-${THEARCH}-sdk/dist
  verb $ASDFTOP/ext/gcc/configure \
    $CFGDEF \
    $CFGHOST \
    $CFGMULTI \
    --enable-languages=$CFGLANG \
    --enable-lto \
    --enable-threads=single \
    --enable-cstdio=stdio_pure

# --enable-libstdcxx-threads=no
# --disable-hosted-libstdcxx
# --enable-libstdcxx-threads=no
# --enable-libstdcxx-filesystem-ts=no
# --disable-libstdcxx

  verb mkdir -p $PREFIX/sdk
  verb $gmake -j $NCPU all-gcc
  verb $gmake -j $NCPU all-target-libgcc
  verb $gmake install-strip-gcc
  verb $gmake install-target-libgcc
  if [ $op_pdf -ne 0 ]; then
    verb $gmake -j $NCPU pdf-gcc
    verb $gmake install-pdf-gcc
  fi
  verb popd
  verb genlinks
}

build_gcc2() {
  verb pushd $ASDFTOP/build-${THEARCH}-gcc
  if [ $op_cclib -ne 0 ]; then
    # Prepare the "build-sysroot" stuff needed for libstdc++
    verb ${ASDFTOP}/../../scripts/sdk.tcl \
      --host=${THEARCH}-all-bros \
      --prefix=${ASDFTOP}/build-${THEARCH}-sdk
    verb $gmake -j $NCPU all-target-libstdc++-v3
    verb $gmake install-target-libstdc++-v3
  fi
  verb popd
}

dist() {
  verb pushd $PREFIX
  verb popd

  verb cp $ASDFTOP/README $PREFIX/

  archname=${ARCHBASE}.tar.gz

  verb "rm -f $PREFIX/sdk/bros"
  verb "rm -f $PREFIX/sdk/libc"
  verb tar czf $ASDFTOP/$archname -C $ASDFTOP/opt ${PKGVERSION}
  verb pushd $ASDFTOP
  verb "$md5 $archname > $ASDFTOP/$archname.md5"
  verb popd
  # Link in the sdk directory of the BROS source root
  verb "ln -s -f ../../../../../sdk/${THEARCH}/dist/bros $PREFIX/sdk/bros"
  verb "ln -s -f ../../../../../sdk/${THEARCH}/dist/libc $PREFIX/sdk/libc"
}

usage() {
  echo "Usage: $0 [OPTION]... WHAT... ARCH..."
  echo ""
  echo "OPTION:"
  echo "  --nopdf     - Do not generate PDF files"
  echo "WHAT:"
  echo "  binutils    - Assembler and tools"
  echo "  gcc1        - Cross compiler"
  echo "  gcc2        - GCC target (libgcc)"
  echo "  dist        - Create binary distribution archive"
  echo "  all         - all above except dep"
  echo "ARCH:"
  echo "  aarch64"
  echo "  arm"
  echo "  avr"
  echo "  riscv"
  echo "  sparc"
}

echo "HOSTDESC: $HOSTDESC"
echo "TOOLVER:  $TOOLVER"
echo "GITDESC:  $GITDESC"
if [ "$#" -lt 1 ]; then
  usage
  exit 0
fi

do_pre=0
do_binutils=0
do_gcc1=0
do_gcc2=0
do_dist=0
do_usage=0
op_pdf=1
op_cclib=0
op_extra=0
do_arch_aarch64=0
do_arch_arm=0
do_arch_avr=0
do_arch_m68k=0
do_arch_riscv=0
do_arch_sparc=0

while [ "$#" -gt 0 ]; do
  key=$1
  case $key in
    --nopdf)    op_pdf=0;;
    --cclib)    op_cclib=1;;
    --extra)    op_extra="$2"; shift;;
    pre)        do_pre=1;;
    binutils)   do_binutils=1;;
    gcc1)       do_gcc1=1;;
    gcc2)       do_gcc2=1;;
    dist)       do_dist=1;;
    aarch64)    do_arch_aarch64=1;;
    arm)  do_arch_arm=1;;
    avr)  do_arch_avr=1;;
    m68k)       do_arch_m68k=1;;
    riscv)      do_arch_riscv=1;;
    sparc)      do_arch_sparc=1;;
    all)
      do_binutils=1
      do_gcc1=1
      do_gcc2=1
      do_dist=1
      ;;
    *)
      echo "unknown argument $1"
      exit 1
      break
      ;;
  esac
  shift
done

if [ $do_arch_aarch64 -ne 0 ]; then
  build_for_one_arch aarch64
fi
if [ $do_arch_arm -ne 0 ]; then
  build_for_one_arch arm
fi
if [ $do_arch_avr -ne 0 ]; then
  build_for_one_arch avr
fi
if [ $do_arch_m68k -ne 0 ]; then
  build_for_one_arch m68k
fi
if [ $do_arch_riscv -ne 0 ]; then
  build_for_one_arch riscv
fi
if [ $do_arch_sparc -ne 0 ]; then
  build_for_one_arch sparc
fi

echo DONE "($0)"

