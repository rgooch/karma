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

# Last updated by	Richard Gooch	2-MAY-1996


# Define Karma installed base (for ordinary users)
if ("$?KARMABASE" == "0") setenv KARMABASE /usr/local/karma

# List of Karma root directories
set karmaroots = (/vindaloo/karma /home/karma /applic/karma /nfs/applic/karma /aips++1/karma /usr/local/src/karma $HOME/karma)
foreach i ($karmaroots)
    if ("$?KARMAROOT" == "0") then
	if (-d $i) setenv KARMAROOT $i
    endif
end

# Set up Karma executable paths and machine dependent environmental variables
if ("$?MACHINE_OS" == "0") then
    if (-x /usr/local/bin/platform) then
	set _platform_file = /usr/local/bin/platform
    else
	set _platform_file = ${KARMABASE}/csh_script/uname_to_platform
    endif
    setenv MACHINE_OS `$_platform_file`
    unset _platform_file
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

# Set version number environment variable
set _versionfile = $KARMAINCLUDEPATH/k_version.h
set tmp = `fgrep KARMA_VERSION $_versionfile | tr '"' ' ' | tr - ' '`
setenv KARMA_VERSION "$tmp[$#tmp]"
if ("$KARMA_VERSION" == "") then
    echo "WARNING: KARMA_VERSION environment variable could not be computed."
endif
unset _versionfile

# Libraries to depend on (for modules)
switch ("$OS")
    case "SunOS":
	setenv KDEPLIB_KARMA $KARMALIBPATH/libkarma.sa.$KARMA_VERSION
	#setenv KDEPLIB_KARMAX11 $KARMALIBPATH/libkarmaX11.sa.$KARMA_VERSION
	#setenv KDEPLIB_KARMAXT $KARMALIBPATH/libkarmaXt.sa.$KARMA_VERSION
	#setenv KDEPLIB_KARMAXVIEW $KARMALIBPATH/libkarmaxview.sa.$KARMA_VERSION
	#setenv KDEPLIB_KARMAGRAPHICS $KARMALIBPATH/libkarmagraphics.sa.$KARMA_VERSION
	if ("$?_k_need_llp" != "0") then
	    if ("$?LD_LIBRARY_PATH" == "0") then
		setenv LD_LIBRARY_PATH $KARMALIBPATH
	    else
		setenv LD_LIBRARY_PATH "${KARMALIBPATH}:${LD_LIBRARY_PATH}"
	    endif
	endif
	breaksw
    case "Solaris":
	if ("$?_k_need_llp" != "0") then
	    if ("$?LD_LIBRARY_PATH" == "0") then
		setenv LD_LIBRARY_PATH $KARMALIBPATH
	    else
		setenv LD_LIBRARY_PATH "${KARMALIBPATH}:${LD_LIBRARY_PATH}"
	    endif
	endif
	breaksw
    case "Linux"
    case "IRIX5":
    case "IRIX6":
    case "OSF1":
    case "AIX":
    case "HPUX":
	# Platforms with shared libraries: no dependencies on libraries.
	breaksw
    default:
	# Platforms with static libraries: modules depend on libraries.
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

if ("$?MANPATH" == "0") then
    # Initialise MANPATH
    setenv MANPATH "${KARMABASE}/man:/usr/man"
else
    setenv MANPATH "${KARMABASE}/man:${MANPATH}"
endif

# Source KARMA .cshrc (not done initiallly)
source $KARMABASE/.cshrc

exit
