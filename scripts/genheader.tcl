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
  puts "Usage: $argv0 command"
  puts ""
  puts "This script reads an API specification from standard input and"
  puts "generates outputs useful for application and library"
  puts "implementation. The output depends on the command."
  puts ""
  puts " Command        Output"
  puts "--------------------------------------------------------------------------------"
  puts " proto          size_t AvailMem(int attr);"
  puts " libcall        size_t lAvailMem(struct ExecBase *lib, int attr);"
  puts " stub \[--dest\]  size_t AvailMem(int attr) \{ .. \}"
  puts "                size_t lAvailMem(struct ExecBase *lib, int attr) \{ .. \}"
  puts " inline         static inline size_t lAvailMem(ExecBase *lib, int attr) \{ .. \}"
  puts ""
  puts " internal       size_t iAvailMem(struct ExecBase *lib, int attr);"
  puts " op             struct ExecBaseOp \{ .. \};"
  puts " optemplate     const struct ExecBaseOp optemplate = \{ .. \};"
  puts " offset         #define offset_AvailMem -123"
  puts " typedef        typedef struct MemHeader MemHeader;"
  puts "--------------------------------------------------------------------------------"
  puts ""
  puts "Example: genheader.tcl < mod/expansion/func.tcl proto"
  puts ""
}

if {[llength $argv] == 0} {
  usage
  exit 1
}

set do_internal false
set do_proto false
set do_libcall false
set do_offset false
set do_typedef false
set do_op false
set do_optemplate false
set do_inline false
set do_stub false

set argv1 [lindex $argv 0]
if {$argv1 == "internal"} {
  set do_internal true
} elseif {$argv1 == "proto"} {
  set do_proto true
} elseif {$argv1 == "libcall"} {
  set do_libcall true
} elseif {$argv1 == "offset"} {
  set do_offset true
} elseif {$argv1 == "typedef"} {
  set do_typedef true
} elseif {$argv1 == "op"} {
  set do_op true
} elseif {$argv1 == "optemplate"} {
  set do_optemplate true
} elseif {$argv1 == "inline"} {
  set do_inline true
} elseif {$argv1 == "stub"} {
  set do_stub true
  set destdir ""
  if {3 <= [llength $argv]} {
    set argv1 [lindex $argv 1]
    set argv2 [lindex $argv 2]
    if {$argv1 == "--dest"} {
      set destdir $argv2
    }
  }
} else {
  usage
  exit 1
}

set data [read stdin]

set script_dir [file dirname [file normalize [info script]]]
source [file join $script_dir mod.tcl]

set d [gen_dict $data]

# "MyBase"
set bs [dict get $d basestruct]
# "struct MyBase *"
set bt "struct $bs *"
# "struct MyBaseOp *"
set optv "struct ${bs}Op"
set opt "$optv *"
set lprefix [dict get $d prefix]
set opheader   "$lprefix/op.h"
set protoheader "$lprefix/proto.h"
set libcallheader "$lprefix/libcall.h"
set globalbase [dict get $d globalbase]

# "my.library"
set libname [dict get $d name]
set guardprefix [string map {. _} $libname]
set guardprefix [string toupper $guardprefix]

proc emitstructs {d} {
  foreach s [dict get $d structs] {
    puts "struct $s;"
  }
}

proc emitheaders {d} {
  foreach s [dict get $d headers] {
    puts "#include <$s>"
  }
}

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
  return [format "%-8s%s %s%s" "" $type $thestar $id]
}

