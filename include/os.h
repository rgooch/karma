/*  os.h

    Header for Operating System dependent definitions.

    Copyright (C) 1992,1993,1994  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*

    This header file contains various compile-time definitions which determine
    how the Karma library should interface to the operating system. This
    file should NOT be included by non-library code, as it is subject to change
    without notice.


    Written by      Richard Gooch   20-MAY-1992

    Last updated by Richard Gooch   4-NOV-1994


*/

/*                          BASIC ASSUMPTIONS

    All platforms have 8 bit bytes.

    The C type  char  and  unsigned char  are one byte on all platforms.

    All platforms have a signed and an unsigned integer which are at least
    4 bytes.

    The size of all pointers is the same.
*/


/*  If  BLOCK_TRANSFER  is defined, this means that the host data format is
    the same as the network data format.
*/
#undef BLOCK_TRANSFER


/*  If  MATCHING_SIZES  is defined, the host C data types are the same size as
    their corresponding network data types.
*/
#undef MATCHING_SIZES


/*  If  BYTE_SWAPPER  is defined, the ONLY difference between the host data
    format and the network data format is that the bytes are swapped.
*/
#undef BYTE_SWAPPER


/*  If  MACHINE_SUPPORTED  is defined, then this machine type (i.e. CPU type)
    has been supported in the Karma library. If it is not defined, compilation
    will stop in this file.
*/
#undef MACHINE_SUPPORTED


/*  If  OS_SUPPORTED  is defined, then this Operating System has been supported
    in the Karma library. If it is not defined, compilation will stop in this
    file.
*/
#undef OS_SUPPORTED


/*  If  RETYPE_NEEDED  is defined, then some of the host C integer data
    types are smaller than their corresponding network sizes, and hence must
    be retyped so as not to change the size when converting between host and
    network data formats.
    Note that no retyping is required for machines which have larger data types
    than the network data types.
*/
#undef RETYPE_NEEDED


/*  If  CAN_FORK  is defined, then the operating system allows forking of new
    processes.
*/
#undef CAN_FORK


/*  If  HAS_GETRUSAGE  is defined, then the operating system has the
    getrusage(2)  system call.
*/
#undef HAS_GETRUSAGE


/*  If  HAS_TIMES  is defined, then the operating system has the
    times(2)  system call.
*/
#undef HAS_TIMES


/*  If HAS_SOCKETS  is defined, then the operating system has socket support.
*/
#undef HAS_SOCKETS


/*  If  HAS_TERMCAP  is defined, then the operating system has the  termcap
    library.
*/
#undef HAS_TERMCAP


/*  If  HAS_SYSV_SHARED_MEMORY  is defined, then the operating system supports
    the System V shared memory calls.
*/
#undef HAS_SYSV_SHARED_MEMORY


/*  If  NEEDS_MISALIGN_COMPILE  is defined, then misaligned data accesses will
    cause a segmentation fault unless the code has been specially compiled to
    get around this problem.
    NOTE: this does NOT apply to platforms where the operating system fixes up
    misaligned data accesses (such as the Dec MipsStations).
*/
#undef NEEDS_MISALIGN_COMPILE


/*  If  HAS_ON_EXIT  is defined, then the platform supports the on_exit(3)
    function (which allows the application to register a function which is
    called when exit(3) is called.
*/
#undef HAS_ON_EXIT


/*  If  HAS_ATEXIT  is defined, then the platform supports the atexit(3)
    function (which allows the application to register a function which is
    called when exit(3) is called.
*/
#undef HAS_ATEXIT


/*  If  HAS_INTERNATIONALISATION  is defined, then the platform has the
    Internationalisation extensions (ie. LC_TYPE, etc).
*/
#undef HAS_INTERNATIONALISATION


/*  If  HAS_COMMUNICATIONS_EMULATION  is defined, then the platform has
    emulation support for interprocess communication.
*/
#undef HAS_COMMUNICATIONS_EMULATION


/*  If  HAS_ENVIRON  is defined, then the platform has an environment defined.
    Otherwise, the environment is emulated with the  r_getenv  and r_setenv
    routines.
*/
#undef HAS_ENVIRON


/*  If  HAS_MMAP  is defined, then the platform supports a flavour of mmap(2)
*/
#undef HAS_MMAP


