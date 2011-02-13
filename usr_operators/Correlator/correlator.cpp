#include	"correlator.h"

#include	"../r_comp/decompiler.h"
#include	"../r_exec/mem.h"


class	CorrelatorController:
public	r_exec::Controller{
private:
	Correlator			*correlator;
	r_comp::Decompiler	decompiler;
public:
	CorrelatorController(r_code::View	*icpp_pgm_view):r_exec::Controller(icpp_pgm_view){

		//	Load arguments here.
		uint16	arg_set_index=getObject()->code(ICPP_PGM_ARGS).asIndex();
		uint16	arg_count=getObject()->code(arg_set_index).getAtomCount();

		correlator=new	Correlator();

		decompiler.init(&r_exec::Metadata);
	}

	~CorrelatorController(){

		delete	correlator;
	}

	void	take_input(r_exec::View	*input,r_exec::Controller	*origin=NULL){
static	bool	once=false;if(once)return;
		//	Inputs are all types of objects - salient or that have become salient depending on their view's sync member.
		//	Manual filtering is needed instead of pattern-matching.
		//	Here we take all inputs until we get an episode notification.
		std::string	episode_end="episode_end";
		if(input->object->code(0).asOpcode()==r_exec::Metadata.getClass(episode_end)->atom.asOpcode()){

			r_exec::ReductionJob<CorrelatorController>	*j=new	r_exec::ReductionJob<CorrelatorController>(input,this);
			r_exec::_Mem::Get()->pushReductionJob(j);once=true;
		}else
			correlator->take_input(input);
	}

	void	reduce(r_exec::View	*input){

		CorrelatorOutput	*output=correlator->get_output();
		output->trace();
		
		//	TODO: exploit the output: build rgroups and inject data therein.

		//decompile(0);
			
		//	For now, we do not retrain the Correlator on more episodes: we build another correlator instead.
		//	We could also implement a method (clear()) to reset the existing correlator.
		//delete	correlator;
		//correlator=new	Correlator();
	}

	void	decompile(uint64	time_offset){

		r_exec::_Mem::Get()->suspend();
		r_comp::Image	*image=((r_exec::Mem<r_exec::LObject>	*)r_exec::_Mem::Get())->getImage();
		r_exec::_Mem::Get()->resume();

		uint32	object_count=decompiler.decompile_references(image);

		// from here on is different, by Bas
		static void* inst;
		struct OID2string {
			r_comp::Decompiler& decompiler;
			r_code::vector<r_code::SysObject*>& objects;
			uint32& object_count;
			uint64& time_offset;

			OID2string(r_comp::Decompiler& d, r_code::vector<r_code::SysObject*>& o, uint32& c, uint64& t):
				decompiler(d), objects(o), object_count(c), time_offset(t) {inst=this;}

			static std::string wrapper(uint32 id) {
				return ((OID2string*)inst)->impl(id);
			}

			std::string impl(uint32 id) {
				for(uint16 j = 0; j < object_count; ++j)
					if(objects[j]->oid == id) {
						std::ostringstream decompiled_code;
						decompiler.decompile_object(j, &decompiled_code, time_offset);
						std::string s = decompiled_code.str();
						for(size_t found = s.find("\n"); found != std::string::npos; found = s.find("\n"))
							s.replace(found, 1, " ");
						return s;
					}
				return "";
			}
		} closure(decompiler, image->code_segment.objects, object_count, time_offset);
		char buf[33];
		std::ofstream file((std::string("_DATA_") + itoa(rand(), buf, 10) + ".txt").c_str(), std::ios_base::trunc);
		if(file.is_open())
			correlator->dump(file, OID2string::wrapper);
		else
			std::cout << "ERROR" << std::endl;
		delete image;
	}
/*
	void	decompile(uint64	time_offset){

		mem->suspend();
		r_comp::Image	*image=((r_exec::Mem<r_exec::LObject>	*)mem)->getImage();
		mem->resume();

		uint32	object_count=decompiler.decompile_references(image);
		std::cout<<object_count<<" objects in the image\n";
		for(uint16	i=0;i<correlator->episode.size();++i){	// episode is ordered by injection times.

			for(uint16	j=0;j<object_count;++j){

				if(((SysObject	*)image->code_segment.objects[j])->oid==correlator->episode[i]){

					std::ostringstream	decompiled_code;
					decompiler.decompile_object(j,&decompiled_code,time_offset);
					std::cout<<"\n\nObject "<<correlator->episode[i]<<":\n"<<decompiled_code.str()<<std::endl;
				}
			}
		}

		delete	image;
	}
*/
};

