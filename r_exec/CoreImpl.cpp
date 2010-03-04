#include "CoreImpl.h"
#include "Object.h"
#include "Expression.h"
#include "Group.h"
#include <algorithm>
#include "mbrane_imports/utils.h"
#include "ExecutionContext.h"
#include "match.h"

using namespace std;

namespace r_exec {

namespace CoreImpl {
	const int num_threads = 1; // TODO: increase to 4 for production
	Impl::Impl()
		:suspendRequested(true), numRunningThreads(0)
	{
		runRelease.acquire();
		suspended.acquire();
		for (int i = 0; i < num_threads; ++i) {
			workerThreads.push_back(mBrane::Thread::New<mBrane::Thread>(Impl::work, this));
		}
	}

	void doCopy(ReductionInstance* ri)
	{
		new ReductionInstance(*ri);
	}

	void Impl::doWork()
	{
		for (;;) {
			runRelease.acquire();
			runRelease.release();
			++numRunningThreads;
			CoreImpl::Instance::Job* job = 0;
			bool progress = true;
			while (progress) {
				implMutex.acquire();
				if (job) {
					job->finish();
					job = 0;
				}
				if (suspendRequested || activeInstances.empty()) {
					progress = false;
				} else {
					CoreImpl::Instance* topInstance = activeInstances.front();
					job = topInstance->nextJob();
					if (!job) {
						topInstance->isActive = false;
						activeInstances.pop_front();
					}
				}
				implMutex.release();
				if (job)
					job->process();
			}
			if (--numRunningThreads == 0)
				suspended.release();
		}
	}

	thread_ret thread_function_call Impl::work(void* args)
	{
		Impl* this_ = reinterpret_cast<Impl*>(args);
		this_->doWork();
	}

	Impl::~Impl()
	{
		suspend();
		for (int i = 0; i < workerThreads.size(); ++i)
			delete workerThreads[i];
	}

	void Impl::resume()
	{
		if (suspendRequested) {
			suspendRequested = false;
			if (!activeInstances.empty())
				runRelease.release();
		}
	}

	void Impl::suspend()
	{
		if (!suspendRequested) {
			suspendRequested = true;
			if (numRunningThreads > 0)
				suspended.acquire();
		}
	}

	
	void Instance::signalProgramsWithNoInputs()
	{
		for (CoreImpl::Instance::ProgramHash::iterator itP = programs.begin(); itP != programs.end(); ++itP) {
			Program& p = itP->second;
			if (p.inputMatchers.size() == 0) {
				Instance::OverlayJob j;
				j.instance = this;
				j.programRI = p.programRI;
				j.programRI->retain();
				j.result = 0;
				overlayJobQueue.push_back(j);
			}
		}
	}

	Core::Instance* Impl::createInstance(Group* group)
	{
		group->asObject()->retain();
		return new CoreImpl::Instance(group, this);
	}

	Instance::Instance(Group* group_, Impl* core_)
		:group(group_), core(core_)
	{
		core->instances.insert(this);
	}

	Instance::~Instance()
	{
		group->asObject()->release();
		core->instances.erase(this);
		if (isActive) {
			deque<Instance*>::iterator it = find(core->activeInstances.begin(), core->activeInstances.end(), this);
			core->activeInstances.erase(it);
		}
	}

	Instance::Job* Instance::nextJob()
	{
		if (!overlayJobQueue.empty()) {
			Job* j = new OverlayJob(overlayJobQueue.front());
			printf("created new overlay job %p\n", j);
			overlayJobQueue.pop_front();
			return j;
		}
		while (inputJobQueue.empty()) {
			if (inputQueue.empty())
				return 0;
			InputQueueEntry& e = inputQueue.front();
			switch(e.type) {
				case InputQueueEntry::ACTIVATION:
					doActivate(e.object);
					break;
				case InputQueueEntry::DEACTIVATION:
					doDeactivate(e.object);
					break;
				case InputQueueEntry::SALIENT:
					doSalient(e.object);
					break;
			}
			e.object->release();
			inputQueue.pop_front();
		}
		Job* j = new InputJob(inputJobQueue.front());
		printf("created new input job %p\n", j);
		inputJobQueue.pop_front();
		return j;
	}

	void Instance::onInput(const InputQueueEntry& e)
	{
		inputQueue.push_back(e);
		e.object->retain();
		if (!isActive) {
			isActive = true;
			core->activeInstances.push_back(this);
		}
	}
	
