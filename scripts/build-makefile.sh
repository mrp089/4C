
# build the Makefile
# This is done in three steps

# step 1: store all user supplied variables
#
cat > $makefile <<EOF
#
# Variables
#
SRC=$SRC

CC=$CC
F77=$F77
LD=$LD

PROGRAM=$PROGRAMNAME

CFLAGS=$CFLAGS $DEBUGFLAG -D$PLATFORM $DEFINES
FFLAGS=$FFLAGS $DEBUGFLAG -D$PLATFORM $DEFINES

LDFLAGS=$LDFLAGS
INCLUDES=$INCLUDEDIRS
LIBS=$LIBDIRS $LIBS

#----------------------- binaries -----------------------------------
include ./Makefile.objects
#--------------------------------------------------------------------
#
\$(PROGRAM): \\
		$OBJECTS
		@echo "Linking \$(LD) \$(LDFLAGS) \$(LIBS) \$(INCLUDES) -o \$(PROGRAM)"
		@\$(LD) \$(LDFLAGS) \\
		$OBJECTS \\
		\$(LIBS)  -o \$(PROGRAM)

EOF


# step 2: copy Makefile.in
cat Makefile.in >> $makefile
#awk '$1 !~ /include/ { print $0 } $1 ~ /include/ { system("cat " $2)}' Makefile.in >> Makefile

# step 3: build dependencies if gcc can be found
# 
# you can skip this step by saying
# $ NODEPS=yes ./configure ...
#
if [ x$NODEPS != "xyes" ] ; then
  # hopefully nobody translates which's messages...
  if which gcc | grep '^no ' 2>&1 > /dev/null ; then
     echo $0: gcc not found. No dependencies generated. Use Makefile with care.
  else
    for file in `find src -name "*.c"` ; do
      echo "build deps for" $file
      gcc -D$PLATFORM $DEFINES -MM -MT `echo $file|sed -e 's/c$/o/'` $INCLUDEDIRS $file >> $makefile
    done
  fi
fi
