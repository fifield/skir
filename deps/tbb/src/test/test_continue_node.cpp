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

#define N 100
#define T 10

struct counting_receiver : public tbb::receiver<tbb::continue_msg> {

    typedef tbb::continue_msg input_type;
    typedef tbb::sender<tbb::continue_msg> predecessor_type;

    tbb::atomic< int > my_count;

    counting_receiver() { my_count = 0; }    

    bool try_put( input_type ) { 
        ++my_count; 
        return true;  
    }

};

#if !__TBB_LAMBDAS_PRESENT
class native_body : tbb::internal::no_assign {
    tbb::continue_node &my_node;

public:

     native_body( tbb::continue_node &n ) : my_node(n) {}
     void operator()( int ) const { 
         my_node.try_put( tbb::continue_msg() ); 
     } 
};
#endif

// Tests
//  
// Test that the node fires after receiving N messages from multiple senders
// Test that the node resets and can be reused
// Test that a successor added after triggering is NOT immediately signalled
// 
int test_parallel_puts() {
    tbb::graph g;
    tbb::continue_node n(g);
    counting_receiver r;
    counting_receiver r2;

    n.register_successor(r);

    for (int i = 0; i < N; ++i) 
        n.register_predecessor( n ); // this is a trick since the predecessor doesn't matter

    for (int t = 0; t < T; ++t)
#if __TBB_LAMBDAS_PRESENT
        NativeParallelFor( N, [&](int) { n.try_put( tbb::continue_msg() ); } );
#else
        NativeParallelFor( N, native_body(n) );
#endif

    g.wait_for_all();
    ASSERT( r.my_count == T, NULL ); 


    n.register_successor(r2);
    ASSERT( r2.my_count == 0, NULL );
    return 0; 
}

//
// Tests
//  
// Test that the node fires after receiving N messages from a single sender
// Test that the node resets and can be reused
// Test that a successor added after triggering is NOT immediately signalled
// 
int test_serial_puts() {
    tbb::graph g;
    tbb::continue_node n(g);
    counting_receiver r;
    counting_receiver r2;

    n.register_successor(r);

    for (int i = 0; i < N; ++i) 
        n.register_predecessor( n ); // this is a trick since the predecessor doesn't matter

    for (int t = 0; t < T; ++t)
        for (int i = 0; i < N; ++i)
            n.try_put( tbb::continue_msg() );

    g.wait_for_all();
    ASSERT( r.my_count == T, NULL ); 

    n.register_successor(r2);
    ASSERT( r2.my_count == 0, NULL );
    return 0; 
}

int TestMain() { 
   test_serial_puts();
   test_parallel_puts();
   return Harness::Done;
}

