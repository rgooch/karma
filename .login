#  .login  shell script. This shell script should be sourced by
# every  Karma  user.


# Written by		Richard Gooch	22-SEP-1992

# Last updated by	Richard Gooch	8-OCT-1993


# Define Karma installed base (for ordinary users)
setenv KARMABASE /usr/local/karma

# Define top of source (for karma programmers)
if (-d /wyvern/karma) then
    setenv KARMAROOT /wyvern/karma
else
    if (-r ~rgooch/karma) then
	setenv KARMAROOT ~rgooch/karma
    else
	if (-r /applic/karma) setenv KARMAROOT /applic/karma
    endif
endif

# Set up Karma executable paths and machine dependent environmental variables
if ("$?MACHINE_ARCH" == "0") then
    if (-r /usr/local/csh_script/machine_type) then
	set _database_file = /usr/local/csh_script/machine_type
    else
	set _database_file = ${KARMABASE}/csh_script/machine_type
    endif
    source $_database_file
endif

#setenv KARMABINPATH ${KARMABASE}/${MACHINE_ARCH}/bin
#setenv KARMALIBPATH ${KARMABASE}/${MACHINE_ARCH}/lib
setenv KARMABINPATH ${KARMABASE}/bin
setenv KARMALIBPATH ${KARMABASE}/lib
setenv KARMAINCLUDEPATH ${KARMABASE}/include

# Libraries to depend on (for modules)
switch ("$MACHINE_ARCH")
    case "SUNsparc":
	setenv KDEPLIB_KARMA $KARMALIBPATH/libkarma.sa.*
	#setenv KDEPLIB_KARMAX11 $KARMALIBPATH/libkarmaX11.sa.*
	#setenv KDEPLIB_KARMAXT $KARMALIBPATH/libkarmaXt.sa.*
	#setenv KDEPLIB_KARMAXVIEW $KARMALIBPATH/libkarmaxview.sa.*
	#setenv KDEPLIB_KARMAGRAPHICS $KARMALIBPATH/libkarmagraphics.sa.*
	setenv KARMA_CC "gcc -fpcc-struct-return"
	setenv KARMA_CPIC "-fPIC"
	breaksw
    case "linux"
	setenv KARMA_CC cc
	breaksw
    default:
	setenv KDEPLIB_KARMA $KARMALIBPATH/libkarma.a
	setenv KDEPLIB_KARMAX11 $KARMALIBPATH/libkarmaX11.a
	setenv KDEPLIB_KARMAXT $KARMALIBPATH/libkarmaXt.a
	setenv KDEPLIB_KARMAXVIEW $KARMALIBPATH/libkarmaxview.a
	setenv KDEPLIB_KARMAGRAPHICS $KARMALIBPATH/libkarmagraphics.a
	setenv KARMA_CC cc
	breaksw
endsw

if ("$?VXPATH" != "0") then
    setenv KARMAVXMVXBASE /phoenix/karmaVXMVX
    setenv KARMAVXMVXBINPATH $KARMAVXMVXBASE/bin
    setenv KARMAVXMVXLIBPATH $KARMAVXMVXBASE/lib
    setenv VXPATH "${KARMAVXMVXBINPATH}:$VXPATH"
endif

if ("$?_newpath" == "0") then
    # User does not use  _newpath  as a fast way of setting the path
    set path = (${KARMABASE}/cm_script ${KARMABASE}/csh_script $KARMABINPATH $path)
else
    # User uses  _newpath  as a fast way of setting the path
    set _newpath = (${KARMABASE}/cm_script ${KARMABASE}/csh_script $KARMABINPATH $_newpath)
endif

# Source KARMA .cshrc (not done initiallly)
source $KARMABASE/.cshrc

# Set version number environment variable
setenv KARMA_VERSION `cat $KARMABASE/.version`

exit
