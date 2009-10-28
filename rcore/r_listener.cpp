#include "r_listener.h"
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include "operator_equ.h"
#include "match.h"

using namespace std;
using namespace tr1;

namespace RListenerImpl {
	struct Pattern;

	struct Impl : public r_listener {
		Impl(r_core *core_, int index_, Pattern *pattern_);
		~Impl();

		r_core* core;
		int index;
		Pattern *pattern;
		void onInput(AtomArray& input) { core->onInput(index, input); }
	};

	struct Pattern {
		Pattern(ExecutionContext& pattern_) :pattern(pattern_) {}

		struct ImplHash { size_t operator() (const Impl* x) const { return reinterpret_cast<size_t>(x); } };
		typedef unordered_set<Impl*, ImplHash> ImplStore;
		
		ImplStore impls;
		ExecutionContext& pattern;
	};

	typedef unordered_map<RExpression, Pattern*, REhash> PatternStore;


	PatternStore patterns;
	
	Impl::Impl(r_core *core_, int index_, Pattern *pattern_)
			:core(core_), index(index_), pattern(pattern_)
	{
		pattern->impls.insert(this);
	}
	Impl::~Impl()
	{
		pattern->impls.erase(this);
	}
}

using namespace RListenerImpl;

r_listener* r_listener::create(r_core *core, int index, ExecutionContext& pattern)
{
	PatternStore::iterator it = patterns.find(pattern);
	if (it == patterns.end())
		patterns[pattern] = new Pattern(pattern);
	return new Impl(core, index, it->second);
}

void r_listener::onMessage(AtomArray& message)
{
	ExecutionContext input(message);
	for (PatternStore::iterator it = patterns.begin(); it != patterns.end(); ++it) {
		Pattern* pattern = it->second;
		if (match(input, pattern->pattern)) {
			for (Pattern::ImplStore::iterator iti = pattern->impls.begin(); iti != pattern->impls.end(); ++iti)
				(*iti)->onInput(message);
		}
	}
}