////////////////////////////////////////////////////////////////////////////////

r_exec::Controller	*correlator(r_code::View	*view){

	return	new	CorrelatorController(view);
}

////////////////////////////////////////////////////////////////////////////////

#include	<ctime>

#ifdef USE_WINEPI

bool operator< (const P<r_code::Code>& x, const P<r_code::Code>& y) {
	return x->getOID() < y->getOID();
}

Correlator::Correlator() : episode(), episode_start(0), winepi() {
	// no-op
}


void Correlator::take_input(r_code::View* input) {

	if(!input || !input->object)
		return;

	episode.push_back(std::pair<timestamp_t,event_t>(input->get_ijt(), input->object));
}

CorrelatorOutput* Correlator::get_output(bool useEntireHistory) {

	if(episode_start == episode.size())
		// no new inputs since last call to get_output => nothing to correlate
		return new CorrelatorOutput;

	Episode::iterator it = episode.begin();
	if(useEntireHistory)
		episode_start = 0;
	std::advance(it, episode_start);
	winepi.setSeq(it, episode.end());

#	define AVG_IN_WINDOW 10
	int window_size = AVG_IN_WINDOW * (episode.back().first - episode[episode_start].first) / (episode.size() - episode_start);
	winepi.setParams(window_size, 0.1, 0.5, 2); // TODO: find a way to set these appropriately!

	std::vector<Rule> rules;
//	clock_t t1 = clock();
	winepi.algorithm_1(rules); // perform the actual WinEpi algorithm
//	clock_t t2 = clock();
//	COUT("Total time taken: " << DIFF_CLOCK(t1,t2) << " seconds.\n");
//	for(size_t i = 0; i < rules.size(); ++i)
//		COUT(rules[i].toString());

	CorrelatorOutput* c = new CorrelatorOutput;
	c->states.reserve(rules.size());
	for(size_t i = 0; i < rules.size(); ++i) {
		Rule& rule = rules[i];
		Pattern* p = new Pattern;
		// LHS and RHS of rules are sorted by OID
		std::map<int,event_t>::iterator lit = rule.lhs.G.begin(), rit = rule.rhs.G.begin();
		while(lit != rule.lhs.G.end()) {
			if(lit->second->getOID() == rit->second->getOID()) {
				p->left.push_back(lit->second);
				++lit;
				++rit;
			}
			else {
				p->right.push_back(rit->second);
				++rit;
			}
		}
		for(; rit != rule.rhs.G.end(); ++rit) {
			p->right.push_back(rit->second);
		}
		p->confidence = rule.conf;
		c->states.push_back(p);
	}

	episode_start = episode.size(); // remember where we left off
	return c;
}

// not yet implemented
void Correlator::dump(std::ostream& out, std::string (*oid2str)(uint32)) const {

//	::dump(episode, enc2obj, out, oid2str);
}

#else // USE_WINEPI

uint16	Correlator::NUM_BLOCKS		= 8;
uint16	Correlator::CELLS_PER_BLOCK	= 1;
uint32	Correlator::NUM_EPOCHS		= 2000;
float64	Correlator::TRAIN_TIME_SEC	= 600;
float64	Correlator::MSE_THR			= 0.001;
float64	Correlator::LEARNING_RATE	= 0.001;
float64	Correlator::MOMENTUM		= 0.01;
uint32	Correlator::SLICE_SIZE		= 5;
float64	Correlator::OBJECT_THR		= 0.5;
float64	Correlator::RULE_THR		= 0.5;

