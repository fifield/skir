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

#include "harness_graph.h"

#include "tbb/task_scheduler_init.h"
#include "tbb/spin_mutex.h"

tbb::spin_mutex global_mutex;

#define N 1000
#define MAX_NODES 4
#define C 8 

struct empty_no_assign : private NoAssign { 
   empty_no_assign() {}
   empty_no_assign( int ) {}
   operator int() { return 0; }
};

template< typename InputType >
struct parallel_puts : private NoAssign {

    tbb::receiver< InputType > * const my_exe_node;

    parallel_puts( tbb::receiver< InputType > &exe_node ) : my_exe_node(&exe_node) {}

    void operator()( int ) const  {
        for ( int i = 0; i < N; ++i ) {
            // the nodes will accept all puts
            ASSERT( my_exe_node->try_put( InputType() ) == true, NULL );
        }
    }

};

template< typename OutputType, typename Body >
void continue_nodes( Body body ) {

    for (int p = 1; p < 2*MaxThread; ++p) {
        tbb::graph g;
        tbb::executable_node< OutputType > exe_node( g, body );
        for (size_t i = 0; i < N; ++i) {
           exe_node.register_predecessor( *reinterpret_cast< tbb::sender< tbb::continue_msg > * >(&exe_node) );
        }

        for (size_t num_receivers = 1; num_receivers <= MAX_NODES; ++num_receivers ) { 
            harness_counting_receiver<OutputType> *receivers = new harness_counting_receiver<OutputType>[num_receivers];
            harness_graph_executor<tbb::continue_msg, OutputType>::execute_count = 0;

            for (size_t r = 0; r < num_receivers; ++r ) {
                ASSERT( exe_node.register_successor( receivers[r] ), NULL );
            }

            NativeParallelFor( p, parallel_puts<tbb::continue_msg>(exe_node) );
            g.wait_for_all(); 

            // 2) the nodes will receive puts from multiple predecessors simultaneously,
            size_t ec = harness_graph_executor<tbb::continue_msg, OutputType>::execute_count;
            ASSERT( (int)ec == p, NULL ); 
            for (size_t r = 0; r < num_receivers; ++r ) {
                size_t c = receivers[r].my_count;
                // 3) the nodes will send to multiple successors.
                ASSERT( (int)c == p, NULL );
            }
        }
    }
}

template< typename OutputType >
void run_continue_nodes() {
    harness_graph_executor< tbb::continue_msg, OutputType>::max_executors = 0;
    #if __TBB_LAMBDAS_PRESENT
    continue_nodes<OutputType>( []( tbb::continue_msg i ) -> OutputType { return harness_graph_executor<tbb::continue_msg, OutputType>::func(i); } );
    #endif
    continue_nodes<OutputType>( &harness_graph_executor<tbb::continue_msg, OutputType>::func );
    continue_nodes<OutputType>( typename harness_graph_executor<tbb::continue_msg, OutputType>::functor() );
}

//! Tests limited concurrency cases for nodes that accept data messages
void test_concurrency(int num_threads) {
    tbb::task_scheduler_init init(num_threads);
    run_continue_nodes<tbb::continue_msg>();
    run_continue_nodes<int>();
    run_continue_nodes<empty_no_assign>();
}

int TestMain() { 
    if( MinThread<1 ) {
        REPORT("number of threads must be positive\n");
        exit(1);
    }
    for( int p=MinThread; p<=MaxThread; ++p ) {
       test_concurrency(p);
   }
   return Harness::Done;
}

