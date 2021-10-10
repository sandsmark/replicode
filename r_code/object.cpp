//	object.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel
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

#include "object.h"

#include <r_code/object.h>  // for SysObject, SysView, Code, View, Mem, etc
#include <cstdint>          // for uint32_t
#include <iostream>         // for operator<<, cout, ostream, endl, etc


namespace r_code
{

SysView::SysView()
{
}

SysView::SysView(View *source)
{
    for (size_t i = 0; i < VIEW_CODE_MAX_SIZE; ++i) {
        code[i] = source->code(i);
    }

    for (size_t i = 0; i < 2; ++i) // to get the right size in Image::add_object().
        if (source->references[i]) {
            references.push_back(0);
        }
}

void SysView::write(uint32_t *data)
{
    data[0] = code.size();
    data[1] = references.size();
    size_t i = 0;

    for (; i < code.size(); ++i) {
        data[2 + i] = code[i].atom;
    }

    for (size_t j = 0; j < references.size(); ++j) {
        data[2 + i + j] = references[j];
    }
}

void SysView::read(uint32_t *data)
{
    size_t code_size = data[0];
    size_t reference_set_size = data[1];
    size_t i;
    size_t j;

    for (i = 0; i < code_size; ++i) {
        code.push_back(Atom(data[2 + i]));
    }

    for (j = 0; j < reference_set_size; ++j) {
        references.push_back(data[2 + i + j]);
    }
}

size_t SysView::get_size() const
{
    return 2 + code.size() + references.size();
}

void SysView::trace()
{
    std::cout << " code size: " << code.size() << std::endl;
    std::cout << " reference set size: " << references.size() << std::endl;
    std::cout << "---code---" << std::endl;

    for (const Atom &atom : code) {
        atom.trace();
        std::cout << std::endl;
    }

    std::cout << "---reference set---" << std::endl;

    for (uint32_t reference : references) {
        std::cout << reference << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t SysObject::LastOID = 0;

SysObject::SysObject(): oid(LastOID++)
{
}

SysObject::SysObject(Code *source)
{
    size_t i;

    for (i = 0; i < source->code_size(); ++i) {
        code[i] = source->code(i);
    }

    std::unordered_set<View *, View::Hash, View::Equal>::const_iterator v;
    source->acq_views();

    for (i = 0, v = source->views.begin(); v != source->views.end(); ++i, ++v) {
        views[i] = new SysView(*v);
    }

    source->rel_views();
    oid = source->get_oid();

    for (i = 0; i < source->references_size(); ++i) { // to get the right size in Image::add_object().
        references.push_back(0);
    }
}

SysObject::~SysObject()
{
    for (SysView *view : views) {
        delete view;
    }
}

void SysObject::write(uint32_t *data)
{
    data[0] = oid;
    data[1] = code.size();
    data[2] = references.size();
    data[3] = markers.size();
    data[4] = views.size();
    size_t i;
    size_t j;
    size_t k;
    size_t l;

    for (i = 0; i < code.size(); ++i) {
        data[5 + i] = code[i].atom;
    }

    for (j = 0; j < references.size(); ++j) {
        data[5 + i + j] = references[j];
    }

    for (k = 0; k < markers.size(); ++k) {
        data[5 + i + j + k] = markers[k];
    }

    size_t offset = 0;

    for (l = 0; l < views.size(); ++l) {
        views[l]->write(data + 5 + i + j + k + offset);
        offset += views[l]->get_size();
    }
}

void SysObject::read(uint32_t *data)
{
    oid = data[0];
    size_t code_size = data[1];
    size_t reference_set_size = data[2];
    size_t marker_set_size = data[3];
    size_t view_set_size = data[4];
    size_t i;
    size_t j;
    size_t k;
    size_t l;

    for (i = 0; i < code_size; ++i) {
        code.push_back(Atom(data[5 + i]));
    }

    for (j = 0; j < reference_set_size; ++j) {
        references.push_back(data[5 + i + j]);
    }

    for (k = 0; k < marker_set_size; ++k) {
        markers.push_back(data[5 + i + j + k]);
    }

    size_t offset = 0;

    for (l = 0; l < view_set_size; ++l) {
        SysView *v = new SysView();
        v->read(data + 5 + i + j + k + offset);
        views.push_back(v);
        offset += v->get_size();
    }
}

size_t SysObject::get_size()
{
    size_t view_set_size = 0;

    for (SysView *view : views) {
        view_set_size += view->get_size();
    }

    return 5 + code.size() + references.size() + markers.size() + view_set_size;
}

void SysObject::trace()
{
    std::cout << "\n---object---\n";
    std::cout << oid << std::endl;
    std::cout << "code size: " << code.size() << std::endl;
    std::cout << "reference set size: " << references.size() << std::endl;
    std::cout << "marker set size: " << markers.size() << std::endl;
    std::cout << "view set size: " << views.size() << std::endl;
    std::cout << "\n---code---\n";
    size_t i;

    for (i = 0; i < code.size(); ++i) {
        std::cout << i << " ";
        code[i].trace();
        std::cout << std::endl;
    }

    std::cout << "\n---reference set---\n";

    for (i = 0; i < references.size(); ++i) {
        std::cout << i << " " << references[i] << std::endl;
    }

    std::cout << "\n---marker set---\n";

    for (i = 0; i < markers.size(); ++i) {
        std::cout << i << " " << markers[i] << std::endl;
    }

    std::cout << "\n---view set---\n";

    for (size_t k = 0; k < views.size(); ++k) {
        std::cout << "view[" << k << "]" << std::endl;
        std::cout << "reference set size: " << views[k]->references.size() << std::endl;
        std::cout << "-code-" << std::endl;
        size_t j;

        for (j = 0; j < views[k]->code.size(); ++i, ++j) {
            std::cout << j << " ";
            views[k]->code[j].trace();
            std::cout << std::endl;
        }

        std::cout << "-reference set-" << std::endl;

        for (j = 0; j < views[k]->references.size(); ++i, ++j) {
            std::cout << j << " " << views[k]->references[j] << std::endl;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Mem *Mem::Singleton = nullptr;

Mem::Mem()
{
    Singleton = this;
}

Mem *Mem::Get()
{
    return Singleton;
}
}
