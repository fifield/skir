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
#include "tbb/tick_count.h"
#include "tbb/atomic.h"

#include <cstdio>

#define N 1000
#define C 10

template< typename T >
struct seq_inspector {
    size_t operator()(T &v) const { return size_t(v); }
};

template< typename T >
bool wait_try_get( tbb::graph &g, tbb::sequencer_node<T> &q, T &value ) {
    g.wait_for_all();
    return q.try_get(value);
}

template< typename T >
void spin_try_get( tbb::queue_node<T> &q, T &value ) {
    while ( q.try_get(value) != true ) ;
}

template< typename T >
struct parallel_puts : NoAssign {

    tbb::sequencer_node<T> &my_q;
    int my_num_threads;

    parallel_puts( tbb::sequencer_node<T> &q, int num_threads ) : my_q(q), my_num_threads(num_threads) {}

    void operator()(int tid) const {
        for (int j = tid; j < N; j+=my_num_threads) {
            bool msg = my_q.try_put( T(j) );
            ASSERT( msg == true, NULL );
        }
    }

};

template< typename T >
struct touches {

    bool **my_touches;
    T *my_last_touch;
    int my_num_threads;

    touches( int num_threads ) : my_num_threads(num_threads) {
        my_last_touch = new T[my_num_threads]; 
        my_touches = new bool* [my_num_threads]; 
        for ( int p = 0; p < my_num_threads; ++p) {
            my_last_touch[p] = T(-1);
            my_touches[p] = new bool[N]; 
            for ( int n = 0; n < N; ++n)
                my_touches[p][n] = false;
        }
    }

    ~touches() {
        for ( int p = 0; p < my_num_threads; ++p) {
            delete [] my_touches[p];
        }
        delete [] my_touches;
        delete [] my_last_touch;
    }

    bool check( int tid, T v ) {
        if ( my_touches[tid][v] != false ) {
            printf("Error: value seen twice by local thread\n");
            return false;
        }
        if ( v <= my_last_touch[tid] ) {
            printf("Error: value seen in wrong order by local thread\n");
            return false;
        }
        my_last_touch[tid] = v; 
        my_touches[tid][v] = true; 
        return true;
    }

    bool validate_touches() {
        bool *all_touches = new bool[N];
        for ( int n = 0; n < N; ++n)
            all_touches[n] = false;
     
        for ( int p = 0; p < my_num_threads; ++p) {
            for ( int n = 0; n < N; ++n) {
                if ( my_touches[p][n] == true ) {
                    ASSERT( all_touches[n] == false, "value see by more than one thread\n" );
                    all_touches[n] = true;
                }
            }
        }
        for ( int n = 0; n < N; ++n) {
            if ( !all_touches[n] )
                printf("No touch at %d, my_num_threads = %d\n", n, my_num_threads);
            //ASSERT( all_touches[n] == true, "value not seen by any thread\n" );
        }
        delete all_touches;
        return true;
    }

};

template< typename T >
struct parallel_gets : NoAssign {

    tbb::sequencer_node<T> &my_q;
    int my_num_threads;
    touches<T> &my_touches;

    parallel_gets( tbb::sequencer_node<T> &q, int num_threads, touches<T> &t ) : my_q(q), my_num_threads(num_threads), my_touches(t) {}

    void operator()(int tid) const {
        for (int j = tid; j < N; j+=my_num_threads) {
            T v;
            spin_try_get( my_q, v );    
            my_touches.check( tid, v );
        }
    }

};

template< typename T >
struct parallel_put_get : NoAssign {

    tbb::sequencer_node<T> &my_s1;
    tbb::sequencer_node<T> &my_s2;
    int my_num_threads;
    tbb::atomic< int > &my_counter;    
    touches<T> &my_touches; 

    parallel_put_get( tbb::sequencer_node<T> &s1, tbb::sequencer_node<T> &s2, int num_threads, 
                      tbb::atomic<int> &counter, touches<T> &t ) : my_s1(s1), my_s2(s2), my_num_threads(num_threads), my_counter(counter), my_touches(t) {}

