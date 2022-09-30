#!/bin/sh
#
# SPDX-License-Identifier: GPL-2.0
# Copyright 2022 Martin Åberg
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
  puts "Usage:"
  puts " text"
  puts " troff"
}

if {[llength $argv] == 0} {
  usage
  exit 0
}

set do_text  false
set do_troff false

set argv0 [lindex $argv 0]
if {$argv0 == "text"} {
  set do_text true
} elseif {$argv0 == "troff"} {
  set do_troff true
} else {
  usage
  exit 1
}

set data [read stdin]

set script_dir [file dirname [file normalize [info script]]]
source [file join $script_dir mod.tcl]

set d [gen_dict $data]

# "my.library"
set libname [dict get $d name]
# "my"
set lprefix [dict get $d prefix]
# "my/proto.h"
set protoheader "$lprefix/proto.h"
# "my/libcall.h"
set libcallheader "$lprefix/libcall.h"
# "MyBase"
set globalbase [dict get $d globalbase]

proc ispointer {type} {
  return [expr {[string index $type end] == "*"}]
}
proc stripstar {type} {
  set type [string range $type 0 end-1]
  return [string trim $type]
}

proc genparam {type id} {
  set type [string trim $type]
  set ptr [ispointer $type]
  set thestar ""
  if {$ptr} {
    set type [stripstar $type]
    set thestar "*"
  }
  return [format "%-2s%s %s%s" "" $type $thestar $id]
}

# the parameter name is in the type
proc genfparam {type id} {
  set type [string trim $type]
  return [format "%-2s%s" "" $type]
}

proc emitproto {v {outf stdout} {prefix ""} {suffix ""} {bt ""} {ind ""}} {
  set fname [dict get $v name]
  set rett [dict get $v return]
  set pspace " "
  if {[ispointer $rett]} {
    set pspace ""
  }
  puts -nonewline $outf "${ind}${rett}${pspace}${prefix}${fname}${suffix}"
  set argz [list]
  if {[string length $bt]} {
    lappend argz "[genparam $bt lib]"
  }
  foreach pdict [dict get $v param] {
    set t [dict get $pdict type]
    set n [dict get $pdict name]
    set funcp [dict get $pdict func]
    if {$funcp} {
      lappend argz "[genfparam $t $n]"
    } else {
      lappend argz "[genparam $t $n]"
    }
  }
  if {[llength $argz]} {
    set argz [join $argz ",\n${ind}"]
    set argz "\n${ind}$argz\n${ind}"
  } else {
     set argz "void"
  }
  puts -nonewline $outf ($argz)
}

proc printlines {long {ind ""}} {
    set long [string trim $long]
    set records [split $long "\n"]
    foreach rec $records {
  puts "${ind}$rec"
    }
}

if {$do_troff} {
  puts ".de IR"
  puts {.I \\$1 \\$2 \\$3}
  puts ".."

  puts ".TL"
  puts "$libname"
  puts ".AB"
  puts "This document describes the functions in"
  puts ".I $libname ."
  puts ".AE"
  puts ".PP"
  foreach f [dict get $d funcs] {
    puts ".bp"
    puts ".SH"
    puts "NAME"
    puts ".QP"
    puts -nonewline "[dict get $f name] \\- "
    set short "FIXME"
    if {[dict exists $f short]} {
      set short [dict get $f short]
    }
    puts $short
    puts ".SH"
    puts "SYNOPSIS"
    puts ".QP"
    puts ".CW"
    puts "#include <$protoheader>"
    puts ".QP"
    puts ".CW"
    emitproto $f stdout
    puts ""

    puts ".SH"
    puts "FUNCTION"
    puts ".QP"
    if {[dict exists $f long]} {
      set long [dict get $f long]
      printlines $long
    }

    puts ".SH"
    puts "RESULTS"
    puts ".QP"
    if {[dict exists $f result]} {
      set res [dict get $f result]
    } else {
      set res "None"
    }
    printlines $res

    if {false} {
        puts ".SH"
        puts "SEE ALSO"
        puts ".QP"
        puts ".CW Read() ,"
        puts ".CW Seek() ,"
        puts ".CW Open() ,"
        puts ".CW Close()"
    }
  }
  exit 0
}

if {$do_text} {
  set ind1 "  "
  set ind2 "${ind1}${ind1}"
  puts "TABLE OF CONTENTS"
  puts ""
  if {false} {
    foreach f [dict get $d funcs] {
      set name [dict get $f name]
      puts "$libname/$name"
    }
  }
  set sep "----------------------------------------------------------------"
  set fmt " %-18s  %s"
  puts "[format $fmt Function Description]"
  puts $sep
  foreach f [dict get $d funcs] {
    set name [dict get $f name]
    set short "FIXME"
    if {[dict exists $f short]} {
      set short [dict get $f short]
    }
    puts "[format $fmt $name $short]"
  }
  puts $sep

  puts ""
  puts ""
  foreach f [dict get $d funcs] {
    set name [dict get $f name]
    puts "$libname/$name"
    puts ""
    puts "${ind1}NAME"
    puts -nonewline "${ind2}${name} - "
    set short "FIXME"
    if {[dict exists $f short]} {
      set short [dict get $f short]
    }
    printlines $short
    puts ""
    puts "${ind1}SYNOPSIS"
    emitproto $f stdout "" "" "" $ind2
    puts ""
    puts ""

    puts "${ind1}FUNCTION"
    set long FIXME
    if {[dict exists $f long]} {
      set long [dict get $f long]
    }
    printlines $long "${ind2}"
    puts ""

    puts "${ind1}RESULTS"
    if {[dict exists $f result]} {
      set res [dict get $f result]
    } else {
      set res "None"
    }
    printlines $res "${ind2}"
    puts ""

    puts "${ind1}SEE ALSO"
    set see "-"
    if {[dict exists $f see]} {
      set see [dict get $f see]
    }
    printlines $see "${ind2}"
    puts ""
    puts ""
  }
  exit 0
}

