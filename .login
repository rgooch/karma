#  .login  C shell script. This C shell script should be sourced by
# every  Karma  user.


#   Copyright (C) 1992,1993,1994,1995  Richard Gooch
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#   Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
#   The postal address is:
#     Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.


# Written by		Richard Gooch	22-SEP-1992

# Last updated by	Richard Gooch	25-JAN-1995


# Define Karma installed base (for ordinary users)
if ("$?KARMABASE" == "0") setenv KARMABASE /usr/local/karma

# List of Karma root directories
set karmaroots = (/wyvern/karma /applic/karma /aips++1/karma /usr/local/src/karma /purgatory_1/rgooch/karma $HOME/karma)
foreach i ($karmaroots)
    if (-d $i) then
	if ("$?KARMAROOT" == "0") setenv KARMAROOT $i
    endif
end

# Set up Karma executable paths and machine dependent environmental variables
if ("$?MACHINE_OS" == "0") then
    if (-r /usr/local/csh_script/machine_type) then
	set _database_file = /usr/local/csh_script/machine_type
    else
	set _database_file = ${KARMABASE}/csh_script/machine_type
    endif
    source $_database_file
endif

if ("$?OS" == "0") then
    set tmp = `echo "$MACHINE_OS"|sed -e "s/_/ /"`
    setenv MACHINE "$tmp[1]"
    setenv OS "$tmp[2]"
    unset tmp
endif

setenv KARMABINPATH ${KARMABASE}/bin
setenv KARMALIBPATH ${KARMABASE}/lib
setenv KARMAINCLUDEPATH ${KARMABASE}/include