    void operator()(int tid) const {
        int i_start = 0;

        while ( (i_start = my_counter.fetch_and_add(C)) < N ) {
            int i_end = ( N < i_start + C ) ? N : i_start + C;
            for (int i = i_start; i < i_end; ++i) {
                bool msg = my_s1.try_put( T(i) );
                ASSERT( msg == true, NULL );
            }

            for (int i = i_start; i < i_end; ++i) {
                T v;
                spin_try_get( my_s2, v );    
                my_touches.check( tid, v );
            }
        }
    }

};

// 
// Tests
//
// multiple parallel senders, multiple receivers, properly sequenced (relative to receiver) at output
// chained sequencers, multiple parallel senders, multiple receivers, properly sequenced (relative to receiver) at output
//

template< typename T >
int test_parallel(int num_threads) {
    tbb::graph g;

    tbb::sequencer_node<T> s(g, seq_inspector<T>());
    NativeParallelFor( num_threads, parallel_puts<T>(s, num_threads) );
    {
        touches<T> t( num_threads );
        NativeParallelFor( num_threads, parallel_gets<T>(s, num_threads, t) );
        g.wait_for_all();
        ASSERT( t.validate_touches(), NULL );
    }
    T bogus_value(-1);
    T j = bogus_value;
    ASSERT( s.try_get( j ) == false, NULL );
    ASSERT( j == bogus_value, NULL );
    g.wait_for_all();

    tbb::sequencer_node<T> s1(g, seq_inspector<T>());
    tbb::sequencer_node<T> s2(g, seq_inspector<T>());
    tbb::sequencer_node<T> s3(g, seq_inspector<T>());
    ASSERT( s1.register_successor( s2 ) == true, NULL );
    ASSERT( s2.register_successor( s3 ) == true, NULL );

    {
        touches<T> t( num_threads );
        tbb::atomic<int> counter;
        counter = 0;
        NativeParallelFor( num_threads, parallel_put_get<T>(s1, s3, num_threads, counter, t) );
        g.wait_for_all();
        t.validate_touches();
    }
    g.wait_for_all();
    ASSERT( s1.try_get( j ) == false, NULL );
    g.wait_for_all();
    ASSERT( s2.try_get( j ) == false, NULL );
    g.wait_for_all();
    ASSERT( s3.try_get( j ) == false, NULL );
    ASSERT( j == bogus_value, NULL );
    return 0;
}


// 
// Tests
//
// No predecessors can be registered
// Request from empty buffer fails
// In-order puts, single sender, single receiver, properly sequenced at output
// Reverse-order puts, single sender, single receiver, properly sequenced at output
// Chained sequencers (3), in-order and reverse-order tests, properly sequenced at output
//

