#include "r_code.h"
#include "container.h"
#include "r_core.h"
#include "r_listener.h"
#include "match.h"
#include <assert.h>
#include <vector>
#include <list>

using namespace std;

namespace RCoreImpl {

	struct Overlay {
		Overlay(int64 ct, vector<AtomArray*> inputs_) :creationTime(ct), inputs(inputs_) {}
		int64 creationTime;
		vector<AtomArray*> inputs;
	};

	class Impl : public r_core {
	public:
		Impl(AtomArray& program);
	protected:
		void onInput(int index, AtomArray& input);
		~Impl();
		void evaluate(Overlay& overlay);

		ExecutionContext exe;
		int numInputs;
		vector<r_listener*> listeners;
		list<Overlay> overlays;
	};

	Impl::Impl(AtomArray& program)
		:exe(program)
	{
		// template parameters NYI
		ExecutionContext inputs(exe.xchild(1).xchild(2).xchild(1));
		numInputs = inputs.head().getAtomCount();
		listeners.resize(numInputs);
		for (int i = 0; i < numInputs; ++i) {
			ExecutionContext input (inputs.xchild(i+1));
			listeners[i] = r_listener::create(this, i, input);
		}
	}

	Impl::~Impl()
	{
		for (size_t i = 0; i < listeners.size(); ++i) {
			delete listeners[i];
		}
	}

	void Impl::onInput(int index, AtomArray& input)
	{
		int64 now = exe.now();
		int64 expireTime = now - exe.child(3).decodeTimestamp();
		// erase expired overlays
		for (list<Overlay>::iterator it = overlays.begin(); it != overlays.end(); ) {
			if (it->creationTime < expireTime)
				it = overlays.erase(it);
			else
				++it;
		}

		// create a new overlay from scratch
		list<Overlay> newOverlays;
		vector<AtomArray*> inputs(numInputs, (AtomArray*)0);
		inputs[index] = &input;
		newOverlays.push_back(Overlay(now, inputs));

		// spawn overlays from existing overlays
		for (list<Overlay>::iterator it = overlays.begin(); it != overlays.end(); ++it) {
			if (it->inputs[index] == 0) {
				inputs = it->inputs;
				inputs[index] = &input;
				newOverlays.push_back(Overlay(now, inputs));
			}
		}

		// splice the new overlays in the front of the overlays list
		list<Overlay>::iterator oldBegin = overlays.begin();
		overlays.splice(overlays.begin(), newOverlays);

		// evaluate and erase evaluatable overlays
		for (list<Overlay>::iterator it = overlays.begin(); it != oldBegin; ) {
			int i;
			for (i = 0; i < numInputs; ++i) {
				if (it->inputs[i] == 0)
					break;
			}
			if (i < numInputs) { // not all inputs matched
				++it;
			} else { // all inputs matched; evaluate & erase
				evaluate(*it);
				it = overlays.erase(it);
			}
		}
	}

	void Impl::evaluate(Overlay& overlay)
	{
		// match the (pre-matched) inputs
		ExecutionContext inputs(exe.xchild(1).xchild(2).xchild(1));
		for (int i = 0; i < numInputs; ++i) {
			ExecutionContext input(*overlay.inputs[i]);
			assert(match(input, inputs.xchild(i+1)));
		}

		// check to see if the timings work
		ExecutionContext timings(exe.xchild(1).xchild(2).xchild(2));
		for (int i = 1; i <= timings.head().getAtomCount(); ++i) {
			ExecutionContext timing(timings.xchild(i));
			RExpression result = timing.evaluate();
			if (result.head().readsAsNil())
				return;
		}

		// check to see if the guards work
		ExecutionContext guards(exe.xchild(1).xchild(2).xchild(3));
		for (int i = 1; i <= guards.head().getAtomCount(); ++i) {
			ExecutionContext guard(guards.xchild(i));
			RExpression result = guard.evaluate();
			if (result.head().readsAsNil())
				return;
		}

		// evaluate the productions
		ExecutionContext prods(exe.xchild(1).xchild(3));
		for (int i = 1; i <= prods.head().getAtomCount(); ++i) {
			ExecutionContext prod(prods.xchild(i));
			ExecutionContext guards(prod.xchild(1));
			bool guardsTrue = true;
			for (int j = 1; guardsTrue && j <= guards.head().getAtomCount(); ++i) {
				ExecutionContext guard(guards.xchild(j));
				RExpression result = guard.evaluate();
				if (result.head().readsAsNil())
					guardsTrue = false;
			}
			if (guardsTrue) {
				ExecutionContext gprods = prod.xchild(2);
				for (int j = 1; j <= gprods.head().getAtomCount(); ++j) {
					ExecutionContext gprod = gprods.xchild(j);
					gprod.evaluate();
				}
			}
		}
	}
}

r_core* r_core::create(AtomArray& program)
{
	return new RCoreImpl::Impl(program);
}
