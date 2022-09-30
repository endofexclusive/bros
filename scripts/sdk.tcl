#!/bin/sh
#
# SPDX-License-Identifier: GPL-2.0
# Copyright 2020-2022 Martin Åberg
#
# \
if type tclsh > /dev/null 2>&1 ; then exec tclsh "$0" ${1+"$@"} ; fi
# \
if type tclsh8.6 > /dev/null 2>&1 ; then exec tclsh8.6 "$0" ${1+"$@"} ; fi
# \
if type tclsh8.5 > /dev/null 2>&1 ; then exec tclsh8.5 "$0" ${1+"$@"} ; fi
# \
echo "$0: tclsh required" ; \
exit 1

proc usage {} {
  global argv0
  puts stderr "usage: $argv0 \[OPTION\]..."
  puts stderr "  --host=HOST      cross compile for HOST"
  puts stderr "  --prefix=PREFIX  install files in PREFIX"
  puts stderr "  --gen            generate OS headers and stub source"
  puts stderr "  --miss           build GCC support library (memset)"
  puts stderr "  --stub           build OS stubs"
  puts stderr "  --libc           build libc"
}

set sdk_prefix "sdkx"
set sdk_host ""
set do_gen  false
set do_stub false
set do_miss false
set do_libc false
set do_all true
foreach arg $argv {
  switch -glob -- $arg {
    --host=*    {set sdk_host   [lindex [split $arg =] 1]}
    --prefix=*  {set sdk_prefix [lindex [split $arg =] 1]}
    --gen       {set do_all false; set do_gen  true}
    --miss      {set do_all false; set do_miss true}
    --stub      {set do_all false; set do_stub true}
    --libc      {set do_all false; set do_libc true}
    default {
      puts stderr "$argv0: illegal option $arg"
      usage
      exit 1
    }
  }
}
if {$do_all} {
  set do_gen  true
  set do_stub true
  set do_miss true
#  set do_libc true
}

puts "host:   $sdk_host"
puts "prefix: $sdk_prefix"

set script_dir [file dirname [file normalize [info script]]]
set root_dir [file join $script_dir ..]
set miss_dir [file join $root_dir libmiss]
set libc_dir [file join $root_dir libc]
set sdk_norm [file normalize $sdk_prefix]
set gmake [exec [file join $script_dir gmake.sh]]

set moddirs [list]
lappend moddirs mod/exec
lappend moddirs mod/expansion
lappend moddirs mod/devices

set genheader [file join $script_dir genheader.tcl]
set ddir [file join $sdk_prefix dist bros include]
file mkdir $sdk_prefix
file mkdir $ddir

# NOTE: "dir" shall NOT be used in file names or data. Use mprefix instead.
foreach dir $moddirs {
  if {!$do_gen} {
    continue
  }
  set pubdir [file join $root_dir $dir pub]
  foreach file [glob -nocomplain -directory $pubdir *] {
    exec cp -rv $file $ddir
  }

  set funcfile [file join $root_dir $dir func.tcl]
  if {![file exists $funcfile]} {
    continue
  }
  set mprefix [exec grep -o mod_prefix.* $funcfile]
  set mprefix [lindex $mprefix 1]
  set incdir [file join $ddir $mprefix]
  file mkdir $incdir

  set fn [file join $incdir op.h]
  exec $genheader op < $funcfile > $fn
  set fn [file join $incdir libcall.h]
  exec $genheader libcall < $funcfile > $fn
  set fn [file join $incdir offset.h]
  exec $genheader offset < $funcfile > $fn
  set fn [file join $incdir proto.h]
  exec $genheader proto < $funcfile > $fn
  set fn [file join $incdir inline.h]
  exec $genheader inline < $funcfile > $fn

  set sdir [file join $sdk_prefix src $mprefix]
  file mkdir $sdir
  exec $genheader stub < $funcfile --dest $sdir
}

set the_makefile [file join $root_dir mk Makefile-for-stub]
set mkhost $sdk_host
if {[string length $mkhost]} {
  set mkhost [string cat $mkhost -]
}

# GCC centric
set multis [exec ${mkhost}gcc --print-multi-lib]
# set multis [list]
if {[llength $multis] == 0} {
  set multis [list . ""]
}

# NOTE: nstart belongs to SDK and ustart belongs to libc.

foreach multi $multis {
  set val [split $multi \;]
  set mdir [lindex $val 0]
  set opt [lindex $val 1]
  set mopt [string map {@ " -"} $opt]
  set mopt [string trim $mopt]

  puts [format "  %-32s %s" $mdir $mopt]

  if {$do_stub} {
    set stub_build_dir [file join $sdk_norm build-stub $mdir]
    set stub_dist_dir [file join $sdk_norm dist bros $mdir]
    exec $gmake -C $sdk_prefix \
      -f $the_makefile \
      HOST=$mkhost \
      BUILD=$stub_build_dir \
      DIST=$stub_dist_dir \
      MULTIDIR=$mdir \
      MULTIFLAGS=$mopt \
      -j 12 \
      all install \
      >& [file join $sdk_prefix log-stub-$opt]
  }

  if {$do_miss} {
    set miss_build_dir [file join $sdk_norm build-miss $mdir]
    set miss_dist_dir [file join $sdk_norm dist bros $mdir]

    exec $gmake -C $miss_dir \
      HOST=$mkhost \
      BUILD=$miss_build_dir \
      DIST=$miss_dist_dir \
      MULTIDIR=$mdir \
      MULTIFLAGS=$mopt \
      -j 12 \
      all install \
      >& [file join $sdk_prefix log-miss-$opt]
  }

  if {$do_libc} {
    set libc_build_dir [file join $sdk_norm build-libc $mdir]
    set libc_dist_dir [file join $sdk_norm dist libc $mdir]
    set sdk_inc_dir [file join $sdk_norm dist bros include]
    set install-headers "foo=bar"
    if {$mdir == "."} {
      set install-headers "install-headers"
    }
    exec $gmake -C $libc_dir \
      HOST=$mkhost \
      BUILD=$libc_build_dir \
      DIST=$libc_dist_dir \
      MULTIDIR=$mdir \
      MULTIFLAGS=$mopt \
      SDKINCDIR=$sdk_inc_dir \
      -j 12 \
      all install ${install-headers} \
      >& [file join $sdk_prefix log-libc-$opt]
    exec ${mkhost}ar rv -c \
      [file join $libc_dist_dir libm.a]
  }
}

