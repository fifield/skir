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
#include "tbb/task_scheduler_init.h"

#define N 1000
#define T 1000

//
// Tests
//
// Single reservable predecessor at each port, single accepting successor
//   * put to buffer before port0, then put to buffer before port1
//   * fill buffer before port0 then fill buffer before port1
//

template<typename T1, typename T2>
int test_serial( ) {
    tbb::graph g;
    tbb::join_node< T1, T2 > join(g);

    tbb::queue_node<T1> q1(g); 
    tbb::queue_node<T2> q2(g); 
    typedef typename tbb::join_node<T1,T2>::output_type q3_input_type;
    tbb::queue_node< q3_input_type >  q3(g); 

    q1.register_successor( std::get<0>(join.inputs() ) );
    q2.register_successor( std::get<1>(join.inputs() ) );
    join.register_successor( q3 );

    T1 i;
    T2 j;

    for ( i = T1(0); i < T1(N); ++i ) {
       j = T2(N) + T2(i);
       q1.try_put(i); 
       q2.try_put(j); 
    }

    g.wait_for_all();
    for ( i = T1(0); i < T1(N); ++i ) {
        j = T2(N) + T2(i);
        q3_input_type c( i, j );
        q3_input_type v;
        g.wait_for_all();
        q3.try_get( v );
        ASSERT( v == c, NULL );
    }

    for ( i = T1(0); i < T1(N); ++i ) {
       q1.try_put(i); 
    }
    for ( i = T1(0); i < T1(N); ++i ) {
       j = T2(N) + T2(i);
       q2.try_put(j); 
    }

    g.wait_for_all();
    for ( i = T1(0); i < T1(N); ++i ) {
        j = T2(N) + T2(i);
        q3_input_type c( i, j );
        q3_input_type v;
        g.wait_for_all();
        q3.try_get( v );
        ASSERT( v == c, NULL );
    }

    return 0;
}

int TestMain() { 
   for (int p = 0; p < 4; ++p) {
       test_serial<int, float>( );
   }
   return Harness::Done;
}

