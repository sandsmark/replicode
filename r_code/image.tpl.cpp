// image.tpl.cpp
//
// Author: Eric Nivel
//
// BSD license:
// Copyright (c) 2010, Eric Nivel
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
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
// THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <iostream>
#include "atom.h"
#include "CoreLibrary/utils.h"
#include "CoreLibrary/debug.h"


namespace r_code{

template<class I> Image<I> *Image<I>::Build(uint64_t timestamp, size_t map_size, size_t code_size, size_t names_size)
{
        I *image=new(map_size+code_size) I(timestamp,map_size,code_size,names_size);
        return (Image<I> *)image;
}

template<class I> Image<I> *Image<I>::Read(ifstream &stream)
{
    uint64_t timestamp;
    size_t map_size;
    size_t code_size;
    size_t names_size;
    stream.read((char *)&timestamp,sizeof(uint64_t));
    stream.read((char *)&map_size,sizeof(size_t));
    stream.read((char *)&code_size,sizeof(size_t));
    stream.read((char *)&names_size,sizeof(size_t));
    Image *image = Build(timestamp, map_size, code_size, names_size);
    stream.read((char *)image->data(), image->get_size() * sizeof(uintptr_t));
    return image;
}

template<class I> void Image<I>::Write(Image<I> *image, ofstream &stream)
{
    uint64_t timestamp=image->timestamp();
    size_t map_size=image->map_size();
    size_t code_size=image->code_size();
    size_t names_size=image->names_size();
    stream.write((char *)&timestamp, sizeof(uint64_t));
    stream.write((char *)&map_size, sizeof(size_t));
    stream.write((char *)&code_size, sizeof(size_t));
    stream.write((char *)&names_size, sizeof(size_t));
    stream.write((char *)image->data(), image->get_size()*sizeof(uintptr_t));
}

template<class I> Image<I>::Image():I()
{
}

template<class I> Image<I>::~Image()
{
}

template<class I> size_t Image<I>::get_size() const
{
    return this->map_size() + this->code_size() + this->names_size();
}

template<class I> size_t Image<I>::getObjectCount() const
{
    return this->map_size();
}

template<class I> uintptr_t *Image<I>::getObject(size_t i)
{
    return this->data() + this->data(i);
}

template<class I> uintptr_t *Image<I>::getCodeSegment()
{
    return this->data() + this->map_size();
}

template<class I> size_t Image<I>::getCodeSegmentSize() const
{
    return this->code_size();
}

template<class I> void Image<I>::trace() const
{
    debug("image trace:") <<"Size: "<< get_size();
    debug("image trace:") <<"Object Map Size: "<< this->map_size();
    debug("image trace:") <<"Code Segment Size: "<< this->code_size();
    debug("image trace:") <<"Names Size: "<< this->names_size();

    size_t i=0;

    for(;i<this->map_size();++i)
        debug("image trace object") << i <<" " << this->data(i);

    // at this point, i is at the first word32 of the first object in the code segment
    size_t code_start=this->map_size();
    for(size_t j = 0;j < code_start; ++j){ // read object map: data[data[j]] is the first word32 of an object, data[data[j]+5] is the first atom
        uintptr_t object_axiom = this->data(this->data(j));
        size_t object_code_size = this->data(this->data(j) + 1);
        size_t object_reference_set_size = this->data(this->data(j) + 2);
        size_t object_marker_set_size = this->data(this->data(j) + 3);
        size_t object_view_set_size = this->data(this->data(j) + 4);
        debug("image trace object") << "object" << i++;
        /*switch(object_axiom){
case SysObject::ROOT_GRP: std::cout<<" root\n"; break;
case SysObject::STDIN_GRP: std::cout<<" stdin\n"; break;
case SysObject::STDOUT_GRP: std::cout<<" stdout\n"; break;
case SysObject::SELF_ENT: std::cout<<" self\n"; break;
default: std::cout<<" non standard\n"; break;
}*/
        debug("image trace object") << i++ <<" code size: "<<object_code_size;
        debug("image trace object") << i++ <<" reference set size: "<< object_reference_set_size;
        debug("image trace object") << i++ <<" marker set size: "<< object_marker_set_size;
        debug("image trace object") << i++ <<" view set size: "<< object_view_set_size;

        for(; i < this->data(j) + 5 + object_code_size; ++i){
            debug("image trace object") << i;
            ((Atom *)&this->data(i))->trace();
        }

        for(; i <this->data(j) + 5 + object_code_size + object_reference_set_size; ++i)
            debug("image trace reference") << i << this->data(i);

        for(; i < this->data(j) + 5 + object_code_size + object_reference_set_size + object_marker_set_size; ++i)
            debug("image trace code") << i << this->data(i);

        std::cout<<"---view set---\n";
        for(size_t k=0;k< object_view_set_size; ++k){

            size_t view_code_size=this->data(i);
            size_t view_reference_set_size=this->data(i+1);

            debug("image trace view") << "view" << k << "";
            debug("image trace view") << i++ << "code size:" << view_code_size;
            debug("image trace view") << i++ << "reference set size:" << view_reference_set_size;

            std::cout<<"---code---\n";
            size_t l;
            for(l = 0; l < view_code_size; ++i, ++l) {
                debug("image trace code") << i;
                ((Atom *)&this->data(i))->trace();
            }

            for(l=0;l<view_reference_set_size;++i,++l)
                debug("image trace reference") << i << this->data(i);
        }
    }
}
}
