//	utils.cpp
//
//	Author: Eric Nivel, Thor List
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel, Thor List
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel or Thor List nor the
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

#include "sharedlibrary.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <sys/time.h>
#include <string.h>

namespace utils
{

SharedLibrary *SharedLibrary::New(const char *fileName)
{
    SharedLibrary *sl = new SharedLibrary();

    if (sl->load(fileName)) {
        return sl;
    } else {
        delete sl;
        return NULL;
    }
}

SharedLibrary::SharedLibrary(): library(NULL)
{
}

SharedLibrary::~SharedLibrary()
{
#if defined(WIN32) || defined(WIN64)

    if (library) {
        FreeLibrary(library);
    }

#else

    if (library) {
        dlclose(library);
    }

#endif
}

SharedLibrary *SharedLibrary::load(const char *fileName)
{
#if defined(WIN32) || defined(WIN64)
    library = LoadLibrary(TEXT(fileName));

    if (!library) {
        DWORD error = GetLastError();
        std::cerr << "> Error: unable to load shared library " << fileName << " :" << error << std::endl;
        return NULL;
    }

#else
    library = dlopen(fileName, RTLD_NOW | RTLD_GLOBAL);

    if (!library) {
        std::cout << "> Error: unable to load shared library " << fileName << " :" << dlerror() << std::endl;
        return NULL;
    }

#endif
    return this;
}

}