# the parameter name is in the type
proc genfparam {type id} {
  set type [string trim $type]
  return [format "%-8s%s" "" $type]
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

proc emitbody {v opt {outf stdout} {bt ""} {gb ""}} {
  set fname [dict get $v name]
  set type [dict get $v return]
  set ret "return "
  set libtype "struct Library *"
  if {0 == [string compare $type "void"]} {
    set ret ""
  }
  puts $outf "\{"
  if {[string length $bt]} {
    puts $outf [format "  extern $bt $gb;"]
    puts $outf [format "  $bt const lib = $gb;"]
  }
  puts $outf [format "  $opt const op = ($opt) lib;"]
  puts -nonewline $outf [format {  %sop[-1].%s} $ret $fname]
  set argz [list]
  lappend argz "lib"
  foreach pdict [dict get $v param] {
    lappend argz [dict get $pdict name]
  }
  puts $outf [format "(%s);" [join $argz ", "]]
  puts $outf "\}"
}

if {$do_internal} {
  set guard "${guardprefix}_INTERNAL_H"
  puts "/* This file was generated by genheder.tcl. */"
  puts ""
  puts "#ifndef $guard"
  puts "#define $guard"
  puts ""
  puts "/* user: library implementation */"
  emitheaders $d
  puts ""
  puts "struct Library;"
  emitstructs $d
  puts ""
  foreach f [dict get $d funcs] {
    emitproto $f stdout i "" $bt
    puts ";\n"
  }
  puts "struct Segment *iExpungeBase(struct Library *lib);"
  puts "void iCloseBase(struct Library *lib);"
  puts "struct Library *iOpenBase(struct Library *lib);"
  puts ""
  puts "#include <$opheader>"
  puts ""
  puts "extern const $optv optemplate;"
  puts ""
  puts "#endif"
  exit 0
}

set proto_info "
/*
 * interface for application
 * Picks up lib from the global symbol \"$globalbase\"
 * Implemented in link library.
 */
"

if {$do_proto} {
  set guard "${guardprefix}_PROTO_H"
  puts "#ifndef $guard"
  puts "#define $guard"
  puts ""
  puts "/* Intended usage: application */"
  puts "$proto_info"
  emitheaders $d
  puts ""
  emitstructs $d
  puts ""
  foreach f [dict get $d funcs] {
    if {[dict exists $f short]} {
      set short [dict get $f short]
      puts "/* $short */"
    }
    emitproto $f stdout
    puts ";\n"
  }
  puts "#endif"
  exit 0
}

set libcall_info "
/*
 * interface for application
 * User provides the lib.
 * Implemented in link library.
 */
"

if {$do_libcall} {
  set guard "${guardprefix}_LIBCALL_H"
  puts "#ifndef $guard"
  puts "#define $guard"
  puts ""
  puts "/* Intended usage: application */"
  puts "$libcall_info"
  emitheaders $d
  puts ""
  emitstructs $d
  puts ""
  foreach f [dict get $d funcs] {
    if {[dict exists $f short]} {
      set short [dict get $f short]
      set short [string trim $short]
      set records [split $short "\n"]
      puts "/*"
      foreach rec $records {
        puts " * $rec"
      }
      puts " */"
    }
    emitproto $f stdout l "" $bt
    puts ";\n"
  }
  puts "#endif"
  exit 0
}

set offset_info "
/* SetFunction() parameters to patch $libname functions */
"

if {$do_offset} {
  set reserved_vectors 3
  set guard "${guardprefix}_OFFSET_H"
  puts "#ifndef $guard"
  puts "#define $guard"
  puts "$offset_info"
  set n [llength [dict get $d funcs]]
  set i [expr -1 -$reserved_vectors]
  puts "/* LibraryOp.Open()    -1 */"
  puts "/* LibraryOp.Close()   -2 */"
  puts "/* LibraryOp.Expunge() -3 */"
  foreach f [lreverse [dict get $d funcs]] {
    set name [dict get $f name]
    puts [format "#define offset_%-24s %3d" ${name} $i]
    incr i -1
  }
  puts ""
  puts "#endif"
  exit 0
}

if {$do_typedef} {
  set reserved_vectors 3
  foreach s [dict get $d structs] {
    puts [format "typedef struct %-16s %s;" $s $s]
  }
  puts ""
  exit 0
}

if {$do_op} {
  set guard "${guardprefix}_OP_H"
  puts "#ifndef $guard"
  puts "#define $guard"
  puts ""
  emitheaders $d
  puts ""
  emitstructs $d
  puts ""
  puts "#include <exec/libraries.h>"
  puts ""
  puts "struct ${bs}Op \{"
  foreach f [dict get $d funcs] {
    emitproto $f stdout "(*" ")" $bt "  "
    puts ";"
  }
  puts [format "%-8s%s %s;" "" "struct LibraryOp" LibraryOp]
  puts "\};\n"
  puts "#endif"
  exit 0
}

if {$do_optemplate} {
  puts "/* This file was generated by genheder.tcl. */"
  puts ""
  puts "#include <$opheader>"
  puts "#include \"internal.h\""
  puts ""
  puts "const $optv optemplate = \{"
  set memb [list]
  set lop "LibraryOp"
  foreach f [dict get $d funcs] {
    set fname [dict get $f name]
    lappend memb [format "%-8s%-24s= %s" "" ".$fname" "i$fname"]
  }
  foreach f {Expunge Close Open} {
    lappend memb [format "%-8s%-24s= %s" "" ".$lop.$f" "i${f}Base"]
  }
  set memb [join $memb ",\n"]
  puts -nonewline $memb
  puts ","
  puts "\};"
  puts ""
  exit 0
}

if {$do_inline} {
  set chan stdout
  puts $chan "#include <$opheader>"
  puts $chan ""
  foreach f [dict get $d funcs] {

    puts -nonewline "static inline "
    emitproto $f $chan l "" $bt
    puts $chan ""
    emitbody $f $opt $chan
    puts $chan ""
  }
  exit 0
}

if {$do_stub} {
  set n 0
  foreach f [dict get $d funcs] {
    set chan stdout
    if {[string length $destdir]} {
      set filename [dict get $f name]
      set filename [string tolower $filename]
      set filename [string cat $filename .c]
      set filename [string cat $filename]
      set filename [file join $destdir $filename]
      set chan [open $filename w]
    }
    puts $chan "#include <$protoheader>"
    puts $chan "#include <$opheader>"
    puts $chan ""

    emitproto $f $chan
    puts $chan ""
    emitbody $f $opt $chan $bt $globalbase
    puts $chan ""
    if {[string length $destdir]} {
      close $chan
      incr n
    }

    set chan stdout
    if {[string length $destdir]} {
      set filename [dict get $f name]
      set filename [string tolower $filename]
      set filename [string cat $filename .c]
      set filename [string cat l $filename]
      set filename [file join $destdir $filename]
      set chan [open $filename w]
    }
    puts $chan "#include <$libcallheader>"
    puts $chan "#include <$opheader>"
    puts $chan ""

    emitproto $f $chan l "" $bt
    puts $chan ""
    emitbody $f $opt $chan
    puts $chan ""
    if {[string length $destdir]} {
      close $chan
      incr n
    }
  }
  if {[string length $destdir]} {
    # puts "wrote $n files to $destdir"
  }
  exit 0
}

