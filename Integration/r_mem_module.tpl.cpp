//	r_mem_module.tpl.cpp
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

template<class	U>	NetworkMem<U>::NetworkMem():r_exec::Mem<RObject,RObject::Hash,RObject::Equal>(){
}

template<class	U>	NetworkMem<U>::~NetworkMem(){
}

template<class	U>	inline	void	NetworkMem<U>::eject(r_exec::View	*view,uint16	nodeID){

	STDGroupID	destination;
	if(view->get_host()==_stdin)
		destination=STDIN;
	else
		destination=STDOUT;

	CodePayload	*c;
	if(!view->object->is_compact()){

		c=new(view->object->code_size()+view->object->references_size())	CodePayload(view->object->code_size());
		c->load(view->object);
	}else
		c=((RCode	*)view->object)->get_payload();

	NODE->send(this,c,NODE->id(),N::PRIMARY);	//	we use for now one single node. In the future, nodeID referes to a node name (ex: 1 -> "node1").
}

template<class	U>	inline	void	NetworkMem<U>::eject(r_exec::LObject	*command,uint16	nodeID){

	Command	*c=new(command->code_size()+command->references_size())	Command(command->code_size());
	c->sid()=command->code(CMD_DEVICE).getDeviceID();	//	must be 1 (0 is the executive).
	c->load(command);

	NODE->send(this,c,NODE->id(),N::PRIMARY);
}