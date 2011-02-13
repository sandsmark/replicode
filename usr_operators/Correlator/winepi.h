#ifndef	winepi_h
#define	winepi_h

// do not uncomment this switch; implementation incomplete!
//#define WINEPI_SERIAL

#include <map>
#include <set>
#include <list>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <utility> // pair
#include <fstream>
#include <iostream> // cout
#include <ctime> // clock_t

#include	<../../CoreLibrary/trunk/CoreLibrary/base.h>
#include	<../r_code/object.h>

//#include "correlator.h"
typedef uint64 timestamp_t;
typedef P<r_code::Code> event_t;


struct Candidate {
	std::map<int,event_t> G; // mapping from [1..] to event_types
#ifdef WINEPI_SERIAL
	std::multimap<int,int> R; // list of tuples of type (int,int)
#endif
	std::map<event_t,size_t> type_count;
	int block_start;
	int event_count;
	int freq_count;
	timestamp_t inwindow;

	Candidate() {
		init();
	}

	Candidate(event_t& A) {
		init();
		set(1, A);
	}

	void init() {
		block_start = 0;
		event_count = 0;
		freq_count = -1;
		inwindow = 0;
	}

	Candidate& operator=(const Candidate& e) {
		G.clear();
		G.insert(e.G.begin(), e.G.end());
#ifdef WINEPI_SERIAL
		R.clear();
		R.insert(e.R.begin(), e.R.end());
#endif
		type_count.clear();
		type_count.insert(e.type_count.begin(), e.type_count.end());
		block_start = e.block_start;
		event_count = e.event_count;
		freq_count = e.freq_count;
		inwindow = e.inwindow;
		return *this;
	}

	bool operator==(const Candidate& e) const {
		return G == e.G;
	}

	bool operator<(const Candidate& e) const {
		std::map<int,event_t>::const_iterator it = G.begin(), it2 = e.G.begin();
		for(; it != G.end() && it2 != e.G.end(); ++it, ++it2)
			if(it->second < it2->second)
				return true;
			else if(it->second > it2->second)
				return false;
		return (it2 != e.G.end());
	}

	size_t size() const {
		return G.size();
	}

	event_t& get(int i) {
		return G.find(i)->second;
	}

	void set(int i, event_t& x) {
//			if(G.find(i) != G.end())
//				--type_count[G[i]];
		++type_count[x];
		G[i] = x;
	}
/*
	bool order(int x, int y) {
		std::pair<std::multimap<int,int>::iterator,std::multimap<int,int>::iterator> its = R.equal_range(x);
		for(std::multimap<int,int>::iterator it = its.first; it != its.second; ++it)
			if(it->second == y)
				return true;
		return false;
	}
*/
	std::string toString() const {
		std::stringstream ss;
		ss << "<";
		bool comma = false;
		for(std::map<int,event_t>::const_iterator it = G.begin(); it != G.end(); ++it) {
			if(comma)
				ss << ", ";
			else
				comma = true;
			ss << it->second->getOID() /*<< it->first*/;
		}
#ifdef WINEPI_SERIAL
		for(std::multimap<int,int>::const_iterator it = R.begin(); it != R.end(); ++it)
			ss << ", " << G.find(it->first)->second /*<< it->first*/ << "->" << G.find(it->second)->second /*<< it->second*/;
#endif
		ss << ">";
		return ss.str();
	}
};

struct Rule {
	Candidate lhs;
	Candidate rhs;
	double conf;

	Rule(const Candidate& lhs_, const Candidate& rhs_, double conf_) : lhs(lhs_), rhs(rhs_), conf(conf_) {}

	Rule& operator=(const Rule& r) {
		lhs = r.lhs;
		rhs = r.rhs;
		conf = r.conf;
		return *this;
	}

	std::string toString() {
		std::stringstream ss;
		ss << lhs.toString() << " implies " << rhs.toString() << " with conf " << conf;
		return ss.str();
	}
};

// a sequence is a tuple (s, starttime, endtime) where s is a list of tuples (timestamp,event)
// a candidate is a tuple (G,R,block_start)
//   where G is a dict: int -> event_types
//         R is a list of (int,int) tuples
//         block_start is an index

