#!/bin/sh
#
if type gmake > /dev/null 2>&1 ; then echo "gmake";exit; fi
if type make  > /dev/null 2>&1 ; then echo "make" ;exit; fi
echo "$0: GNU make required" ; \
exit 1

