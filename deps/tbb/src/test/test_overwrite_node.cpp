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
#define TBB_PREVIEW_GRAPH 1
#include "tbb/graph.h"

#include "tbb/task_scheduler_init.h"

#define N 1000
#define T 10
#define M 4

template< typename R >
void simple_read_write_tests() {
    tbb::overwrite_node<R> n;

    for ( int t = 0; t < T; ++t ) {
        harness_counting_receiver<R> r[M];

        for (int i = 0; i < M; ++i) {
           ASSERT( n.register_successor(r[i]), NULL );
        }
        R v0;
        ASSERT( n.is_valid() == false, NULL );
        ASSERT( n.try_get( v0 ) == false, NULL );

        for (int i = 0; i < N; ++i ) {
            R v1(static_cast<R>(i));
            ASSERT( n.try_put( v1 ), NULL );
            ASSERT( n.is_valid() == true, NULL );
            for (int j = 0; j < N; ++j ) {
                R v2(0);
                ASSERT( n.try_get( v2 ), NULL );
                ASSERT( v1 == v2, NULL );
            }
        }
        for (int i = 0; i < M; ++i) {
             size_t c = r[i].my_count;
             ASSERT( int(c) == N, NULL );
        }
        for (int i = 0; i < M; ++i) {
           ASSERT( n.remove_successor(r[i]), NULL );
        }
        ASSERT( n.try_put( R(0) ), NULL );
        for (int i = 0; i < M; ++i) {
             size_t c = r[i].my_count;
             ASSERT( int(c) == N, NULL );
        }
        n.clear();
        ASSERT( n.is_valid() == false, NULL );
        ASSERT( n.try_get( v0 ) == false, NULL );
    }
}

template< typename R >
class native_body : NoAssign {
    tbb::overwrite_node<R> &my_node;

public:

     native_body( tbb::overwrite_node<R> &n ) : my_node(n) {}

     void operator()( int i ) const {
         R v1(static_cast<R>(i));
         ASSERT( my_node.try_put( v1 ), NULL );
         ASSERT( my_node.is_valid() == true, NULL );
     }
};

template< typename R >
void parallel_read_write_tests() {
    tbb::overwrite_node<R> n;

    for ( int t = 0; t < T; ++t ) {
        harness_counting_receiver<R> r[M];

        for (int i = 0; i < M; ++i) {
           ASSERT( n.register_successor(r[i]), NULL );
        }
        R v0;
        ASSERT( n.is_valid() == false, NULL );
        ASSERT( n.try_get( v0 ) == false, NULL );

        NativeParallelFor( N, native_body<R>( n ) );

        for (int i = 0; i < M; ++i) {
             size_t c = r[i].my_count;
             ASSERT( int(c) == N, NULL );
        }
        for (int i = 0; i < M; ++i) {
           ASSERT( n.remove_successor(r[i]), NULL );
        }
        ASSERT( n.try_put( R(0) ), NULL );
        for (int i = 0; i < M; ++i) {
             size_t c = r[i].my_count;
             ASSERT( int(c) == N, NULL );
        }
        n.clear();
        ASSERT( n.is_valid() == false, NULL );
        ASSERT( n.try_get( v0 ) == false, NULL );
    }
}

int TestMain() { 
    if( MinThread<1 ) {
        REPORT("number of threads must be positive\n");
        exit(1);
    }
    simple_read_write_tests<int>();
    simple_read_write_tests<float>();
    for( int p=MinThread; p<=MaxThread; ++p ) {
        tbb::task_scheduler_init init(p);
        parallel_read_write_tests<int>();
        parallel_read_write_tests<float>();
    }
    return Harness::Done;
}

