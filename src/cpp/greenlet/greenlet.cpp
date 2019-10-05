/*
 * This file is part of cgreenlet. CGreenlet is free software available under the terms of the MIT
 * license. Consult the file LICENSE that was shipped together with this source file for the exact
 * licensing terms.
 *
 * Copyright (c) 2012 by the cgreenlet authors. See the file AUTHORS for a
 * full list.
 */

#include "greenlet.h"

#include <k273/logging.h>
#include <k273/strutils.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>


///////////////////////////////////////////////////////////////////////////////
// asm:

#ifdef __cplusplus
extern "C" {
#endif

    int asm_greenlet_savecontext(void *frame);
    void asm_greenlet_switchcontext(void *frame, void (*inject)(void *), void *arg)
        __attribute__((noreturn));
    void asm_greenlet_newstack(void *stack, void (*main)(void *), void *arg)
        __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

///////////////////////////////////////////////////////////////////////////////
/* globals */

const int _greenlet_def_max_stacksize = 2 * 1024 * 1024;

thread_local greenlet_t _root_greenlet = { nullptr,
                                           nullptr,
                                           0,
                                           GREENLET_STARTED };
thread_local greenlet_t *_current_greenlet = nullptr;

thread_local std::vector <greenlet_t*> _dead_greenlets;

///////////////////////////////////////////////////////////////////////////////
/* Stack allocation. */

static void* greenlet_alloc_stack(long *size) {
    long stacksize = *size;
    if (stacksize == 0) {

        struct rlimit rlim;
        if (getrlimit(RLIMIT_STACK, &rlim) < 0) {
	    return nullptr;
        }

        stacksize = rlim.rlim_cur;

        if (stacksize > _greenlet_def_max_stacksize) {
            stacksize = _greenlet_def_max_stacksize;
        }
    }

    // XXX do this once?
    long pagesize = sysconf(_SC_PAGESIZE);
    if ((pagesize < 0) || (stacksize < 2*pagesize)) {
        return nullptr;
    }

    stacksize = (stacksize + pagesize - 1) & ~(pagesize - 1);


    void *stack = ::mmap(nullptr, stacksize,
                         PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (stack == nullptr) {
        return nullptr;
    }

    if (mprotect(stack, pagesize, PROT_NONE) < 0) {
	munmap(stack, stacksize);
	return nullptr;
    }

    *size = stacksize;
    return stack;
}

static void greenlet_dealloc_stack(greenlet_t* g) {
    munmap(g->gr_stack, g->gr_stacksize);
}

///////////////////////////////////////////////////////////////////////////////

static void cleanupDeadGreenlets() {
    if (_dead_greenlets.empty()) {
        return;
    }

    for (greenlet_t *g : _dead_greenlets) {
        ASSERT(g != _current_greenlet);
        greenlet_dealloc_stack(g);
    }

    _dead_greenlets.clear();
}

static void greenlet_start(void *arg) {
    cleanupDeadGreenlets();

    greenlet_t *greenlet = (greenlet_t *) arg;

    _current_greenlet = greenlet;
    greenlet->gr_flags |= GREENLET_STARTED;
    void *ret = greenlet->gr_start(greenlet->gr_arg);
    greenlet->gr_flags |= GREENLET_DEAD;

    while (greenlet->gr_flags & GREENLET_DEAD) {
        _dead_greenlets.push_back(greenlet);
        greenlet = greenlet->gr_parent;
    }

    greenlet->gr_arg = ret;
    _current_greenlet = greenlet;
    asm_greenlet_switchcontext(&greenlet->gr_frame, nullptr, ret);
}

///////////////////////////////////////////////////////////////////////////////

greenlet_t* greenlet_new(greenlet_start_func_t start_func,
                         greenlet_t *parent, long stacksize) {
    greenlet_t* greenlet = (greenlet_t *) calloc(1, sizeof(greenlet_t));
    if (greenlet == nullptr) {
        return nullptr;
    }

    greenlet->gr_parent = parent ? parent : &_root_greenlet;
    if (greenlet->gr_parent == nullptr) {
        return nullptr;
    }

    greenlet->gr_start = start_func;
    greenlet->gr_stacksize = stacksize;
    greenlet->gr_stack = greenlet_alloc_stack(&greenlet->gr_stacksize);
    if (greenlet->gr_stack == nullptr) {
        return nullptr;
    }

    return greenlet;
}

void* greenlet_switch_to(greenlet_t* greenlet, void* arg) {
    cleanupDeadGreenlets();

    greenlet_t* current = greenlet_current();

    if (asm_greenlet_savecontext(&current->gr_frame)) {
        return current->gr_arg;
    }

    if (!(greenlet->gr_flags & GREENLET_STARTED)) {
        greenlet->gr_arg = arg;
        asm_greenlet_newstack((char *) greenlet->gr_stack + greenlet->gr_stacksize,
                              greenlet_start, greenlet);
    }

    while (greenlet->gr_flags & GREENLET_DEAD) {
        _dead_greenlets.push_back(greenlet);
        greenlet = greenlet->gr_parent;
    }

    greenlet->gr_arg = arg;
    _current_greenlet = greenlet;
    asm_greenlet_switchcontext(&greenlet->gr_frame, nullptr, arg);
}

greenlet_t* greenlet_root() {
    return &_root_greenlet;
}

greenlet_t* greenlet_current() {
    greenlet_t* greenlet = _current_greenlet;
    if (greenlet == nullptr) {
        greenlet = greenlet_root();
    }

    return greenlet;
}

greenlet_t* greenlet_parent(greenlet_t* greenlet) {
    return greenlet->gr_parent;
}

int greenlet_isstarted(greenlet_t* greenlet) {
    return (greenlet->gr_flags & GREENLET_STARTED) > 0;
}

int greenlet_isdead(greenlet_t* greenlet) {
    return (greenlet->gr_flags & GREENLET_DEAD) > 0;
}
