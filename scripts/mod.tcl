# SPDX-License-Identifier: GPL-2.0
# Copyright 2020-2022 Martin Åberg

# Domain-specific language for Libraries
# Useful for generating headers and library function manuals

proc mod_base {val} {
  global d
  dict set d globalbase $val
}

proc mod_basetype {val} {
  global d
  dict set d basestruct $val
}

proc mod_libname {val} {
  global d
  dict set d name $val
}

proc mod_prefix {val} {
  global d
  dict set d prefix $val
}

proc mod_header {val} {
  global d
  dict lappend d headers $val
}

proc mod_struct {val} {
  global d
  dict lappend d structs $val
}

proc func_begin {val} {
  global curfunc
  if {[info exists curfunc]} {
    func_end
  }
  set curfunc [dict create]
  dict set curfunc param [list]
  dict set curfunc name $val
}

proc func_end {} {
  global d
  global curfunc
  dict lappend d funcs $curfunc
  unset curfunc
}

proc func_return {val} {
  global curfunc
  dict set curfunc return $val
}

proc func_param {type name {extra ""}} {
  global curfunc
  set thisparam [dict create]
  dict set thisparam name $name
  dict set thisparam type $type
  set isfunc false
  if {$extra == "isfunc"} {
    set isfunc true
  }
  dict set thisparam func $isfunc
  dict lappend curfunc param $thisparam
}

proc func_env {env} {
}

proc func_short {val} {
  global curfunc
  dict set curfunc short $val
}

proc func_long {val} {
  global curfunc
  dict set curfunc long $val
}

proc func_result {val} {
  global curfunc
  dict set curfunc result $val
}

proc func_see {val} {
  global curfunc
  dict set curfunc see $val
}

proc gen_dict {data} {
  global d

  set d [dict create]
  dict set d headers [list]
  dict set d structs [list]
  dict set d funcs [list]
  eval $data
  func_end

  return $d
}

