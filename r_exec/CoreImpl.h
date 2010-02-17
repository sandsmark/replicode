/*
 *  CoreImpl.h
 *  rsystem
 *
 *  Created by Nathaniel Thurston on 23/12/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __CORE_IMPL_H
#define __CORE_IMPL_H
#include "Core.h"
#include "hash_containers"
#include "../r_code/atom.h"
#include "mbrane_imports/utils.h"
#include <deque>
#include "ReductionInstance.h"

namespace r_exec {

namespace CoreImpl {
	struct Impl;
	struct Instance;
	
	struct Instance : public Core::Instance
	{
		// ===== Interface =====
		struct Job {
			virtual void process() = 0; // process is multithread-safe
			virtual void finish() = 0; // finish() is not multithread-safe;
				// calls to finish() must be externally serialized.
		};

		Instance(Group* group_, Impl* core_);
		~Instance();
		void activate(Object* program);
		void deactivate(Object* program);
		void salientObject(Object* object);
		Job* nextJob();
		
		// ===== Structures =====
		
		struct Program;
		
		struct InputMatcher {
			ReductionInstance* inputPattern;
			struct Output {
				Program *program;
				int inputIndex;
			};
			typedef std::vector<Output> OutputSet;
			OutputSet outputs;
			~InputMatcher() { inputPattern->release(); }
		};
		
		// TODO: anti-program
		struct Program {
			struct MatchedInputs {
				int64 creationTime;
				std::vector<ReductionInstance*> matches;
			};
			ReductionInstance programRI;
			std::vector<InputMatcher*> inputMatchers;
			std::vector<MatchedInputs> matchSets;
			int64 timeScope;
			// signalling period isn't in the notes... what happened?
		};

		struct InputJob : public Job {
			Instance* instance;
			InputMatcher* matcher;
			Object* input;
			ReductionInstance* result;
			void process();
			void finish();
		};
		
		struct OverlayJob : public Job {
			Instance* instance;
			ReductionInstance* programRI;
			std::vector<ReductionInstance*> inputs;
			ReductionInstance* result;
			void process();
			void finish();
		};

		struct InputQueueEntry {
			Object *object;
			enum InputType { ACTIVATION, DEACTIVATION, SALIENT } type;
			InputQueueEntry(Object *o, InputType t) :object(o), type(t) {} 
		};
		
		typedef hash_map<Object*, Program> ProgramHash;
		typedef hash_map<ReductionInstance*, InputMatcher, ReductionInstance::ptr_hash> InputHash;
		typedef	hash_multimap<int, InputMatcher*> InputTable;
		typedef std::deque<InputQueueEntry> InputQueue;
		typedef std::deque<InputJob> InputJobQueue;
		typedef std::deque<OverlayJob> OverlayJobQueue;
		
		ProgramHash programs;
		InputHash inputMatchers;
		InputTable inputTable;
		InputQueue inputQueue;
		InputJobQueue inputJobQueue;
		OverlayJobQueue overlayJobQueue;
			
		Impl* core;
		Group* group;
		bool isActive; // an active instance is one with a non-empty input,
			// input job, or overlay job queue.  The Core processes active
			// Instances in FIFO order.
			
		// ====== Internal Methods ======
		void onInput(const InputQueueEntry& e);
		void doActivate(Object* program);
		void doDeactivate(Object* program);
		void doSalient(Object* object);
		void onProgramInput(Program* program, int index, ReductionInstance* input);
		void createOverlays(Program* program);
	};
		
	struct Impl : public Core
	{
		// implementation of interface
		Impl();
		~Impl();
		void resume();
		void suspend();
		void doWork();
		static thread_ret thread_function_call work(void* args);
		Core::Instance* createInstance(Group* group);
		
		hash_set<Instance*> instances;
		
		bool suspendRequested;
		int numRunningThreads;
		mBrane::Mutex runRelease;
		mBrane::Mutex suspended;
		mBrane::Mutex implMutex;
		
		// There are some compelling advantages in having only one thread
		// working on a given Instance:
		//  * no need to synchronize access to the Instance structures
		//  * better core-locality of memory access (this would be further
		//    improved if there were persistent binding of Instance->thread)
		// The downside would be markedly slower throughput of highly-
		// active groups, as only one core would be available.  An advanced
		// implementation might try for the best of both worlds by having
		// single-threaded Instance mode for all but the most-active groups.
		//
		// ...I'm not sure which of the simple options is the best choice
		// for the first implementation, but for the first implementation
		// I'm not doing the one thread/instance idea.
		std::deque<CoreImpl::Instance*> activeInstances;
		std::vector<mBrane::Thread*> workerThreads;
	};
}

}

#endif // __CORE_IMPL_H