// assigns to "result" size-32 vectors representing
// all elements in episode, in order, that occur at least twice
// runtime: O(N log N) where N = distance(first, last)
static void makeNoislessInputs(Episode::const_iterator first, Episode::const_iterator last, LSTMInput& result) {

	std::set<enc_t> singles(first, last);
	std::set<enc_t> noise(singles);

	for(Episode::const_iterator it = first; it != last; ++it) {
		std::set<enc_t>::iterator sit = singles.find(*it);
		if(sit != singles.end())
			singles.erase(sit); // first occurrence
		else
			noise.erase(*it); // second occurrence means not noise
	}

	result.assign(std::distance(first, last) - noise.size(), LSTMState(32));

	uint32 i = 0;
	for(Episode::const_iterator it = first; it != last; ++it) {
		enc_t code = *it;
		if(noise.find(code) == noise.end()) {
			for(uint32 j = 0, b = 1 << 31; j < 32; ++j, b >>= 1)
				result[i][j] = code & b ? 1 : 0;
			++i;
		}
	}
}

// converts Jacobian rules A <- [B1,..,Bn] to small worlds {Bn,..,B1,A}
static void rules2worlds(const JacobianRules& rules, SmallWorlds& worlds) {

	worlds.reserve(rules.size());

	JacobianRules::const_iterator rit;
	for(rit = rules.begin(); rit != rules.end(); ++rit) {
		const JacobianRule& rule = *rit;
		SmallWorld objects;
		Sources::const_iterator sit;
		for(sit = rule.sources.begin(); sit != rule.sources.end(); ++sit)
			objects.push_front(sit->source);
		objects.push_back(rule.target);
		worlds.push_back(objects);
	}

/*	// test for motor babbling example
	enc_t white = episode[0];
	enc_t small = episode[1];
	enc_t cyl   = episode[2];
	enc_t att   = episode[3];
	enc_t handp1 = episode[4];
	enc_t objp1 = episode[5];
	enc_t handp2 = episode[6];
	enc_t objp2 = episode[7];
	enc_t handp3 = episode[8];
	enc_t objp3 = episode[9];

	P<r_code::Code> w1[] = {enc2obj[white], enc2obj[small], enc2obj[cyl]};
	P<r_code::Code> w2[] = {enc2obj[white], enc2obj[small], enc2obj[cyl], enc2obj[att]};
	P<r_code::Code> w3[] = {enc2obj[white], enc2obj[small], enc2obj[cyl], enc2obj[att], enc2obj[handp1], enc2obj[objp1]};
	P<r_code::Code> w4[] = {enc2obj[white], enc2obj[small], enc2obj[cyl], enc2obj[att], enc2obj[handp2], enc2obj[objp2]};
	P<r_code::Code> w5[] = {enc2obj[white], enc2obj[small], enc2obj[cyl], enc2obj[att], enc2obj[handp3], enc2obj[objp3]};

	worlds.push_back(SmallWorld(w1, w1+3));
	worlds.push_back(SmallWorld(w2, w2+4));
	worlds.push_back(SmallWorld(w3, w3+6));
	worlds.push_back(SmallWorld(w4, w4+6));
	worlds.push_back(SmallWorld(w5, w5+6));
*/
}

// for each set in range [first,last), subtracts all elements from w
// stops as soon as no elements of w could be subtracted
// returns iterator pointing to first untouched SmallWorld
static SmallWorlds::iterator mapDiff(const SmallWorld& w,
	SmallWorlds::iterator first, SmallWorlds::iterator last) {
	
	for(; first != last; ++first) {
		bool stop = true;
		for(SmallWorld::const_iterator it = w.begin(); it != w.end(); ++it) {
			SmallWorld::iterator found = std::find(first->begin(), first->end(), *it);
			if(found != first->end()) {
				first->erase(found);
				stop = false;
			}
		}
		if(stop)
			break;
	}

	return first;
}

