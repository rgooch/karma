/*  os.h

    Header for Operating System dependent definitions.

    Copyright (C) 1992,1993  Richard Gooch

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
    how the Karma library should interface to the operating system.


    Written by      Richard Gooch   20-MAY-1992

    Last updated by Richard Gooch   1-SEP-1993


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


/*  If  PLATFORM_SUPPORTED  is defined, then this platform has been supported
    in the Karma library. If it is not defined, compilation will stop in this
    file.
*/
#undef PLATFORM_SUPPORTED


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


/*  If  HAS_FORTRAN  is defined, then the platform has a fortran compiler
    available. Unfortunately, there is some code (Singleton's FFT) which still
    requires a fortran compiler.
*/
#undef HAS_FORTRAN


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


/*  Special consideration for machines which have the network data format  */
/*  Sun Sparc Station  */
#ifdef ARCH_SUNsparc
#define BLOCK_TRANSFER
#define MATCHING_SIZES
#define PLATFORM_SUPPORTED
#define CAN_FORK
#define HAS_GETRUSAGE
#define HAS_SOCKETS
#define HAS_TERMCAP
#define HAS_SYSV_SHARED_MEMORY
#define NEEDS_MISALIGN_COMPILE
#define HAS_FORTRAN
#define HAS_ON_EXIT
#define HAS_INTERNATIONALISATION
#define HAS_ENVIRON
#define HAS_MMAP
#endif

/*  Convex  */
#ifdef ARCH_convex
#define BLOCK_TRANSFER
#define MATCHING_SIZES
#define PLATFORM_SUPPORTED
#define CAN_FORK
#define HAS_GETRUSAGE
#define HAS_SOCKETS
#define HAS_TERMCAP
#define HAS_FORTRAN
#define HAS_ATEXIT
#define HAS_INTERNATIONALISATION
#define HAS_ENVIRON
#define HAS_MMAP
#endif

/*  IBM RS6000 with AIX  */
#ifdef ARCH_rs6000
#define BLOCK_TRANSFER
#define MATCHING_SIZES
#define PLATFORM_SUPPORTED
#define CAN_FORK
#define HAS_GETRUSAGE
#define HAS_SOCKETS
#define HAS_TERMCAP
#define HAS_SYSV_SHARED_MEMORY
#define HAS_ATEXIT
#define HAS_INTERNATIONALISATION
#define HAS_ENVIRON
#endif

/*  VX/ MVX system for a Sun Sparc Station  */
#ifdef ARCH_VXMVX
#define BLOCK_TRANSFER
#define MATCHING_SIZES
#define PLATFORM_SUPPORTED
#define HAS_COMMUNICATIONS_EMULATION
#endif

/*  SGI Mips Station  */
#ifdef ARCH_SGImips
#define BLOCK_TRANSFER
#define MATCHING_SIZES
#define PLATFORM_SUPPORTED
#define CAN_FORK
#define HAS_GETRUSAGE
#define HAS_SOCKETS
#define HAS_TERMCAP
#define HAS_SYSV_SHARED_MEMORY
/*#define NEEDS_MISALIGN_COMPILE*/
#define HAS_FORTRAN
#define HAS_ATEXIT
#define HAS_INTERNATIONALISATION
#define HAS_ENVIRON
#define HAS_MMAP
#endif

/*  HP/Apollo 9000  */
#ifdef ARCH_hp9000
#define BLOCK_TRANSFER
#define MATCHING_SIZES
#define PLATFORM_SUPPORTED
#define CAN_FORK
#define HAS_SOCKETS
#define HAS_TERMCAP
#define HAS_SYSV_SHARED_MEMORY
#define HAS_ATEXIT
#define HAS_INTERNATIONALISATION
#define HAS_ENVIRON
#endif


/*  Special consideration for machines which only do byte swapping  */
/*  DEC Mips workstation  */
#ifdef ARCH_dec
#define BYTE_SWAPPER
#define MATCHING_SIZES
#define PLATFORM_SUPPORTED
#define CAN_FORK
#define HAS_GETRUSAGE
#define HAS_SOCKETS
#define HAS_TERMCAP
#define HAS_SYSV_SHARED_MEMORY
#define HAS_ATEXIT
#define HAS_INTERNATIONALISATION
#define HAS_ENVIRON
#define HAS_MMAP
#endif

/*  IBM PC with 80386 processor running in full, 32 bit 386 mode under
    MS-DOS  */
#ifdef ARCH_MS_DOS_386
#define BYTE_SWAPPER
#define MATCHING_SIZES
#define PLATFORM_SUPPORTED
#define HAS_ENVIRON
#endif

/*  IBM PC with 80386 processor running in full, 32 bit 386 mode under linux */
#ifdef ARCH_linux
#define BYTE_SWAPPER
#define MATCHING_SIZES
#define CAN_FORK
#define HAS_GETRUSAGE
#define HAS_SOCKETS
#define HAS_TERMCAP
#define HAS_SYSV_SHARED_MEMORY
#define PLATFORM_SUPPORTED
#define HAS_ON_EXIT
#define HAS_INTERNATIONALISATION
#define HAS_ENVIRON
#endif

/*  Phantom machine  */
#ifdef ARCH_phantom
#define BYTE_SWAPPER
#define MATCHING_SIZES
#define PLATFORM_SUPPORTED
#define HAS_ENVIRON
#endif

/*  All other machines will require some special conversion routines  */
/*  Cray Y-MP  */
#ifdef ARCH_cray
#define PLATFORM_SUPPORTED
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
#endif  /*  ARCH_cray  */


/*  The platform should be supported by now  */
#ifndef PLATFORM_SUPPORTED
/*  Platform has not been supported  */
    !!!! ERROR !!! *** Platform not supported ****
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
