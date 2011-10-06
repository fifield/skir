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

#include "harness.h"
#define TBB_PREVIEW_GRAPH 1
#include "tbb/graph.h"

#include "tbb/atomic.h"

const int N = 1000;
const int R = 4;

class int_convertable_type : private NoAssign {

   int my_value;

public:

   int_convertable_type( int v ) : my_value(v) {}
   operator int() { return my_value; }

};


template< typename T >
class counting_array_receiver : public tbb::receiver<T> {

    tbb::atomic<size_t> my_counters[N];

public:

    counting_array_receiver() {
        for (int i = 0; i < N; ++i )
           my_counters[i] = 0;
    }

    size_t operator[]( int i ) {
        size_t v = my_counters[i];
        return v;
    }

    /* override */ bool try_put( T v ) {
        ++my_counters[(int)v];
        return true;
    }

};

template< typename T >
void test_serial_broadcasts() {

    tbb::broadcast_node<T> b;
    
    for ( int num_receivers = 1; num_receivers < R; ++num_receivers ) {
        counting_array_receiver<T> *receivers = new counting_array_receiver<T>[num_receivers];

        for ( int r = 0; r < num_receivers; ++r ) {
            ASSERT( b.register_successor( receivers[r] ), NULL );
        } 

        for (int n = 0; n < N; ++n ) {
            ASSERT( b.try_put( (T)n ), NULL );
        }

        for ( int r = 0; r < num_receivers; ++r ) {
            for (int n = 0; n < N; ++n ) {
                ASSERT( receivers[r][n] == 1, NULL );
            }
            ASSERT( b.remove_successor( receivers[r] ), NULL );
        } 
        ASSERT( b.try_put( (T)0 ), NULL );
        for ( int r = 0; r < num_receivers; ++r ) 
            ASSERT( receivers[0][0] == 1, NULL ) ;

        delete [] receivers;

    }

}

template< typename T >
class native_body : private NoAssign {
 
   tbb::broadcast_node<T> &my_b;

public:

    native_body( tbb::broadcast_node<T> &b ) : my_b(b) {} 

    void operator()(int) const {
        for (int n = 0; n < N; ++n ) {
            ASSERT( my_b.try_put( (T)n ), NULL );
        }
    }
 
};

template< typename T >
void test_parallel_broadcasts(int p) {

    tbb::broadcast_node<T> b;
    
    for ( int num_receivers = 1; num_receivers < R; ++num_receivers ) {
        counting_array_receiver<T> *receivers = new counting_array_receiver<T>[num_receivers];

        for ( int r = 0; r < num_receivers; ++r ) {
            ASSERT( b.register_successor( receivers[r] ), NULL );
        } 

        NativeParallelFor( p, native_body<T>( b ) );

        for ( int r = 0; r < num_receivers; ++r ) {
            for (int n = 0; n < N; ++n ) {
                ASSERT( (int)receivers[r][n] == p, NULL );
            }
            ASSERT( b.remove_successor( receivers[r] ), NULL );
        } 
        ASSERT( b.try_put( (T)0 ), NULL );
        for ( int r = 0; r < num_receivers; ++r ) 
            ASSERT( (int)receivers[0][0] == p, NULL ) ;

        delete [] receivers;

    }

}

int TestMain() { 
    if( MinThread<1 ) {
        REPORT("number of threads must be positive\n");
        exit(1);
    }

   test_serial_broadcasts<int>();
   test_serial_broadcasts<float>();
   test_serial_broadcasts<int_convertable_type>();

   for( int p=MinThread; p<=MaxThread; ++p ) {
       test_parallel_broadcasts<int>(p);
       test_parallel_broadcasts<float>(p);
       test_parallel_broadcasts<int_convertable_type>(p);
   }
   return Harness::Done;

}