struct Sequence {
	std::multimap<timestamp_t,event_t> seq;
	timestamp_t start;
	timestamp_t end;

	Sequence() {}

	Sequence(std::multimap<timestamp_t,event_t>& seq_)
		: seq(seq_.begin(), seq_.end())
		, start(seq.begin()->first)
		, end(seq.rbegin()->first + 1)
	{}

	template<class InputIterator>
	void init(InputIterator it, InputIterator last) {
		seq.clear();
		seq.insert(it, last);
		start = seq.begin()->first;
		end = seq.rbegin()->first + 1;
	}

	void addEvent(timestamp_t t, event_t ev) {
		seq.insert(std::pair<timestamp_t, event_t>(t, ev));
	}

	std::string toString() {
		std::stringstream ss;
		ss << "<{";
		bool comma = false;
		for(std::multimap<timestamp_t,event_t>::iterator it = seq.begin(); it != seq.end(); ++it) {
			if(comma)
				ss << ", ";
			else
				comma = true;
			ss << it->first << ":" << it->second->getOID();
		}
		ss << "}, s:" << start << ", e:" << end << ">";
		return ss.str();
	}
};




class WinEpi {
public:

	Sequence seq;
	int win;
	double min_fr;
	double min_conf;
	int max_size;

	std::set<event_t> event_types;

	WinEpi(/*std::multimap<timestamp_t,event_t>& seq, int win, double min_fr, double min_conf, int max_size = -1*/);

	// initialize the sequence; iterator must return pairs
//	template<class InputIterator> void setSeq(InputIterator it, InputIterator end);
	template<class InputIterator>
	void setSeq(InputIterator it, InputIterator end) {
		seq.init(it, end);
		for(; it != end; ++it) {
			event_types.insert(it->second);
		}
	}

	void setParams(int win, double min_fr, double min_conf, int max_size = -1);

	void algorithm_1(std::vector<Rule>& out);
	void algorithm_2(std::vector<std::vector<Candidate> >& F);
	void algorithm_3(std::vector<Candidate>& F, int el, std::vector<Candidate>& C);
	void algorithm_4(std::vector<Candidate>& C, double min_fr, std::vector<Candidate>& F);
	double fr(Candidate& a);

	template<class K, class V>
	static void subsets(std::map<K,V>& in, std::vector<std::map<K,V> >& out) {
		size_t nIn = in.size();
		size_t nOut = 1 << nIn;
		out.resize(nOut);
		std::map<K,V>::iterator it = in.begin();
		for(size_t i = 0; i < nIn; ++i, ++it) {
			size_t chunk_size = 1 << (nIn - i - 1);
			for(size_t chunk_start = 0; chunk_start < nOut; chunk_start += 2 * chunk_size)
				for(size_t j = chunk_start; j < chunk_start + chunk_size; ++j)
					out[j].insert(*it);
		}
	}

#ifdef WINEPI_SERIAL
	static void restrict(std::multimap<int,int>& in, std::map<int,event_t>& seq, std::multimap<int,int>& out) {
		for(std::multimap<int,int>::iterator it = in.begin(); it != in.end(); ++it)
			if(seq.find(it->first) != seq.end() && seq.find(it->second) != seq.end())
				out.insert(*it);
	}
#endif

	static void strict_subcandidates(Candidate& a, std::vector<Candidate>& out) {

		if(a.size() < 2)
			return;

		out.resize((1 << a.size()) - 2);

		std::vector<std::map<int,event_t> > subseqv(1 << a.size());
		subsets(a.G, subseqv);
		std::vector<std::map<int,event_t> >::iterator it = subseqv.begin();
		std::vector<std::map<int,event_t> >::iterator end = subseqv.end();
		size_t i = 0;
		for(++it, --end; it != end; ++it, ++i) {
#ifdef WINEPI_SERIAL
			std::multimap<int,int> R;
			restrict(a.R, *it, R);
			out[i].R.insert(R.begin(), R.end());
#endif
			for(std::map<int,event_t>::iterator it2 = it->begin(); it2 != it->end(); ++it2)
				out[i].set(it2->first, it2->second);
		}
	}
};

#endif