	void Instance::activate(Object* program)
	{
		printf("activate %p, %p\n", this, program);
		onInput(InputQueueEntry(program, InputQueueEntry::ACTIVATION));
	}

	// getInputs creates a vector of ReductionInstance pointers corresponding
	// to each of the inputs for the supplied program.  The entries start
	// with refcount == 0, and it's the responsibility of the caller to
	// either retain or destroy them.
	vector<ReductionInstance*> getInputs(ReductionInstance& ri)
	{
		vector<ReductionInstance*> result;
		Expression head(&ri);
		Expression inputs = head.child(1).child(2).child(1);
		int numInputs = inputs.head().getAtomCount();
		for (int i = 1; i <= numInputs; ++i) {
			Expression input(inputs.child(i));
			result.push_back(ri.split(ExecutionContext(input)));
		}
		return result;
	}
	
	// getFirstAtom returns the opcode of the first atom in the supplied
	// ReductionInstance.
	uint32 getFirstAtom(ReductionInstance* ri)
	{
		return Expression(ri).head().atom;
	}

	void Instance::doActivate(Object *program)
	{
		Program p(group);
		program->copy(*p.programRI);

		// connect template arguments
		p.programRI->syncSizes();
		Expression ipgm(p.programRI, 0, false);
		Expression pgm = ipgm.child(1);
		Expression templates(pgm.child(1));
		Expression argpairs(ipgm.child(2));
		vector<bool> foundArg(templates.head().getAtomCount());
		int numPairs = argpairs.head().getAtomCount();
		bool templateParametersMatch = true;
		for (int i = 1; i <= numPairs; ++i) {
			Expression pair(argpairs.child(i));
			int n = pair.child(2).head().asFloat();
			if (n > 0 && n <= foundArg.size()) {
				foundArg[n-1] = true;
				Expression templateArgument(pair.child(1));
				Expression tpl(templates.child(n));
				if (!match(templateArgument, ExecutionContext(tpl))) {
					templateParametersMatch = false;
				}
			} else {
				printf("malformed template parameter %d for %p\n", n, program);
			}
		}
		
		bool foundAllTemplateParameters = true;
		for (int i = 0; i < foundArg.size(); ++i) {
			if (!foundArg[i])
				foundAllTemplateParameters = false;
		}

		if (!foundAllTemplateParameters || !templateParametersMatch) {
			return;
		}

		p.timeScope = pgm.child(4).head().asFloat();
		
		std::vector<ReductionInstance*> inputs = getInputs(*p.programRI);
		pair<ProgramHash::iterator, bool> ins = programs.insert(make_pair(program, p));

		for (size_t i = 0; i < inputs.size(); ++i) {
			InputHash::iterator it = inputMatchers.find(inputs[i]);
			InputMatcher* im = 0;
			if (it == inputMatchers.end()) {
				// This is a new input matcher.
				inputs[i]->retain();
				im = inputMatchers[inputs[i]] = new InputMatcher();
				im->inputPattern = inputs[i];
				uint32 a = Expression(inputs[i]).child(1).head().atom; // the first atom of skel
				inputTable.insert(make_pair(a, im));
			} else {
				// This input matcher is already known to this Instance.
				delete inputs[i];
				im = it->second;
			}
			
			// At this point, im points to the correct InputMatcher for this
			// input.  Create the bi-directional link between the Program the
			// InputMatcher.
			InputMatcher::Output o;
			o.program = &ins.first->second;
			o.inputIndex = i;
			im->outputs.push_back(o);
			ins.first->second.inputMatchers.push_back(im);
		}
	}
	
	void Instance::deactivate(Object* program)
	{
		onInput(InputQueueEntry(program, InputQueueEntry::DEACTIVATION));
	}

		
	void Instance::doDeactivate(Object* program)
	{
		ProgramHash::iterator it = programs.find(program);
		if (it == programs.end())
			return;
		Program& p = it->second;
		InputMatcher::OutputSet::iterator itO;
		for (int i = 0; i < p.inputMatchers.size(); ++i) {
			InputMatcher* im = p.inputMatchers[i];
			for (itO = im->outputs.begin(); itO != im->outputs.end(); ++itO) {
				if (itO->program == &p && itO->inputIndex == i) {
					im->outputs.erase(itO);
					break;
				}
			}
			if (im->outputs.empty()) {
				// this input matcher is now unused; erase it.
				int n = getFirstAtom(im->inputPattern);
				pair<InputTable::iterator, InputTable::iterator> bounds = inputTable.equal_range(n);
				for (InputTable::iterator it = bounds.first; it != bounds.second; ++it) {
					if (it->second == im) {
						inputTable.erase(it);
						break;
					}
				}
				inputMatchers.erase(im->inputPattern);
			}
		}
		programs.erase(program);
	}
	
