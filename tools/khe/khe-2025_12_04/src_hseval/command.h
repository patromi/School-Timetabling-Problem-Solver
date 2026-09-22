
/*****************************************************************************/
/*                                                                           */
/*  THE HSEVAL HIGH SCHOOL TIMETABLE EVALUATOR                               */
/*  COPYRIGHT (C) 2009, Jeffrey H. Kingston                                  */
/*                                                                           */
/*  Jeffrey H. Kingston (jeff@it.usyd.edu.au)                                */
/*  School of Information Technologies                                       */
/*  The University of Sydney 2006                                            */
/*  AUSTRALIA                                                                */
/*                                                                           */
/*  This program is free software; you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation; either Version 3, or (at your option)      */
/*  any later version.                                                       */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program; if not, write to the Free Software              */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston MA 02111-1307 USA   */
/*                                                                           */
/*  FILE:         command.h                                                  */
/*  MODULE:       Reading CGI or command line data                           */
/*                                                                           */
/*****************************************************************************/
#ifndef COMMAND_HEADER_FILE
#define COMMAND_HEADER_FILE
#include <stdbool.h>
#include <stdio.h>
#include "html.h"

typedef struct command_rec *COMMAND;

typedef HTML(*COMMAND_PAGE_BEGIN_FN)(char *title);
typedef void(*COMMAND_PAGE_END_FN)(HTML html);

extern COMMAND CommandBeginCGI(bool rerun, COMMAND_PAGE_BEGIN_FN page_begin_fn,
  COMMAND_PAGE_END_FN page_end_fn, HA_ARENA a);
extern COMMAND CommandBeginCommandLine(char *argv[], int argc, int start_pos,
  HA_ARENA a);
extern FILE *CommandRerunFile(COMMAND c);
extern bool CommandNextPair(COMMAND c, char **key, char **val, FILE **file_val);
/* extern char *CommandNextOp(COMMAND c); */
/* extern char *CommandNextOneLineVal(COMMAND c); */
/* extern FILE *CommandFile(COMMAND c); */
extern void CommandError(COMMAND c, char *fmt, ...);
extern void CommandEnd(COMMAND c);

#endif
