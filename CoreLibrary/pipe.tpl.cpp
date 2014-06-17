//	pipe1.tpl.cpp
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

#include <memory.h>


namespace core {

#ifdef PIPE_1
template<typename T, uint64 _S> Pipe11<T, _S>::Pipe11(): Semaphore(0, 65535) {

    head = tail = -1;
    first = last = new Block(NULL);
    spare = NULL;
}

template<typename T, uint64 _S> Pipe11<T, _S>::~Pipe11() {

    delete first;
    if (spare)
        delete spare;
}

template<typename T, uint64 _S> inline void Pipe11<T, _S>::_clear() { // leaves spare as is

    enter();
    reset();
    if (first->next)
        delete first->next;
    first->next = NULL;
    head = tail = -1;
    leave();
}

template<typename T, uint64 _S> inline T Pipe11<T, _S>::_pop() {

    T t = first->buffer[head];
    if (++head == _S) {

        enter();
        if (first == last)
            head = tail = -1; // stay in the same block; next push will reset head and tail to 0
        else {

            if (!spare) {

                spare = first;
                first = first->next;
                spare->next = NULL;
            } else {

                Block *b = first->next;
                first->next = NULL;
                delete first;
                first = b;
            }
            head = 0;
        }
        leave();
    }
    return t;
}

template<typename T, uint64 _S> inline void Pipe11<T, _S>::push(T &t) {

    enter();
    if (++tail == 0)
        head = 0;
    uint64 index = tail;
    if (tail == _S) {

        if (spare) {

            last->next = spare;
            last = spare;
            last->next = NULL;
            spare = NULL;
        } else
            last = new Block(last);
        tail = 0;
        index = tail;
    }
    leave();

    last->buffer[index] = t;
    release();
}

template<typename T, uint64 _S> inline T Pipe11<T, _S>::pop() {

    Semaphore::acquire();
    return _pop();
}

template<typename T, uint64 _S> inline void Pipe11<T, _S>::clear() {

    _clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint64 _S> Pipe1N<T, _S>::Pipe1N() {
}

template<typename T, uint64 _S> Pipe1N<T, _S>::~Pipe1N() {
}

template<typename T, uint64 _S> void Pipe1N<T, _S>::clear() {

    popCS.enter();
    Pipe11<T, _S>::_clear();
    popCS.leave();
}

template<typename T, uint64 _S> T Pipe1N<T, _S>::pop() {

    Semaphore::acquire();
    popCS.enter();
    T t = Pipe11<T, _S>::_pop();
    popCS.leave();
    return t;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint64 _S> PipeN1<T, _S>::PipeN1() {
}

template<typename T, uint64 _S> PipeN1<T, _S>::~PipeN1() {
}

template<typename T, uint64 _S> void PipeN1<T, _S>::clear() {

    pushCS.enter();
    Pipe11<T, _S>::_clear();
    pushCS.leave();
}

template<typename T, uint64 _S> void PipeN1<T, _S>::push(T &t) {

    pushCS.enter();
    Pipe11<T, _S>::push(t);
    pushCS.leave();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint64 _S> PipeNN<T, _S>::PipeNN() {
}

template<typename T, uint64 _S> PipeNN<T, _S>::~PipeNN() {
}

template<typename T, uint64 _S> void PipeNN<T, _S>::clear() {

    pushCS.enter();
    popCS.enter();
    Pipe11<T, _S>::_clear();
    popCS.leave();
    pushCS.leave();
}

template<typename T, uint64 _S> void PipeNN<T, _S>::push(T &t) {

    pushCS.enter();
    Pipe11<T, _S>::push(t);
    pushCS.leave();
}

template<typename T, uint64 _S> T PipeNN<T, _S>::pop() {

    Semaphore::acquire();
    popCS.enter();
    T t = Pipe11<T, _S>::_pop();
    popCS.leave();
    return t;
}
#elif defined PIPE_2
template<typename T, uint64 _S, typename Head, typename Tail, class P, template<typename, uint64, class> class Push, template<typename, uint64, class> class Pop> Pipe<T, _S, Head, Tail, P, Push, Pop>::Pipe(): Semaphore(0, 1) {

    head = -1;
    tail = 0;

    waitingList = 0;

    first = last = new Block(NULL);
    spare = new Block(NULL);

    _push = new Push<T, _S, P>(*(P *)this);
    _pop = new Pop<T, _S, P>(*(P *)this);
}

template<typename T, uint64 _S, typename Head, typename Tail, class P, template<typename, uint64, class> class Push, template<typename, uint64, class> class Pop> Pipe<T, _S, Head, Tail, P, Push, Pop>::~Pipe() {

    delete first;
    if (spare)
        delete spare;
    delete _push;
    delete _pop;
}

template<typename T, uint64 _S, typename Head, typename Tail, class P, template<typename, uint64, class> class Push, template<typename, uint64, class> class Pop> inline void Pipe<T, _S, Head, Tail, P, Push, Pop>::shrink() {

    if (!spare) {

        spare = first;
        first = first->next;
        spare->next = NULL;
    } else {

        Block *b = first->next;
        first->next = NULL;
        delete first;
        first = b;
    }
    head = -1;
}

template<typename T, uint64 _S, typename Head, typename Tail, class P, template<typename, uint64, class> class Push, template<typename, uint64, class> class Pop> inline void Pipe<T, _S, Head, Tail, P, Push, Pop>::grow() {

    if (spare) {

        last->next = spare;
        last = spare;
        last->next = NULL;
        spare = NULL;
    } else
        last = new Block(last);
    tail = 0;
}

template<typename T, uint64 _S, typename Head, typename Tail, class P, template<typename, uint64, class> class Push, template<typename, uint64, class> class Pop> inline void Pipe<T, _S, Head, Tail, P, Push, Pop>::push(T &t) {

    (*_push)(t);
}

template<typename T, uint64 _S, typename Head, typename Tail, class P, template<typename, uint64, class> class Push, template<typename, uint64, class> class Pop> inline T Pipe<T, _S, Head, Tail, P, Push, Pop>::pop() {

    return (*_pop)();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Pipe> PipeFunctor<Pipe>::PipeFunctor(Pipe &p): pipe(p) {
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint64 _S, class Pipe> Push1<T, _S, Pipe>::Push1(Pipe &p): PipeFunctor<Pipe>(p) {
}

template<typename T, uint64 _S, class Pipe> void Push1<T, _S, Pipe>::operator()(T &t) {

    pipe.last->buffer[pipe.tail] = t;

    if (++pipe.tail == (int64)_S)
        pipe.grow();

    int64 count = Atomic::Decrement32(&pipe.waitingList);
    if (count >= 0) // at least one reader is waiting
        pipe.release(); // unlock one reader
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint64 _S, class Pipe> PushN<T, _S, Pipe>::PushN(Pipe &p): PipeFunctor<Pipe>(p), Semaphore(0, 1) {
}

template<typename T, uint64 _S, class Pipe> void PushN<T, _S, Pipe>::operator()(T &t) {

check_tail: int64 tail = Atomic::Increment32(&pipe.tail) - 1;

    if (tail < (int64)_S)
        pipe.last->buffer[tail] = t;
    else if (tail == (int64)_S) {

        pipe.grow(); // tail set to 0

        pipe.last->buffer[pipe.tail++] = t;

        release(); // unlock writers
        acquire(); // make sure the sem falls back to 0
    } else { // tail>_S: pipe.last and pipe.tail are being changed

        acquire(); // wait
        release(); // unlock other writers
        goto check_tail;
    }

    int64 count = Atomic::Decrement32(&pipe.waitingList);
    if (count >= 0) // at least one reader is waiting
        pipe.release(); // unlock one reader
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint64 _S, class Pipe> Pop1<T, _S, Pipe>::Pop1(Pipe &p): PipeFunctor<Pipe>(p) {
}

template<typename T, uint64 _S, class Pipe> T Pop1<T, _S, Pipe>::operator()() {

    int64 count = Atomic::Increment32(&pipe.waitingList);
    if (count > 0) // no free lunch
        pipe.acquire(); // wait for a push

    if (pipe.head == (int64)_S - 1)
        pipe.shrink();

    T t = pipe.first->buffer[++pipe.head];

    return t;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint64 _S, class Pipe> PopN<T, _S, Pipe>::PopN(Pipe &p): PipeFunctor<Pipe>(p), Semaphore(0, 1) {
}

template<typename T, uint64 _S, class Pipe> T PopN<T, _S, Pipe>::operator()() {

    int64 count = Atomic::Increment32(&pipe.waitingList);
    if (count > 0) // no free lunch
        pipe.acquire(); // wait for a push

check_head: int64 head = Atomic::Increment32(&pipe.head);
    if (head < (int64)_S)
        return pipe.first->buffer[head];

    if (head == (int64)_S) {

        pipe.shrink(); // head set to -1

        release(); // unlock readers
        acquire(); // make sure the sem falls back to 0

        return pipe.first->buffer[++pipe.head];
    } else { // head>_S: pipe.first and pipe.head are being changed

        acquire(); // wait
        release(); // unlock other readers
        goto check_head;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint64 S> Pipe11<T, S>::Pipe11(): Pipe<T, S, int64, int64, Pipe11<T, S>, Push1, Pop1>() {
}

template<typename T, uint64 S> Pipe11<T, S>::~Pipe11() {
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint64 S> Pipe1N<T, S>::Pipe1N(): Pipe<T, S, int64, int64 volatile, Pipe1N, Push1, PopN>() {
}

template<typename T, uint64 S> Pipe1N<T, S>::~Pipe1N() {
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint64 S> PipeN1<T, S>::PipeN1(): Pipe<T, S, int64 volatile, int64, PipeN1, PushN, Pop1>() {
}

template<typename T, uint64 S> PipeN1<T, S>::~PipeN1() {
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint64 S> PipeNN<T, S>::PipeNN(): Pipe<T, S, int64 volatile, int64 volatile, PipeNN, PushN, PopN>() {
}

template<typename T, uint64 S> PipeNN<T, S>::~PipeNN() {
}
#endif
}
