#if !defined(TYPES_H)
#define TYPES_H
/*
**
**	$Id: Types.h,v 1.3 1998/10/25 16:05:52 bobak Exp $
**	$Revision: 1.3 $
**
**	$Filename: Types.h $
**	$Date: 1998/10/25 16:05:52 $
**
**	Author: Andreas F. Bobak (bobak@abstrakt.ch)
**
**	Basic datatype definitions
**
**	Copyright (C) 1996-1998 by Abstrakt Design, Andreas F. Bobak.
**
**	This is free software; you can redistribute it and / or modify it 
**	under the terms of the GNU General Public License as published by
**	the Free Software Foundation; either version 2 of the License, or
**	(at your option) any later version.
**
**	This software is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY;  without even the  implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
**	General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this software; if not, write to the Free Software
**	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
**	USA.
**
*/

typedef   signed char	s8;			// signed 8-bit quantity
typedef unsigned char	u8;			// unsigned 8-bit quantity
typedef   signed short	s16;		// signed 16-bit quantity
typedef unsigned short	u16;		// unsigned 16-bit quantity
typedef   signed int	s32;		// signed 32-bit quantity
typedef unsigned int	u32;		// unsigned 32-bit quantity

typedef   signed char	schar;		// signed character quantity
typedef unsigned char	uchar;		// unsigned character quantity
typedef   signed short	sshort;		// signed short integer quantity
typedef unsigned short	ushort;		// unsigned short integer quantity
typedef   signed int	sint;		// signed integer quantity
typedef unsigned int	uint;		// unsigned integer quantity
typedef	  signed long	slong;		// signed long integer quantity
typedef unsigned long	ulong;		// unsigned long integer quantity

#ifdef _MSC_VER
#pragma warning( disable: 4237 )
#undef bool
#undef true
#undef false
enum bool{ true = 1, false = 0 };
#endif

/*
**	swap a 32 bit quantity
*/
inline s32
swap32( s32 lw )
{
	s16	wl	= (s16)(lw & 0x0000FFFF);
	s16	wu	= (s16)(lw >> 16);
	u8	bl, bu;

	bl	= (u8)(wl & 0x00FF);
	bu	= (u8)(wl >> 8);
	wl	= (s16)(bu + (bl << 8));

	bl	= (u8)(wu & 0x00FF);
	bu	= (u8)(wu >> 8);
	wu	= (s16)(bu + (bl << 8));

	return (wu + (wl << 16));
}

/*
**	swap a 16 bit quantity
*/
inline s16
swap16( s16 w )
{
	u8	bl	= (u8)(w & 0x00FF);
	u8	bu	= (u8)(w >> 8);

	return (s16)(bu + (bl << 8));
}


/*
**	swap two pointers
*/
template <class Type> inline void
swap( Type& a, Type& b )
{
	Type t = a;
	a = b;
	b = t;
}

/*
**	null pointer
*/
#ifndef NULL
#define NULL	0L
#endif

#endif
