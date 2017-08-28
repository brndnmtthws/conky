/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2010 Pavel Labath et al.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SEMAPHORE_HH
#define SEMAPHORE_HH

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#if defined (__APPLE__)

//
//  On Darwin, unnamed semaphores are not supported!
//  The only close equivalent to unnamed semaphores is using
//      GCD!
//

#include <dispatch/dispatch.h>

class semaphore {
    dispatch_semaphore_t    sem;
    
    semaphore(const semaphore &) = delete;
    semaphore& operator=(const semaphore &) = delete;
public:
    semaphore(unsigned int value = 0) throw(std::logic_error)
    {
        printf( "conky: EXPERIMENTAL semaphore implementation is used ( for Darwin ).\nANYONE WHO WANTS TO HELP VISIT THE PORT PROJECT IN GITHUB: https://github.com/npyl/conky \n" );
        
        sem = dispatch_semaphore_create(value);
    }
    
    ~semaphore() throw()
    {
        printf( "conky: ~semaphore: not implemented yet!\n" );
    }
    void post() throw(std::overflow_error)
    {
        dispatch_semaphore_signal(sem);
    }
    
    void wait() throw()
    {
        dispatch_semaphore_wait( sem, DISPATCH_TIME_FOREVER);
    }
    
    bool trywait() throw()
    {
        printf( "semaphore: trywait() not implemented!\n" );
        return false;
    }
};


#else

#include <semaphore.h>

class semaphore {
    sem_t sem;
    
    semaphore(const semaphore &) = delete;
    semaphore& operator=(const semaphore &) = delete;
public:
    semaphore(unsigned int value = 0) throw(std::logic_error)
    {
        if(sem_init(&sem, 0, value))
            throw std::logic_error(strerror(errno));
    }
    
    ~semaphore() throw()
    { sem_destroy(&sem); }
    
    void post() throw(std::overflow_error)
    {
        if(sem_post(&sem))
            throw std::overflow_error(strerror(errno));
    }
    
    void wait() throw()
    {
        while(sem_wait(&sem)) {
            if(errno != EINTR)
                abort();
        }
    }
    
    bool trywait() throw()
    {
        while(sem_trywait(&sem)) {
            
            if(errno == EAGAIN)
                return false;
            else if(errno != EINTR)
                abort();
        }
        return true;
    }
};

#endif /* defined (__APPLE__) */

#endif