	void Instance::salientObject(Object* object)
	{
		printf("salient object %p, %p\n", this, object);
		onInput(InputQueueEntry(object, InputQueueEntry::SALIENT));
	}
	
	void Instance::doSalient(Object* object)
	{
		ReductionInstance ri(group);
		object->copy(ri);
		int n = getFirstAtom(&ri);
		pair<InputTable::const_iterator, InputTable::const_iterator> bounds
		 = inputTable.equal_range(n);
		for (InputTable::const_iterator it = bounds.first; it != bounds.second; ++it) {
			InputJob j;
			j.instance = this;
			j.matcher = it->second;
			j.input = object;
			j.result = 0;
			inputJobQueue.push_back(j);
		}
	}
	
	void Instance::InputJob::process()
	{
		result = matcher->inputPattern->reduce(input);
	}
	
	void Instance::InputJob::finish()
	{
		if (result) {
			InputMatcher::OutputSet::iterator it;
			for (it = matcher->outputs.begin(); it != matcher->outputs.end(); ++it) {
				instance->onProgramInput(it->program, it->inputIndex, result);
			}
		}
		delete this;
	}

	void Instance::OverlayJob::process()
	{
		result = programRI->reduce(inputs);
	}
	
	void Instance::OverlayJob::finish()
	{
		if (result) {
			instance->group->receive(result);
		}
		for (int i = 0; i < inputs.size(); ++i) {
			inputs[i]->release();
		}
		programRI->release();
		delete this;
	}
	
	void Instance::onProgramInput(Program* program, int index, ReductionInstance* input)
	{
		int64 now = mBrane::Time::Get();
		vector<Program::MatchedInputs> newMatches;
		int iRead=0, iWrite=0;
		while (iRead < program->matchSets.size()) {
			if (program->matchSets[iRead].creationTime + program->timeScope > now) {
				for (int j = 0; j < program->matchSets[iRead].matches.size(); ++j) {
					program->matchSets[iRead].matches[j]->release();
				}
				++iRead;
			} else {
				if (program->matchSets[iRead].matches[index] == 0) {
					Program::MatchedInputs m(program->matchSets[iRead]);
					m.matches[index] = input;
					for (int k = 0; k < m.matches.size(); ++k) {
						if (m.matches[k] != 0)
							m.matches[k]->retain();
					}
					newMatches.push_back(m);
				}
				program->matchSets[iWrite++] = program->matchSets[iRead++];
			}
		}
		program->matchSets.resize(iWrite);
		program->matchSets.insert(program->matchSets.end(), newMatches.begin(), newMatches.end());
		Program::MatchedInputs m;
		m.matches.resize(program->inputMatchers.size());
		m.matches[index] = input;
		input->retain();
		program->matchSets.push_back(m);
		createOverlays(program);
	}
	
	void Instance::createOverlays(Program* program)
	{
		int iRead=0, iWrite=0;
		while (iRead < program->matchSets.size()) {
			bool isComplete = true;
			for (int k=0; k < program->matchSets[iRead].matches.size(); ++k) {
				if (program->matchSets[iRead].matches[k] == 0) {
					isComplete = false;
					break;
				}
			}
			if (isComplete) {
				OverlayJob j;
				j.instance = this;
				j.programRI = program->programRI;
				j.programRI->retain();
				j.inputs = program->matchSets[iRead].matches;
				j.result = 0;
				overlayJobQueue.push_back(j);
				++iRead;
			} else {
				program->matchSets[iWrite++] = program->matchSets[iRead++];
			}
		}
		program->matchSets.resize(iWrite);
	}
	Instance::Program::Program(Group* g) :programRI(new ReductionInstance(g)) { programRI->retain(); }
	Instance::Program::Program(const Program& p)
	{
		programRI = p.programRI;
		inputMatchers = p.inputMatchers;
		matchSets = p.matchSets;
		timeScope = p.timeScope;
		programRI->retain();
	}
	Instance::Program::~Program() { programRI->release(); }
}

Core* Core::create()
{
	return new CoreImpl::Impl();
}

}