// converts a small worlds representation to a correlator tree
static Correlations::iterator swm2corr(
	SmallWorlds::iterator first, SmallWorlds::iterator last,
	Correlations::iterator result) {

	if(first == last) // done
		return result;
	if(first->empty()) // boriiing
		return swm2corr(++first, last, result);

	// try to see if objects in the first world appear in subsequent worlds
	SmallWorlds::iterator next = first;
	SmallWorlds::iterator contextEnd = mapDiff(*first, ++next, last);

	// no context found; make a pattern
	if(contextEnd == next) {
		Pattern* pat = new Pattern();
		pat->left.assign(first->begin(), --first->end()); // this is pretty arbitrary
		pat->right.assign(--first->end(), first->end());
		*result++ = pat;
	}
	// context found; recurse on it
	else {
		Context* con = new Context();
		con->objects.assign(first->begin(), first->end());
		con->states.resize(std::distance(next, contextEnd));
		Correlations::iterator subEnd = swm2corr(next, contextEnd, con->states.begin());
		con->states.resize(std::distance(con->states.begin(), subEnd));
		*result++ = con;
	}

	return swm2corr(contextEnd, last, result); // continue after end of context
}

// calculate the hamming distance between the bit representations of a and b
template<class In, class Out>
static Out hamdist(In a, In b) {
	
	Out dist = 0;
	for(In x = a ^ b; x; x &= x - 1)
		++dist;
	return dist;
}

