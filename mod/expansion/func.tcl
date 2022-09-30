# SPDX-License-Identifier: GPL-2.0
# Copyright 2022 Martin Åberg

mod_base        ExpansionBase
mod_basetype    ExpansionBase
mod_libname     expansion.library
mod_prefix      expansion
mod_struct      ExpansionBase
mod_struct      ExpansionDriver
mod_struct      ExpansionBus
mod_struct      ExpansionDev
mod_struct      Interrupt

func_begin      AddExpansionDriver
func_return     "int"
func_param      "struct ExpansionDriver *" driver
func_short      "add a driver"
func_long       "
Add a driver to the expansion driver database.
"
func_result     "EXPANSION_OK"
func_see        "SetExpansionRoot()"

func_begin      SetExpansionRoot
func_return     "int"
func_param      "struct ExpansionDriver *" driver
func_short      "set the root driver"
func_long       "
Set the expansion root driver. The root device is always
provided by expansion. The driver shall have bustype root.

SetExpansionRoot() shall only be called once.
"
func_result     "EXPANSION_OK"
func_see        "AddExpansionDriver(), UpdateExpansion()"

func_begin      AddExpansionDev
func_return     "int"
func_param      "struct ExpansionDev *" dev
func_param      "struct ExpansionBus *" parent
func_short      "add device"
func_long       "
Add a device to the expansion tree of devices. The device will
be located under the parent bus.
"
func_result     "EXPANSION_OK"
func_see        "UpdateExpansion()"

func_begin      UpdateExpansion
func_return     "int"
func_short      "match devices with drivers"
func_long       "
This function will recurse trough all devices on all buses, pair
the device with a driver, and run the driver init functions.

SetExpansionRoot() must be called before calling
UpdateExpansion().
"
func_result     "EXPANSION_OK"
func_see        "AddExpansionDev(), AddExpansionDriver()"

func_begin      GetDevFreq
func_return     "int"
func_param      "struct ExpansionDev *" dev
func_param      "long long *" hz
func_short      "get frequency of a device"
func_long       "
This function obtains the frequency of the device by consulting
the parent bus. It can be used by drivers to to set devce scaler
to generate bitrates etc.

The frequency is stored in the hz parameter which is valid if
the return value is 0.
"
func_result     "EXPANSION_OK iff success"

func_begin      AddDevInt
func_return     "int"
func_param      "struct ExpansionDev *" dev
func_param      "struct Interrupt *" interrupt
func_param      "int" intnum
func_short      "add interrupt handler for device"
func_long       "
Add an interrupt handler for a device.
"
func_result     "EXPANSION_OK iff success"
func_see        "RemDevInt()"

func_begin      RemDevInt
func_return     "int"
func_param      "struct ExpansionDev *" dev
func_param      "struct Interrupt *" interrupt
func_param      "int" intnum
func_short      "remove interrupt handler for device"
func_long       "
Remove an interrupt handler for a device.
"
func_result     "EXPANSION_OK iff success"
func_see        "AddDevInt()"