/*  If  HAS_WAIT3  is defined, then the platform supports the wait3(2)  system
    call, with complete  rusage  support.
*/
#undef HAS_WAIT3


/*  Slowaris 2  */
#ifdef OS_Solaris
#  define OS_SUPPORTED
#  if defined(MACHINE_sparc)
#    define MACHINE_SUPPORTED
#    define BLOCK_TRANSFER
#    define MATCHING_SIZES
/*#  define NEEDS_MISALIGN_COMPILE*/ /*  Needs "optional" C compiler  */
#  endif
#  define CAN_FORK
#  define HAS_TIMES
#  define HAS_SOCKETS
#  define HAS_TERMCAP
#  define HAS_SYSV_SHARED_MEMORY
#  define HAS_INTERNATIONALISATION
#  define HAS_ENVIRON
#  define HAS_MMAP
#  define HAS_ATEXIT
#endif  /*  OS_Solaris  */


/*  SunOS 4.x  */
#ifdef OS_SunOS
#  define OS_SUPPORTED
#  ifdef MACHINE_sparc
#    define MACHINE_SUPPORTED
#    define BLOCK_TRANSFER
#    define MATCHING_SIZES
#    define NEEDS_MISALIGN_COMPILE  /*  change this later?  */
#  endif
#  define CAN_FORK
#  define HAS_GETRUSAGE
#  define HAS_TIMES
#  define HAS_SOCKETS
#  define HAS_TERMCAP
#  define HAS_SYSV_SHARED_MEMORY
#  define HAS_INTERNATIONALISATION
#  define HAS_ENVIRON
#  define HAS_MMAP
#  define HAS_ON_EXIT
#  define HAS_WAIT3
#endif  /*  OS_SunOS  */

/*  Convex  */
#if defined(OS_ConvexOS) && defined(MACHINE_c2)
#  define OS_SUPPORTED
#  define MACHINE_SUPPORTED
#  define BLOCK_TRANSFER
#  define MATCHING_SIZES
#  define CAN_FORK
#  define HAS_GETRUSAGE
#  define HAS_SOCKETS
#  define HAS_TERMCAP
#  define HAS_ATEXIT
#  define HAS_INTERNATIONALISATION
#  define HAS_ENVIRON
#  define HAS_MMAP
#  define HAS_WAIT3
#endif

/*  IBM RS6000 with AIX  */
#if defined(OS_AIX) && defined(MACHINE_rs6000)
#  define OS_SUPPORTED
#  define MACHINE_SUPPORTED
#  define BLOCK_TRANSFER
#  define MATCHING_SIZES
#  define CAN_FORK
#  define HAS_GETRUSAGE
#  define HAS_SOCKETS
#  define HAS_TERMCAP
/*#  define HAS_SYSV_SHARED_MEMORY*/  /*  Disabled because of missing Xshm  */
#  define HAS_ATEXIT
#  define HAS_INTERNATIONALISATION
#  define HAS_ENVIRON
#  define HAS_WAIT3
#endif

/*  VX/ MVX system for a Sun Sparc Station  */
#if defined(OS_VXMVX) && defined(MACHINE_i860)
#  define OS_SUPPORTED
#  define MACHINE_SUPPORTED
#  define BLOCK_TRANSFER
#  define MATCHING_SIZES
#  define HAS_COMMUNICATIONS_EMULATION
#endif

/*  SGI Station  */
#if defined(OS_IRIX5)
#  define OS_SUPPORTED
#  if defined(MACHINE_mips1) || defined(MACHINE_mips2)
#    define MACHINE_SUPPORTED
#    define BLOCK_TRANSFER
#    define MATCHING_SIZES
#  endif
#  define CAN_FORK
#  define HAS_GETRUSAGE
#  define HAS_SOCKETS
#  define HAS_TERMCAP
#  define HAS_SYSV_SHARED_MEMORY
/*#  define NEEDS_MISALIGN_COMPILE*/
#  define HAS_ATEXIT
#  define HAS_INTERNATIONALISATION
#  define HAS_ENVIRON
#  define HAS_MMAP
#  define HAS_WAIT3
#endif

