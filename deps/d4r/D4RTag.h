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
 */
#ifndef D4R_TAG_H
#define D4R_TAG_H
#pragma once
#include "uint128_t.h"
/** 
 * \brief D4R algorithm as described in "A Distributed Deadlock Detection And
 * Resolution Algorithm For Process Networks" by Allen, Zucknick and Evans
 */
namespace D4R {

    /**
     * A class to encapsulate the D4R tag.
     *
     * This is implemented with keys as the lower order bits and the counters
     * and queue size as the higher order bits as suggested in the literature.
     * This gives a total order to the Tags so that one and only one of the
     * nodes in the deadlock will detect the deadlock.
     */
    class Tag {
    public:
        Tag(uint64_t k) : label(0, k), priority(uint64_t(-1), k) {}
        Tag() : label(), priority() {}
        /**
         * Perform comparison as specified in "A Distributed Deadlock Detection And
         * Resolution Algorithm For Process Networks" by Allen, Zucknick and Evans
         */
        bool operator<(const Tag &t) const {
            if (label == t.label) {
                return priority > t.priority;
            } else {
                return label < t.label;
            }
        }
        bool operator>=(const Tag &t) const { return !(*this < t); }
        bool operator<=(const Tag &t) const { return !(t < *this); }
        bool operator>(const Tag &t) const { return (t < *this); }
        bool operator==(const Tag &t) const {
            return (label == t.label && priority == t.priority);
        }
        bool operator!=(const Tag &t) const { return !(*this == t); }

        /** The D4R algorithm count @{ */
        uint64_t Count() const { return label.high; }
        uint64_t Count(uint64_t c) { return label.high = c; }
        /** @} */
        /** The key of the tag @{ */
        uint64_t Key() const { return label.low; }
        uint64_t Key(uint64_t k) { return label.low = k; }
        /** @} */
        /** The queue size of the tag. @{ */
        uint64_t QueueSize() const { return priority.high; }
        uint64_t QueueSize(uint64_t qs) { return priority.high = qs; }
        /** @} */
        /** The key of the node blocked on the queue,
         * this provides a total order to the tags. @{ */
        uint64_t QueueKey() const { return priority.low; }
        uint64_t QueueKey(uint64_t k) { return priority.low = k; }
        /** @} */
        const uint128_t &Label() const { return label; }
        const uint128_t &Label(const uint128_t &l) { return label = l; }
        const uint128_t &Priority() const { return priority; }
        const uint128_t &Priority(const uint128_t &p) { return priority = p; }
    private:
        uint128_t label;
        uint128_t priority;
    };

}
#endif
