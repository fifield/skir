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
#include "tbb/task.h"
#include "tbb/task_scheduler_init.h"

const int NumRepeats = 200;
const int MaxNumThreads = 16;
static volatile bool Finished[MaxNumThreads] = {};

static volatile bool CanStart;

//! Custom user task interface
class ITask {
public: 
	virtual ~ITask() {} 
	virtual void Execute() = 0;
	virtual void Release() { delete this; }
};

class TestTask : public ITask {
	volatile bool *m_pDone;
public:
    TestTask ( volatile bool *pDone ) : m_pDone(pDone) {}

	/* override */ void Execute() {
		*m_pDone = true;
	}
};

class CarrierTask : public tbb::task {
	ITask* m_pTask;
public:
    CarrierTask(ITask* pTask) : m_pTask(pTask) {}

	/*override*/ task* execute() {
		m_pTask->Execute();
		m_pTask->Release();
		return NULL;
	}
};

class SpawnerTask : public ITask {
	ITask* m_taskToSpawn;
public:
	SpawnerTask(ITask* job) : m_taskToSpawn(job) {}

	void Execute() {
		while ( !CanStart )
		    __TBB_Yield();
		tbb::task::enqueue( *new( tbb::task::allocate_root() ) CarrierTask(m_taskToSpawn) );
	}
};

class EnqueuerBody {
public:
    void operator() ( int id ) const {
	    tbb::task_scheduler_init init(tbb::task_scheduler_init::default_num_threads() + 1);

	    SpawnerTask* pTask = new SpawnerTask( new TestTask(Finished + id) );
	    tbb::task::enqueue( *new( tbb::task::allocate_root() ) CarrierTask(pTask) );
    }
};

//! Regression test for a bug that caused premature arena destruction
void TestCascadedEnqueue () {
	tbb::task_scheduler_init init(tbb::task_scheduler_init::default_num_threads() + 1);

    int minNumThreads = min(tbb::task_scheduler_init::default_num_threads(), MaxNumThreads) / 2;
	int maxNumThreads = min(tbb::task_scheduler_init::default_num_threads() * 2, MaxNumThreads);

	for ( int numThreads = minNumThreads; numThreads <= maxNumThreads; ++numThreads ) {
		for ( int i = 0; i < NumRepeats; ++i ) {
			CanStart = false;
            __TBB_Yield();
			NativeParallelFor( numThreads, EnqueuerBody() );
			CanStart = true;
            int j = 0;
			while ( j < numThreads ) {
                if ( Finished[j] )
                    ++j;
                else
    				__TBB_Yield();
			}
            for ( j = 0; j < numThreads; ++j )
                Finished[j] = false;
            REMARK("%02d threads; Iteration %03d\r", numThreads, i);
		}
	}
    REMARK( "                                 \r" );
}

class DummyTask : public tbb::task {
public:
    task *execute() {
        Harness::Sleep(1);
        return NULL;
    }
};

class SharedRootBody {
    tbb::task *my_root;
public:
	SharedRootBody ( tbb::task *root ) : my_root(root) {}

	void operator() ( int ) const {
	    tbb::task::enqueue( *new( tbb::task::allocate_additional_child_of(*my_root) ) DummyTask );
	}
};

//! Test for enqueuing children of the same root from different master threads
void TestSharedRoot () { 
    for ( int p = MinThread; p <= MaxThread; ++p ) {
        tbb::task_scheduler_init init(p);
    	tbb::task *root =  new ( tbb::task::allocate_root() ) tbb::empty_task; 
    	root->set_ref_count(1);
        for( int n = MinThread; n <= MaxThread; ++n )
            NativeParallelFor( n, SharedRootBody(root) );
    	root->wait_for_all();
        tbb::task::destroy(*root);
    } 
}


int TestMain () {
    TestCascadedEnqueue();
    TestSharedRoot();
    return Harness::Done;
}