/*  HP/Apollo 9000  */
#if defined(OS_HPUX) && defined(MACHINE_hp9000)
#  define OS_SUPPORTED
#  define MACHINE_SUPPORTED
#  define BLOCK_TRANSFER
#  define MATCHING_SIZES
#  define CAN_FORK
#  define HAS_SOCKETS
#  define HAS_TERMCAP
/*#  define HAS_SYSV_SHARED_MEMORY*/  /*  No Xshm in standard HPUX library  */
#  define HAS_ATEXIT
#  define HAS_INTERNATIONALISATION
#  define HAS_ENVIRON
#endif


/*  Special consideration for machines which only do byte swapping  */
/*  DEC Mips workstation  */
#if defined(OS_ULTRIX) && defined(MACHINE_mips1)
#  define OS_SUPPORTED
#  define MACHINE_SUPPORTED
#  define BYTE_SWAPPER
#  define MATCHING_SIZES
#  define CAN_FORK
#  define HAS_GETRUSAGE
#  define HAS_SOCKETS
#  define HAS_TERMCAP
#  define HAS_SYSV_SHARED_MEMORY
#  define HAS_ATEXIT
#  define HAS_INTERNATIONALISATION
#  define HAS_ENVIRON
/*#  define HAS_MMAP  Ultrix mmap(2) does not seem to support regular files! */
#  define HAS_WAIT3
#endif

/*  IBM PC with 80386 processor running in full, 32 bit 386 mode under
    MS-DOS  */
#if defined(OS_MSDOS) && defined(MACHINE_i386)
#  define OS_SUPPORTED
#  define MACHINE_SUPPORTED
#  define BYTE_SWAPPER
#  define MATCHING_SIZES
#  define HAS_ENVIRON
#endif

/*  IBM PC with 80386 processor running in full, 32 bit 386 mode under linux */
#if defined(OS_Linux)
#  define OS_SUPPORTED
#  if defined(MACHINE_i386) || defined(MACHINE_i486)
#    define MACHINE_SUPPORTED
#    define BYTE_SWAPPER
#    define MATCHING_SIZES
#  endif
#  define CAN_FORK
#  define HAS_GETRUSAGE
#  define HAS_SOCKETS
#  define HAS_TERMCAP
#  define HAS_SYSV_SHARED_MEMORY
#  define HAS_ON_EXIT
#  define HAS_INTERNATIONALISATION
#  define HAS_ENVIRON
#  define HAS_MMAP
#  define HAS_WAIT3
#endif

/*  Phantom machine  */
#ifdef phantom_machine
#  define OS_SUPPORTED
#  define MACHINE_SUPPORTED
#  define BYTE_SWAPPER
#  define MATCHING_SIZES
#  define HAS_ENVIRON
#endif

/*  All other machines will require some special conversion routines  */
/*  Cray Y-MP  */
#ifdef arch_cray
#  define OS_SUPPORTED
#  define MACHINE_SUPPORTED
#  ifdef OS_H_VARIABLES
static int host_to_net_size[NUMTYPES] =
{
    0,		/* 	None Length		*/
    4,		/*	Float length		*/
    8,		/*	Double length		*/
    1,		/*	Char length		*/
    4,		/*	Int length		*/
    2,		/*	Short length		*/
    0,		/*	Array pointer length	*/
    0,		/*	List pointer length	*/
    0,		/*	Multi Array length	*/
    8,		/*	Float Complex length	*/
    16,		/*	Double Complex length	*/
    2,		/*	Byte Complex length	*/
    8,		/*	Int Complex length	*/
    4,		/*	Short Complex length	*/
    4,		/*	Long length		*/
    8,		/*	Long Complex length	*/
    1,		/*	Unsigned Char length	*/
    4,		/*	Unsigned Int length	*/
    2,		/*	Unsigned Short length	*/
    4,		/*	Unsigned Long length	*/
    2,		/*	Unsigned Byte Complex length	*/
    8,		/*	Unsigned Int Complex length	*/
    4,		/*	Unsigned Short Complex length	*/
    8,		/*	Unsigned Long Complex length	*/
    0,		/*	Array pointer length	*/
    0,		/*	Variable string pointer length	*/
    0		/*	Fixed string pointer length	*/
};
#  endif  /*  OS_H_VARIABLES  */
#endif  /*  arch_cray  */

