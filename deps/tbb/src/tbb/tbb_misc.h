/*
    Copyright 2005-2010 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

#ifndef _TBB_tbb_misc_H
#define _TBB_tbb_misc_H

#include "tbb/tbb_stddef.h"
#include "tbb/tbb_machine.h"
#include "tbb/atomic.h"     // For atomic_xxx definitions

namespace tbb {
namespace internal {

const size_t MByte = 1<<20;

#if !defined(__TBB_WORDSIZE)
    const size_t ThreadStackSize = 1*MByte;
#elif __TBB_WORDSIZE<=4
    const size_t ThreadStackSize = 2*MByte;
#else
    const size_t ThreadStackSize = 4*MByte;
#endif


#ifndef __TBB_HardwareConcurrency

//! Returns maximal parallelism level supported by the current OS configuration.
int AvailableHwConcurrency();

#else

inline int AvailableHwConcurrency() {
    int n = __TBB_HardwareConcurrency();
    return n > 0 ? n : 1; // Fail safety strap
}
#endif /* __TBB_HardwareConcurrency */


#if _WIN32||_WIN64

//! Returns number of processor groups in the current OS configuration.
/** AvailableHwConcurrency must be called at least once before calling this method. **/
int NumberOfProcessorGroups();

//! Retrieves index of processor group containing processor with the given index
int FindProcessorGroupIndex ( int processorIndex );

//! Affinitizes the thread to the specified processor group
void MoveThreadIntoProcessorGroup( void* hThread, int groupIndex );

#endif /* _WIN32||_WIN64 */

//! Throws std::runtime_error with what() returning error_code description prefixed with aux_info
void handle_win_error( int error_code );

//! True if environment variable with given name is set and not 0; otherwise false.
bool GetBoolEnvironmentVariable( const char * name );

//! Prints TBB version information on stderr
void PrintVersion();

//! Prints arbitrary extra TBB version information on stderr
void PrintExtraVersionInfo( const char* category, const char* description );

//! A callback routine to print RML version information on stderr
void PrintRMLVersionInfo( void* arg, const char* server_info );

// For TBB compilation only; not to be used in public headers
#if defined(min) || defined(max)
#undef min
#undef max
#endif

//! Utility template function returning lesser of the two values.
/** Provided here to avoid including not strict safe <algorithm>.\n
    In case operands cause signed/unsigned or size mismatch warnings it is caller's
    responsibility to do the appropriate cast before calling the function. **/
template<typename T1, typename T2>
T1 min ( const T1& val1, const T2& val2 ) {
    return val1 < val2 ? val1 : val2;
}

//! Utility template function returning greater of the two values.
/** Provided here to avoid including not strict safe <algorithm>.\n
    In case operands cause signed/unsigned or size mismatch warnings it is caller's
    responsibility to do the appropriate cast before calling the function. **/
template<typename T1, typename T2>
T1 max ( const T1& val1, const T2& val2 ) {
    return val1 < val2 ? val2 : val1;
}

//------------------------------------------------------------------------
// FastRandom
//------------------------------------------------------------------------

/** Defined in tbb_main.cpp **/
unsigned GetPrime ( unsigned seed );

//! A fast random number generator.
/** Uses linear congruential method. */
class FastRandom {
    unsigned x, a;
public:
    //! Get a random number.
    unsigned short get() {
        return get(x);
    }
    //! Get a random number for the given seed; update the seed for next use.
    unsigned short get( unsigned& seed ) {
        unsigned short r = (unsigned short)(seed>>16);
        seed = seed*a+1;
        return r;
    }
    //! Construct a random number generator.
    FastRandom( unsigned seed ) {
        x = seed;
        a = GetPrime( seed );
    }
};

//------------------------------------------------------------------------
// Atomic extensions
//------------------------------------------------------------------------

//! Atomically replaces value of dst with newValue if they satisfy condition of compare predicate
/** Return value semantics is the same as for CAS. **/
template<typename T1, typename T2, class Pred> 
T1 atomic_update ( tbb::atomic<T1>& dst, const T2& newValue, Pred compare ) {
    T1 oldValue = dst;
    while ( compare(oldValue, newValue) ) {
        if ( dst.compare_and_swap(newValue, oldValue) == oldValue )
            break;
        oldValue = dst;
    }
    return oldValue;
}

//! One-time initialization states
enum do_once_state {
    do_once_fallow = 0, ///< No execution attempts have been undertaken yet
    do_once_pending,    ///< A thread is executing associated do-once routine
    do_once_executed,   ///< Do-once routine has been executed
    initialization_complete = do_once_executed  ///< Convenience alias
};

//! One-time initialization function
/** /param initializer Either pointer to function without arguments or functor 
            object with operator function without arguments.
    /param state Shared state associated with initializer that specifies its 
            initialization state. Must be initially set to #uninitialized value
            (e.g. by means of default static zero initialization). **/
template <typename F>
void atomic_do_once ( const F& initializer, atomic<do_once_state>& state ) {
    // tbb::atomic provides necessary acquire and release fences.
    if( state != do_once_executed ) {
        if( state == do_once_fallow ) {
            if( state.compare_and_swap( do_once_pending, do_once_fallow ) == do_once_fallow ) {
                initializer();
                state = do_once_executed;
            }
        }
        else
            spin_wait_until_eq( state, do_once_executed );
    }
}

} // namespace internal
} // namespace tbb

#endif /* _TBB_tbb_misc_H */
