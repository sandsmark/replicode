#ifndef	pgm_overlay_h
#define	pgm_overlay_h

#include	"base.h"
#include	"../r_code/object.h"

using	namespace	r_code;

namespace	r_exec{

	class	Mem;
	class	View;

	class	Object;
	class	InstantiatedProgram;
	class	IPGMController;
	class	Context;

	class	dll_export	Overlay:
	public	_Object{
	friend	class	IPGMController;
	friend	class	Context;
	protected:
		//	Copy of the pgm code. Will be patched during matching and evaluation:
		//	any area indexed by a vl_ptr will be overwritten with:
		//		the evaluation result if it fits in a single atom,
		//		a ptr to the value array if the result is larger than a single atom,
		//		a ptr to an input if the result is a pattern input.
		Atom	*pgm_code;
		uint16	pgm_code_size;

		//	Convenience.
		uint16	first_timing_constraint_index;
		uint16	last_timing_constraint_index;
		uint16	first_guard_index;
		uint16	last_guard_index;
		uint16	first_production_index;
		uint16	last_production_index;

		std::vector<Atom>	values;				//	value array.
		uint16				value_commit_index;	//	index of the last computed value+1; used for rollbacks.
		std::vector<uint16>	patch_indices;		//	indices where patches are applied; used for rollbacks.

		std::list<uint16>				input_pattern_indices;	//	stores the input patterns still waiting for a match: will be plucked upon each successful match.
		std::vector<P<r_code::View> >	input_views;			//	copies of the inputs; vector updated at each successful match.

		std::vector<P<Object> >			productions;	//	receives the results of ins, inj and eje; views are retrieved (fvw) or built (reduction) in the value array.

		typedef	enum{
			SUCCESS=0,
			FAILURE=1,
			IMPOSSIBLE=3	//	when the input's class does not even match the object class in the pattern's skeleton.
		}MatchResult;

		MatchResult	match(r_exec::View	*input,uint16	&input_index);	//	delegates to _match; input_index is set to the index of the pattern that matched the input.
		bool		check_timings();									//	return true upon successful evaluation.
		bool		check_guards();										//	return true upon successful evaluation.
		bool		inject_productions(Mem	*mem);						//	return true upon successful evaluation.

		MatchResult	_match(r_exec::View	*input,uint16	pattern_index);	//	delegates to -match_pattern
		MatchResult	_match_pattern(r_exec::View	*input,uint16	pattern_index);	//	return SUCCESS upon a successful match, IMPOSSIBLE if the input is not of the right class, FAILURE otherwise.
		bool		_match_skeleton(r_exec::View	*input,uint16	pattern_index);
		bool		evaluate(uint16	index);	//	evaluates the pgm_code at the specified index.

		void		rollback();	//	reset the overlay to the last commited state: unpatch code and values.
		void		commit();	//	empty the patch_indices and set value_commit_index to values.size().
		void		reset();	//	reset to original state (pristine copy of the pgm code and empty value set).

		void		patch_tpl_args();	//	no views in tpl args; patches the ptn skeleton's first atom with IPGM_PTR with an index in the ipgm arg set; patches wildcards with similar IPGM_PTRs.
		void		patch_tpl_code(uint16	pgm_code_index,uint16	ipgm_code_index);	//	to recurse.
		void		patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index);
		void		patch_code(uint16	index,Atom	value);

		IPGMController	*controller;

		Overlay(IPGMController	*c);
		Overlay(Overlay	*original,uint16	last_input_index,uint16	value_commit_index);	//	copy from the original and rollback.
	public:
		virtual	~Overlay();

		virtual	void	kill();
		virtual	bool	is_alive()	const;

		virtual	void	reduce(r_exec::View	*input,Mem	*mem);	//	called upon the processing of a reduction job.

		InstantiatedProgram	*getIPGM()		const;
		r_exec::View		*getIPGMView()	const;

		r_exec::Object		*getInputObject(uint16	i)	const;
		r_exec::View		*getInputView(uint16	i)	const;
	};

	class	AntiOverlay:
	public	Overlay{
	friend	class	IPGMController;
	private:
		bool	alive;

		AntiOverlay(IPGMController	*c);
		AntiOverlay(AntiOverlay	*original,uint16	last_input_index,uint16	value_limit);
	public:
		~AntiOverlay();

		void	kill();
		bool	is_alive()	const;

		void	reduce(r_exec::View	*input,Mem	*mem);	//	called upon the processing of a reduction job.
	};

	class	IPGMController:
	public	_Object{
	private:
		r_code::View			*ipgm_view;
		bool					alive;
		std::list<P<Overlay> >	overlays;
		bool					successful_match;
	public:
		IPGMController(r_code::View	*ipgm_view):_Object(),ipgm_view(ipgm_view),alive(true),successful_match(false){}
		~IPGMController(){}

		InstantiatedProgram	*getIPGM()		const;
		r_exec::View		*getIPGMView()	const;

		void	kill();
		bool	is_alive()	const;

		void	take_input(r_exec::View	*input,Mem	*mem);	//	push one job for each overlay; called by the rMem at update time and at injection time.
		void	signal_anti_pgm(Mem	*mem);
		void	signal_input_less_pgm(Mem	*mem);

		void	remove(Overlay	*overlay);
		void	add(Overlay	*overlay);
		void	restart(AntiOverlay	*overlay,Mem	*mem,bool	match);

		class	Hash{
		public:
			size_t	operator	()(IPGMController	*o)	const{
				return	(size_t)o;
			}
		};

		class	Equal{
		public:
			bool	operator	()(const	IPGMController	*lhs,const	IPGMController	*rhs)	const{
				return	lhs==rhs;
			}
		};
	};
}


#include	"pgm_overlay.inline.cpp"


#endif