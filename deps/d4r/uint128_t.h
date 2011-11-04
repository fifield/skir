//=============================================================================
//	Computational Process Networks class library
//	Copyright (C) 1997-2006  Gregory E. Allen and The University of Texas
//
//	This library is free software; you can redistribute it and/or modify it
//	under the terms of the GNU Library General Public License as published
//	by the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This library is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//	Library General Public License for more details.
//
//	The GNU Public License is available in the file LICENSE, or you
//	can write to the Free Software Foundation, Inc., 59 Temple Place -
//	Suite 330, Boston, MA 02111-1307, USA, or you can find it on the
//	World Wide Web at http://www.fsf.org.
//=============================================================================
/** \file
 * \author John Bridgman
 * \brief This is a very simple implementation of a 128 bit number
 *
 * No arithmatic operators are provided as they aren't currently needed.
 * Maybe if they are needed add them.
 */
#ifndef UINT128_T_H
#define UINT128_T_H
#pragma once
#include <stdint.h>
struct uint128_t {
    uint128_t(uint64_t h, uint64_t l) : high(h), low(l) {}
    uint128_t(uint64_t v) : high(0), low(v) {}
    uint128_t() : high(0), low(0) {}
    uint64_t high, low;
};

inline bool operator<(const uint128_t &l, const uint128_t &r) {
    if (l.high == r.high) { return l.low < r.low; }
    else { return l.high < r.high; }
}
inline bool operator>=(const uint128_t &l, const uint128_t &r) { return !(l < r); }
inline bool operator<=(const uint128_t &l, const uint128_t &r) { return !(r < l); }
inline bool operator>(const uint128_t &l, const uint128_t &r) { return (r < l); }
inline bool operator==(const uint128_t &l, const uint128_t &r) {
    return (l.high == r.high && l.low == r.low);
}
inline bool operator!=(const uint128_t &l, const uint128_t &r) { return !(r == l); }
#endif