/*  DEC Alpha running OSF/1  */
#if defined(OS_OSF1) && defined(MACHINE_alpha)
#  define OS_SUPPORTED
#  define MACHINE_SUPPORTED
#  define CAN_FORK
#  define HAS_GETRUSAGE
#  define HAS_SOCKETS
#  define HAS_TERMCAP
#  define HAS_SYSV_SHARED_MEMORY
/*#  define NEEDS_MISALIGN_COMPILE*/
#  define HAS_ATEXIT
#  define HAS_INTERNATIONALISATION
#  define HAS_ENVIRON
#  define HAS_MMAP
#  define HAS_WAIT3
#  ifdef OS_H_VARIABLES
static int host_to_net_size[NUMTYPES] =
{
    0,		/* 	None Length		*/
    4,		/*	Float length		*/
    8,		/*	Double length		*/
    1,		/*	Char length		*/
    4,		/*	Int length		*/
    2,		/*	Short length		*/
    0,		/*	Array pointer length	*/
    0,		/*	List pointer length	*/
    0,		/*	Multi Array length	*/
    8,		/*	Float Complex length	*/
    16,		/*	Double Complex length	*/
    2,		/*	Byte Complex length	*/
    8,		/*	Int Complex length	*/
    4,		/*	Short Complex length	*/
    4,		/*	Long length		*/
    8,		/*	Long Complex length	*/
    1,		/*	Unsigned Char length	*/
    4,		/*	Unsigned Int length	*/
    2,		/*	Unsigned Short length	*/
    4,		/*	Unsigned Long length	*/
    2,		/*	Unsigned Byte Complex length	*/
    8,		/*	Unsigned Int Complex length	*/
    4,		/*	Unsigned Short Complex length	*/
    8,		/*	Unsigned Long Complex length	*/
    0,		/*	Array pointer length	*/
    0,		/*	Variable string pointer length	*/
    0		/*	Fixed string pointer length	*/
};
#  endif  /*  OS_H_VARIABLES  */
#endif  /*  alpha_OSF1  */


/*  The machine should be supported by now  */
#ifndef MACHINE_SUPPORTED
/*  Machine has not been supported  */
    !!!! ERROR !!! *** Machine (CPU type) not supported ****
#endif
/*  The OS should be supported by now  */
#ifndef OS_SUPPORTED
/*  OS has not been supported  */
    !!!! ERROR !!! *** Operating System not supported ****
#endif

#define NET_FLOAT_SIZE 4
#define NET_DOUBLE_SIZE 8

/*  Define network type sizes  */
#ifdef OS_H_VARIABLES
static char network_type_bytes[NUMTYPES] =
{
    0,		/* 	None Length		*/
    4,		/*	Float length		*/
    8,		/*	Double length		*/
    1,		/*	Char length		*/
    4,		/*	Int length		*/
    2,		/*	Short length		*/
    0,		/*	Array pointer length	*/
    0,		/*	List pointer length	*/
    0,		/*	Multi Array length	*/
    8,		/*	Float Complex length	*/
    16,		/*	Double Complex length	*/
    2,		/*	Byte Complex length	*/
    8,		/*	Int Complex length	*/
    4,		/*	Short Complex length	*/
    4,		/*	Long length		*/
    8,		/*	Long Complex length	*/
    1,		/*	Unsigned Char length	*/
    4,		/*	Unsigned Int length	*/
    2,		/*	Unsigned Short length	*/
    4,		/*	Unsigned Long length	*/
    2,		/*	Unsigned Byte Complex length	*/
    8,		/*	Unsigned Int Complex length	*/
    4,		/*	Unsigned Short Complex length	*/
    8,		/*	Unsigned Long Complex length	*/
    0,		/*	Array pointer length	*/
    0,		/*	Variable string pointer length	*/
    0		/*	Fixed string pointer length	*/
};
#endif  /*  OS_H_VARIABLES  */

#if __STDC__ == 1
#  define NET_UINT_MAX 4294967295u
#  define NET_INT_MAX (int) 2147483647
#  define NET_INT_MIN (int) -2147483648
#  define NET_USHORT_MAX (unsigned short) 65535
#  define NET_SHORT_MAX (short) 32767
#  define NET_SHORT_MIN (short) -32768
#else

#  define NET_UINT_MAX (unsigned int) 4294967295
#  define NET_INT_MAX (int) 2147483647
#  define NET_INT_MIN (int) -2147483648
#  define NET_USHORT_MAX (unsigned short) 65535
#  define NET_SHORT_MAX (short) 32767
#  define NET_SHORT_MIN (short) -32768
#endif