# Libraries to depend on (for modules)
switch ("$OS")
    case "SunOS":
	setenv KARMA_CC "gcc -fpcc-struct-return"
	setenv KARMA_CPIC "-fPIC"
	setenv KARMA_LD gcc
	setenv KDEPLIB_KARMA $KARMALIBPATH/libkarma.sa.*
	#setenv KDEPLIB_KARMAX11 $KARMALIBPATH/libkarmaX11.sa.*
	#setenv KDEPLIB_KARMAXT $KARMALIBPATH/libkarmaXt.sa.*
	#setenv KDEPLIB_KARMAXVIEW $KARMALIBPATH/libkarmaxview.sa.*
	#setenv KDEPLIB_KARMAGRAPHICS $KARMALIBPATH/libkarmagraphics.sa.*
	if ("$?_k_need_llp" != "0") then
	    if ("$?LD_LIBRARY_PATH" == "0") then
		setenv LD_LIBRARY_PATH $KARMALIBPATH
	    else
		setenv LD_LIBRARY_PATH "${KARMALIBPATH}:${LD_LIBRARY_PATH}"
	    endif
	endif
	breaksw
    case "Solaris":
	setenv KARMA_CC "gcc -fpcc-struct-return -D_REENTRANT"
	setenv KARMA_CPIC "-fPIC"
	setenv KARMA_LD gcc
	setenv OS_LIBS "-lthread"
	if ("$?_k_need_llp" != "0") then
	    if ("$?LD_LIBRARY_PATH" == "0") then
		setenv LD_LIBRARY_PATH $KARMALIBPATH
	    else
		setenv LD_LIBRARY_PATH "${KARMALIBPATH}:${LD_LIBRARY_PATH}"
	    endif
	endif
	breaksw
    case "Linux"
	setenv KARMA_CC cc
	setenv KARMA_LD "$KARMA_CC"
	setenv KDEPLIB_KARMA $KARMALIBPATH/libkarma.sa
	setenv KDEPLIB_KARMAX11 $KARMALIBPATH/libkarmaX11.sa
	setenv KDEPLIB_KARMAXT $KARMALIBPATH/libkarmaXt.sa
	setenv KDEPLIB_KARMAXVIEW $KARMALIBPATH/libkarmaxview.sa
	setenv KDEPLIB_KARMAGRAPHICS $KARMALIBPATH/libkarmagraphics.sa
	setenv KDEPLIB_KARMAWIDGETS $KARMALIBPATH/libkarmawidgets.sa
	breaksw
    case "IRIX5":
	setenv KARMA_CC "cc -xansi -signed -D_SGI_MP_SOURCE"
	setenv KARMA_LD "cc -xansi -Wl,-no_library_replacement"
	breaksw
    case "OSF1":
	# gcc causes problems with shared Xt library ???
	#setenv KARMA_CC "gcc -fpcc-struct-return"
	setenv KARMA_CC "cc -std"
	setenv KARMA_LD "$KARMA_CC -Wl,-no_library_replacement"
	breaksw
    case "AIX":
	setenv KARMA_CC "cc -qchars=signed -qlanglvl=ansi"
	setenv KARMA_LD "cc"
	breaksw
    case "ConvexOS":
	setenv KARMA_CC "cc -ext"
	setenv KARMA_LD "$KARMA_CC"
	setenv KDEPLIB_KARMA $KARMALIBPATH/libkarma.a
	setenv KDEPLIB_KARMAX11 $KARMALIBPATH/libkarmaX11.a
	setenv KDEPLIB_KARMAXT $KARMALIBPATH/libkarmaXt.a
	setenv KDEPLIB_KARMAGRAPHICS $KARMALIBPATH/libkarmagraphics.a
	setenv KDEPLIB_KARMAWIDGETS $KARMALIBPATH/libkarmawidgets.a
	breaksw
    case "HPUX":
	setenv KARMA_CC "cc -Aa -D_HPUX_SOURCE"
	setenv KARMA_LD "cc"
	breaksw
    case "ULTRIX":
	setenv KARMA_CC "gcc -fpcc-struct-return"
	setenv KARMA_LD gcc
	setenv KDEPLIB_KARMA $KARMALIBPATH/libkarma.a
	setenv KDEPLIB_KARMAX11 $KARMALIBPATH/libkarmaX11.a
	setenv KDEPLIB_KARMAXT $KARMALIBPATH/libkarmaXt.a
	setenv KDEPLIB_KARMAGRAPHICS $KARMALIBPATH/libkarmagraphics.a
	setenv KDEPLIB_KARMAWIDGETS $KARMALIBPATH/libkarmawidgets.a
	breaksw
    default:
	setenv KARMA_CC cc
	setenv KARMA_LD "$KARMA_CC"
	setenv KDEPLIB_KARMA $KARMALIBPATH/libkarma.a
	setenv KDEPLIB_KARMAX11 $KARMALIBPATH/libkarmaX11.a
	setenv KDEPLIB_KARMAXT $KARMALIBPATH/libkarmaXt.a
	setenv KDEPLIB_KARMAGRAPHICS $KARMALIBPATH/libkarmagraphics.a
	setenv KDEPLIB_KARMAWIDGETS $KARMALIBPATH/libkarmawidgets.a
	breaksw
endsw

if ("$?VXPATH" != "0") then
    if ("$?KARMAVXMVXBASE" == "0") setenv KARMAVXMVXBASE /phoenix/karmaVXMVX
    setenv KARMAVXMVXBINPATH $KARMAVXMVXBASE/bin
    setenv KARMAVXMVXLIBPATH $KARMAVXMVXBASE/lib
    setenv VXPATH "${KARMAVXMVXBINPATH}:$VXPATH"
endif

set karmabin = (${KARMABASE}/cm_script ${KARMABASE}/csh_script $KARMABINPATH)
if ("$?_newpath" == "0") then
    # User does not use  _newpath  as a fast way of setting the path
    set path = ($karmabin $path)
else
    # User uses  _newpath  as a fast way of setting the path
    set _newpath = ($karmabin $_newpath)
endif

# Source KARMA .cshrc (not done initiallly)
source $KARMABASE/.cshrc

# Set version number environment variable
setenv KARMA_VERSION `cat $KARMABASE/.version`

exit
