#undef LIB_SPEC
#define LIB_SPEC \
" \
-lc \
-lbros \
"

#undef STARTFILE_SPEC
#define STARTFILE_SPEC \
" \
%{qstart=*:%*.c.o%s; :ustart.c.o%s} \
"

#undef ENDFILE_SPEC
#define ENDFILE_SPEC ""