template< typename T >
int test_serial() {
    tbb::graph g;
    T bogus_value(-1);

    tbb::sequencer_node<T> s(g, seq_inspector<T>());
    tbb::sequencer_node<T> s2(g, seq_inspector<T>());
    T j = bogus_value;

    //
    // Rejects attempts to add / remove predecessor
    // Rejects request from empty Q
    //
    ASSERT( s.register_predecessor( s2 ) == false, NULL );
    ASSERT( s.remove_predecessor( s2 ) == false, NULL );
    ASSERT( s.try_get( j ) == false, NULL );
    ASSERT( j == bogus_value, NULL );

    //
    // In-order simple puts and gets
    //

    for (int i = 0; i < N; ++i) {
        bool msg = s.try_put( T(i) );
        ASSERT( msg == true, NULL );
    }


    for (int i = 0; i < N; ++i) {
        j = bogus_value;
        ASSERT(wait_try_get( g, s, j ) == true, NULL);
        ASSERT( i == j, NULL );
    }
    j = bogus_value;
    g.wait_for_all();
    ASSERT( s.try_get( j ) == false, NULL );
    ASSERT( j == bogus_value, NULL );

    //
    // Reverse-order simple puts and gets
    //

    for (int i = N-1; i >= 0; --i) {
        bool msg = s2.try_put( T(i) );
        ASSERT( msg == true, NULL );
    }

    for (int i = 0; i < N; ++i) {
        j = bogus_value;
        ASSERT(wait_try_get( g, s2, j ) == true, NULL);
        ASSERT( i == j, NULL );
    }
    j = bogus_value;
    g.wait_for_all();
    ASSERT( s2.try_get( j ) == false, NULL );
    ASSERT( j == bogus_value, NULL );

    //
    // Chained in-order simple puts and gets
    //

    tbb::sequencer_node<T> s3(g, seq_inspector<T>());
    tbb::sequencer_node<T> s4(g, seq_inspector<T>());
    tbb::sequencer_node<T> s5(g, seq_inspector<T>());
    ASSERT( s3.register_successor( s4 ) == true, NULL );
    ASSERT( s4.register_successor( s5 ) == true, NULL );

    for (int i = 0; i < N; ++i) {
        bool msg = s3.try_put( T(i) );
        ASSERT( msg == true, NULL );
    }

    for (int i = 0; i < N; ++i) {
        j = bogus_value;
        ASSERT(wait_try_get( g, s5, j ) == true, NULL);
        ASSERT( i == j, NULL );
    }
    j = bogus_value;
    ASSERT( wait_try_get( g, s3, j ) == false, NULL );
    ASSERT( wait_try_get( g, s4, j ) == false, NULL );
    ASSERT( wait_try_get( g, s5, j ) == false, NULL );
    ASSERT( j == bogus_value, NULL );

    g.wait_for_all();
    ASSERT( s3.remove_successor( s4 ) == true, NULL );
    ASSERT( s3.try_put( N ) == true, NULL );
    ASSERT( wait_try_get( g, s4, j ) == false, NULL );
    ASSERT( j == bogus_value, NULL );
    ASSERT( wait_try_get( g, s5, j ) == false, NULL );
    ASSERT( j == bogus_value, NULL );
    ASSERT( wait_try_get( g, s3, j ) == true, NULL );
    ASSERT( j == N, NULL );

    //
    // Chained reverse-order simple puts and gets
    //

    tbb::sequencer_node<T> s6(g, seq_inspector<T>());
    tbb::sequencer_node<T> s7(g, seq_inspector<T>());
    tbb::sequencer_node<T> s8(g, seq_inspector<T>());
    ASSERT( s6.register_successor( s7 ) == true, NULL );
    ASSERT( s7.register_successor( s8 ) == true, NULL );

    for (int i = N-1; i >= 0; --i) {
        bool msg = s6.try_put( T(i) );
        ASSERT( msg == true, NULL );
    }

    for (int i = 0; i < N; ++i) {
        j = bogus_value;
        ASSERT( wait_try_get( g, s8, j ) == true, NULL );
        ASSERT( i == j, NULL );
    }
    j = bogus_value;
    ASSERT( wait_try_get( g, s6, j ) == false, NULL );
    ASSERT( wait_try_get( g, s7, j ) == false, NULL );
    ASSERT( wait_try_get( g, s8, j ) == false, NULL );
    ASSERT( j == bogus_value, NULL );

    g.wait_for_all();
    ASSERT( s6.remove_successor( s7 ) == true, NULL );
    ASSERT( s6.try_put( N ) == true, NULL );
    ASSERT( wait_try_get( g, s7, j ) == false, NULL );
    ASSERT( j == bogus_value, NULL );
    ASSERT( wait_try_get( g, s8, j ) == false, NULL );
    ASSERT( j == bogus_value, NULL );
    ASSERT( wait_try_get( g, s6, j ) == true, NULL );
    ASSERT( j == N, NULL );

    return 0;
}

int TestMain() { 
    tbb::tick_count start = tbb::tick_count::now(), stop;
    for (int p = 1; p < 4; ++p) {
        tbb::task_scheduler_init init(p);
        test_serial<int>();
        test_parallel<int>(p);
    } 
    stop = tbb::tick_count::now();
    REMARK("Sequencer_Node Time=%6.6f\n", (stop-start).seconds());
    return Harness::Done;
}
