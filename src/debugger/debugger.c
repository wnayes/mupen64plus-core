/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - debugger.c                                              *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 DarkJeztr                                          *
 *   Copyright (C) 2002 davFr                                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <SDL.h>

#include "api/debugger.h"

#include "dbg_types.h"
#include "debugger.h"
#include "dbg_breakpoints.h"
#include "dbg_memory.h"

// State of the Emulation Thread:
// 0 -> pause, 2 -> run.

int g_DebuggerActive = 0;    // whether the debugger is enabled or not
int run;

// Holds the number of pending steps the debugger needs to perform.
static SDL_sem *sem_pending_steps;

uint32 previousPC;

//]=-=-=-=-=-=-=-=-=-=-=[ Initialisation du Debugger ]=-=-=-=-=-=-=-=-=-=-=-=[

void init_debugger()
{
    g_DebuggerActive = 1;
    run = 0;

    DebuggerCallback(DEBUG_UI_INIT, 0); /* call front-end to initialize user interface */

    init_host_disassembler();

    sem_pending_steps = SDL_CreateSemaphore(0);
}

void destroy_debugger()
{
    SDL_DestroySemaphore(sem_pending_steps);
    sem_pending_steps = NULL;
    g_DebuggerActive = 0;
}

//]=-=-=-=-=-=-=-=-=-=-=-=-=[ Mise-a-Jour Debugger ]=-=-=-=-=-=-=-=-=-=-=-=-=[

void update_debugger(uint32 pc)
// Update debugger state and display.
// Should be called after each R4300 instruction
// Checks for breakpoint hits on PC
{
    int bpt;

    if(run!=0) {//check if we hit a breakpoint
        bpt = check_breakpoints(pc);
        if( bpt!=-1 ) {
            run = 0;
            
            if(BPT_CHECK_FLAG(g_Breakpoints[bpt], BPT_FLAG_LOG))
                log_breakpoint(pc, BPT_FLAG_EXEC, 0);
        }
    }

    if(run!=2) {
        DebuggerCallback(DEBUG_UI_UPDATE, pc);  /* call front-end to notify user interface to update */
    }
    if(run==0) {
        // The emulation thread is blocked until a step call via the API.
        SDL_SemWait(sem_pending_steps);
    }

    previousPC = pc;
}

void debugger_step()
{
    SDL_SemPost(sem_pending_steps);
}


