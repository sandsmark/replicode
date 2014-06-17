//	pipe.h
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

#ifndef core_pipe_h
#define core_pipe_h

#include "utils.h"


#define PIPE_1

namespace core {

// Pipes are thread safe, depending on their type:
// 11: 1 writer, 1 reader
// 1N: 1 writer, N readers
// N1: N writers, 1 reader
// NN: N writers, N readers
#ifdef PIPE_1
template<typename T, uint64 _S> class Pipe11 : public Semaphore {
private:
    class Block {
    public:
        T buffer[_S*sizeof(T)];
        Block *next;
        Block(Block *prev): next(NULL) {
            if (prev)prev->next = this;
        }
        ~Block() {
            if (next) delete next;
        }
    };
    int64 head;
    int64 tail;
    Block *first;
    Block *last;
    Block *spare;
protected:
    /// leaves spare as is
    void _clear();
    T _pop();
    std::mutex m_mutex;
public:
    Pipe11();
    ~Pipe11();
    void clear();
    void push(T &t); // increases the size as necessary
    T pop(); // decreases the size as necessary
};

template<typename T, uint64 _S> class Pipe1N:
    public Pipe11<T, _S> {
private:
    std::mutex m_popMutex;
public:
    Pipe1N();
    ~Pipe1N();
    void clear();
    T pop();
};

template<typename T, uint64 _S> class PipeN1:
    public Pipe11<T, _S> {
private:
    std::mutex m_pushMutex;
public:
    PipeN1();
    ~PipeN1();
    void clear();
    void push(T &t);
};

template<typename T, uint64 _S> class PipeNN:
    public Pipe11<T, _S> {
private:
    std::mutex m_pushMutex;
    std::mutex m_popMutex;
public:
    PipeNN();
    ~PipeNN();
    void clear();
    void push(T &t);
    T pop();
};
#elif defined PIPE_2
template<typename T, uint64 _S, class Pipe> class Push1;
template<typename T, uint64 _S, class Pipe> class PushN;
template<typename T, uint64 _S, class Pipe> class Pop1;
template<typename T, uint64 _S, class Pipe> class PopN;

// a Pipe<T,_S> is a linked list of blocks containing _S objects of type T
// push() adds an object at the tail of the last block and moves the tail forward; when the tail reaches the end of the last block, a new block is appended to the list
// pop() moves the head forward and returns the object at this location; when the head reaches the end of the first block, this block is deallocated
// Synchronization between readers and writers is lock-free under no contention (reqCount), and uses a lock (Semaphore::) under contention
// single writer pipes can use an int64 tail, whereas multiple writer versions require int64 volatile tail; idem for readers
// The Head and Tail argument is meant to allow the parameterizing of heads and tails
// Push and Pop are functors tailored to the multiplicity of resp. the read and write threads
// P is the actual pipe class (e.g. Pipe11, etc.)
template<typename T, uint64 _S, typename Head, typename Tail, class P, template<typename, uint64, class> class Push, template<typename, uint64, class> class Pop> class Pipe:
    public Semaphore {
    template<typename T, uint64 _S, class Pipe> friend class Push1;
    template<typename T, uint64 _S, class Pipe> friend class PushN;
    template<typename T, uint64 _S, class Pipe> friend class Pop1;
    template<typename T, uint64 _S, class Pipe> friend class PopN;
protected:
    class Block {
    public:
        T buffer[_S];
        Block *next;
        Block(Block *prev): next(NULL) {
            if (prev)prev->next = this;
        }
        ~Block() {
            if (next) delete next;
        }
    };
    Block *first;
    Block *last;
    Block *spare; // pipes always retain a spare block: if a block is to be deallocated and there is no spare, it becomes the spare

    Head head; // starts at -1
    Tail tail; // starts at 0
    int64 volatile waitingList; // amount of readers that have to wait, negative value indicate free lunch

    Push<T, _S, P> *_push;
    Pop<T, _S, P> *_pop;

    void shrink(); // deallocates the first block when head reaches its end
    void grow(); // allocate a new last block when tail reaches its end

    Pipe();
public:
    ~Pipe();
    void push(T &t);
    T pop();
};

template<class Pipe> class PipeFunctor {
protected:
    Pipe &const pipe;
    PipeFunctor(Pipe &p);
};

template<typename T, uint64 _S, class Pipe> class Push1:
    public PipeFunctor<Pipe> {
public:
    Push1(Pipe &p);
    void operator()(T &t);
};

template<typename T, uint64 _S, class Pipe> class PushN:
    public PipeFunctor<Pipe>,
    public Semaphore {
public:
    PushN(Pipe &p);
    void operator()(T &t);
};

template<typename T, uint64 _S, class Pipe> class Pop1:
    public PipeFunctor<Pipe> {
public:
    Pop1(Pipe &p);
    T operator()();
};

template<typename T, uint64 _S, class Pipe> class PopN:
    public PipeFunctor<Pipe>,
    public Semaphore {
public:
    PopN(Pipe &p);
    T operator()();
};

template<typename T, uint64 S> class Pipe11:
    public Pipe<T, S, int64, int64, Pipe11<T, S>, Push1, Pop1> {
public:
    Pipe11();
    ~Pipe11();
};

template<typename T, uint64 S> class Pipe1N:
    public Pipe<T, S, int64, int64 volatile, Pipe1N<T, S>, Push1, PopN> {
public:
    Pipe1N();
    ~Pipe1N();
};

template<typename T, uint64 S> class PipeN1:
    public Pipe<T, S, int64 volatile, int64, PipeN1<T, S>, PushN, Pop1> {
public:
    PipeN1();
    ~PipeN1();
};

template<typename T, uint64 S> class PipeNN:
    public Pipe<T, S, int64 volatile, int64 volatile, PipeNN<T, S>, PushN, PopN> {
public:
    PipeNN();
    ~PipeNN();
};
#endif
}


#include "pipe.tpl.cpp"


#endif
