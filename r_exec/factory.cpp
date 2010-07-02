//	factory.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel nor the
//     names of their contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
//	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include	"factory.h"


namespace	r_exec{
	namespace	factory{

		MkNew::MkNew(r_code::Mem	*m,Code	*object):LObject(m){

			uint16	write_index=0;
			code(write_index++)=r_code::Atom::Object(Opcodes::MkNew,5);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
			set_reference(0,object);
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkLowRes::MkLowRes(r_code::Mem	*m,Code	*object):LObject(m){

			uint16	write_index=0;
			code(write_index++)=r_code::Atom::Object(Opcodes::MkLowRes,5);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
			set_reference(0,object);
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkLowSln::MkLowSln(r_code::Mem	*m,Code	*object):LObject(m){

			uint16	write_index=0;
			code(write_index++)=r_code::Atom::Object(Opcodes::MkLowSln,5);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
			set_reference(0,object);
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkHighSln::MkHighSln(r_code::Mem	*m,Code	*object):LObject(m){

			uint16	write_index=0;
			code(write_index++)=r_code::Atom::Object(Opcodes::MkHighSln,5);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
			set_reference(0,object);
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkLowAct::MkLowAct(r_code::Mem	*m,Code	*object):LObject(m){

			uint16	write_index=0;
			code(write_index++)=r_code::Atom::Object(Opcodes::MkLowAct,5);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
			set_reference(0,object);
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkHighAct::MkHighAct(r_code::Mem	*m,Code	*object):LObject(m){

			uint16	write_index=0;
			code(write_index++)=r_code::Atom::Object(Opcodes::MkHighAct,5);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
			set_reference(0,object);
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkSlnChg::MkSlnChg(r_code::Mem	*m,Code	*object,float32	value):LObject(m){

			uint16	write_index=0;
			code(write_index++)=r_code::Atom::Object(Opcodes::MkSlnChg,6);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::Float(value);	//	change.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
			set_reference(0,object);
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkActChg::MkActChg(r_code::Mem	*m,Code	*object,float32	value):LObject(m){

			uint16	write_index=0;
			code(write_index++)=r_code::Atom::Object(Opcodes::MkActChg,6);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::Float(value);	//	change.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
			set_reference(0,object);
		}
	}
}