// useful for debugging
static void dump(const Episode& episode, const Table_Enc2Obj& enc2obj, std::ostream& out = std::cout, std::string (*oid2str)(OID_t) = NULL) {
	
	for(Episode::const_iterator it = episode.begin(); it != episode.end(); ++it) {
		enc_t code = *it;
		OID_t id = enc2obj.find(code)->second->getOID();
		out << id << "\t";
		for(enc_t b = 1 << 31; b; b >>= 1)
			out << (code & b ? 1 : 0);
		if(oid2str)
			out << "\t" << oid2str(id);
		out << std::endl;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

Correlator::Correlator() : episode_start(0) {
}

// stores the object pointed to by the provided View
// also finds a binary encoding to associate with the object
// runtime: O(log N) where N = size of episode so far
void Correlator::take_input(r_code::View* input){

	if(!input || !input->object)
		return;

//	OID_t id = input->code(VIEW_OID).atom;
	OID_t id = input->object->getOID();
	bool is_new;
	enc_t code = encode(id, is_new);
	episode.push_back(code);
	if(is_new) {
		enc2obj[code] = input->object;
		oid2enc[id] = code;
	}
}

// trains the LSTM network on the episode so far and
// returns the found correlations
// useEntireHistory determines whether to use entire episode
// or only from last call to get_output
CorrelatorOutput* Correlator::get_output(bool useEntireHistory){

	static bool first_call = true;

	//dump(); // DEBUG

	// determine where to start
	Episode::iterator first = episode.begin();
	if(!useEntireHistory)
		std::advance(first, episode_start);

	// remember where we left off
	episode_start = episode.size();

	// remove noise and generate LSTM inputs
	LSTMInput inputs;
	makeNoislessInputs(first, episode.end(), inputs);

	// train the LSTM
	std::cout << "*** START TRAINING ***" << std::endl; // DEBUG
	if(first_call) {
		corcor.initializeOneStepPrediction(CELLS_PER_BLOCK, NUM_BLOCKS, inputs);
		corcor.lstmNetwork->setRandomWeights(.5);
		first_call = false;
	} else
		corcor.appendBuffers(inputs);
	time_t start, current;
	uint16 epoch;
	float64 mse;
	for(time(&start), time(&current), epoch = 0, mse = 0xFFFFFFFF;
		epoch < NUM_EPOCHS && mse > MSE_THR && difftime(current, start) < TRAIN_TIME_SEC;
		++epoch, time(&current)) {
		mse = corcor.trainingEpoch(LEARNING_RATE, MOMENTUM);
		std::cout << epoch << "\t" << mse << std::endl; // DEBUG
	}
	std::cout << "#epochs done: " << epoch << std::endl; // DEBUG
	std::cout << "final MSE: " << mse << std::endl; // DEBUG
	std::cout << "seconds taken: " << difftime(current, start) << std::endl; // DEBUG

	// extract rules, and convert to small worlds representation
	JacobianRules rules;
	extract_rules(rules, inputs.size());
	SmallWorlds worlds;
	rules2worlds(rules, worlds);

	// the small worlds representation is easily converted to the desired correlator output
	CorrelatorOutput* c = new CorrelatorOutput();
	c->states.resize(worlds.size());
	Correlations::iterator corrEnd = swm2corr(worlds.begin(), worlds.end(), c->states.begin());
	c->states.resize(std::distance(c->states.begin(), corrEnd));
	return c;
}

// dump a listing of object IDs and their encodings to some output
// the oid2str function can be used to provide a way of printing objects
void Correlator::dump(std::ostream& out, std::string (*oid2str)(OID_t)) const {

	::dump(episode, enc2obj, out, oid2str);
}

// finds a sparse binary encoding of the provided identifier
// sets is_new to true iff an encoding already exists
// currently, the encoding is generated randomly,
// but in the future we may implement a better algorithm
enc_t Correlator::encode(OID_t id, bool& is_new) {
	
	const static uint8 INIT_NUM_ONES = 4; // can be changed
	static uint8 NUM_ONES = INIT_NUM_ONES; // #ones we want in the encoding
	static uint8 NUM_POS = NUM_ONES; // #positions to flip
	static enc_t INIT_CODE = 0; // either 0 of 0xFFFFFFFF
//	const static uint32 BINOMIALS[] = {1, 32, 496, 4960, 35960, 201376,
//		906192, 3365856, 10518300, 28048800, 64512240, 129024480,
//		225792840, 347373600, 471435600, 565722720, 601080390};
	// cumulative binomials of 32 above <index>
	// Mathematica: Accumulate[Binomial[32, Range[0, 31]]]
	const static uint32 CUML_BINS[] = {1, 33, 529, 5489, 41449, 242825,
		1149017, 4514873, 15033173, 43081973, 107594213, 236618693,
		462411533, 809785133, 1281220733, 1846943453, 2448023843,
		3013746563, 3485182163, 3832555763, 4058348603, 4187373083,
		4251885323, 4279934123, 4290452423, 4293818279, 4294724471,
		4294925847, 4294961807, 4294966767, 4294967263, 4294967295};
	// used to determine when to use more ones in encodings
	static uint32 MAX_SIZE = CUML_BINS[NUM_ONES] - 
		(INIT_NUM_ONES > 0 ? CUML_BINS[INIT_NUM_ONES - 1] : 0);

	// first, check if we already have an encoding
	Table_OID2Enc::iterator it = oid2enc.find(id);
	if(it != oid2enc.end()) {
		is_new = false;
		return it->second;
	}

	// check if encodings with current number of ones have been depleted
	// if so, use more ones
	// to use LESS possibilities: >= CUML_BINS[NUM_ONES] / 2
	if(oid2enc.size() >= MAX_SIZE) {
		++NUM_ONES;
		MAX_SIZE = CUML_BINS[NUM_ONES] -
			(INIT_NUM_ONES > 0 ? CUML_BINS[INIT_NUM_ONES - 1] : 0);
		NUM_POS = NUM_ONES <= 16 ? NUM_ONES : 32 - NUM_ONES;
		INIT_CODE = NUM_ONES <= 16 ? 0 : 0xFFFFFFFF;
	}

	// now find a random but unique binary encoding
	enc_t code;
	do {
		code = INIT_CODE;

		// collect enough random positions
		std::set<int> positions;
		while(positions.size() < NUM_POS)
			positions.insert(rand() % 32); // & 31

		// flip bits at the chosen positions
		std::set<int>::iterator it;
		for(it = positions.begin(); it != positions.end(); ++it)
			code ^= (enc_t) 1 << *it;

	} while(enc2obj.find(code) != enc2obj.end()); // check uniqueness

	is_new = true;
	return code;
}

// extracts rules of the form Target <= {(Src_1,Dt_1),..,(Src_n,Dt_n)}
// for deltaTimes Dt_1 < .. < Dt_n
void Correlator::extract_rules(JacobianRules& rules, uint32 episode_size) {

	uint32 num_calls = episode_size - SLICE_SIZE;
	rules.reserve(num_calls);
	JacobianSlice slice(SLICE_SIZE, LSTMState(32));


	for(int32 t = num_calls - 1; t >= 0; --t) {

		// perform sensitivity analysis
		corcor.getJacobian(t, t + SLICE_SIZE, slice);

		// the last one is the target
		JacobianRule rule;
		rule.target = findBestMatch(slice.back().begin(), rule.confidence);
		if(rule.confidence < OBJECT_THR)
			continue;

		// the later objects occurred closer to the target, so iterate in reverse
		JacobianSlice::reverse_iterator it = ++slice.rbegin();
		JacobianSlice::reverse_iterator end = slice.rend();
		for(uint16 deltaTime = 1; it != end; ++it, ++deltaTime) {

			float64 match;
			r_code::Code* source = findBestMatch(it->begin(), match);

			// no confidence, no correlation
			if(match >= OBJECT_THR) {
				Source src;
				src.source = source;
				src.deltaTime = deltaTime;
				rule.sources.push_back(src);
				rule.confidence += match;
			}
		}

		if(!rule.sources.empty()) {
			rule.confidence /= rule.sources.size() + 1;
			if(rule.confidence >= RULE_THR)
				rules.push_back(rule);
		}
	}
}

// returns the object whose binary encoding best matches
//   the contents of the container starting at "first"
// in case of ties, returns an unspecified best match
// returns NULL iff enc2obj is empty
// NOTE: assumes there are (at least) 32 elements accessible from "first"
// TODO: might return an object not occurring in the partial episode if not using entire history
template<class InputIterator>
r_code::Code* Correlator::findBestMatch(InputIterator first, float64& bestMatch) const {

	r_code::Code* object = NULL;
	bestMatch = -1;

	Table_Enc2Obj::const_iterator it;
	for(it = enc2obj.begin(); it != enc2obj.end(); ++it) {

		enc_t code = it->first;
		if(code == 0) { // can only happen if encode::INIT_NUM_ONES = 0
			if(!object)
				object = (r_code::Code*) it->second;
			continue;
		}

		float64 match = 0;
		InputIterator vit = first;
		for(enc_t b = 1 << 31; b; b >>= 1, ++vit)
			if(code & b)
				match += fabs(*vit);
		match /= hamdist<enc_t,uint8>(code, 0);
		
		if(match > bestMatch) {
			bestMatch = match;
			object = (r_code::Code*) it->second;
		}
	}

	/* DEBUG //
	enc_t code = oid2enc.find(object->getOID())->second;
	std::cout << "Best match found: ";
	for(enc_t b = 1 << 31; b; b >>= 1)
		std::cout << (code & b ? 1 : 0);
	std::cout << std::endl;
	std::cout << "Matching bits:";
	InputIterator vit = first;
	for(enc_t b = 1 << 31; b; b >>= 1, ++vit)
		if(code & b)
			std::cout << " " << *vit;
	std::cout << std::endl;
	//DEBUG END */

	return object;
}

#endif // USE_WINEPI
