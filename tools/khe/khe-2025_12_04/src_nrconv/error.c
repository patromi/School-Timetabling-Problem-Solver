
/*****************************************************************************/
/*                                                                           */
/*  THE NRCONV NURSE ROSTERING TO XHSTT CONVERTER                            */
/*  COPYRIGHT (C) 2016, Jeffrey H. Kingston                                  */
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
/*  FILE:         error.c                                                    */
/*  MODULE:       Error messages                                             */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"


/*****************************************************************************/
/*                                                                           */
/*  Submodule "error handling"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void Error(char *file_name, int line_num, char *fmt, ...)                */
/*                                                                           */
/*  Print an error message.                                                  */
/*                                                                           */
/*****************************************************************************/

void Warning(char *file_name, int line_num, char *fmt, ...)
{
  va_list args;
  if( file_name == NULL )
    fprintf(stderr, "nrconv: ");
  else if( line_num <= 0 )
    fprintf(stderr, "%s: ", file_name);
  else
    fprintf(stderr, "%s:%d: ", file_name, line_num);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void FatalError(char *file_name, int line_num, char *fmt, ...)           */
/*                                                                           */
/*  Print an error message and exit.                                         */
/*                                                                           */
/*****************************************************************************/

void FatalError(char *file_name, int line_num, char *fmt, ...)
{
  va_list args;
  if( file_name == NULL )
    fprintf(stderr, "nrconv: ");
  else if( line_num <= 0 )
    fprintf(stderr, "%s: ", file_name);
  else
    fprintf(stderr, "%s:%d: ", file_name, line_num);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlError(KML_ERROR ke, char *file_name)                             */
/*                                                                           */
/*  Print the error represented by ke.                                       */
/*                                                                           */
/*****************************************************************************/

void KmlWarning(KML_ERROR ke, char *file_name)
{
  fprintf(stderr, "%s:%d:%d: %s\n", file_name, KmlErrorLineNum(ke),
    KmlErrorColNum(ke), KmlErrorString(ke));
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlFatalError(KML_ERROR ke, char *file_name)                        */
/*                                                                           */
/*  Print the error represented by ke and exit.                              */
/*                                                                           */
/*****************************************************************************/

void KmlFatalError(KML_ERROR ke, char *file_name)
{
  fprintf(stderr, "%s:%d:%d: %s\n", file_name, KmlErrorLineNum(ke),
    KmlErrorColNum(ke), KmlErrorString(ke));
  exit(1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlEltError(KML_ELT elt, char *file_name, char *fmt, ...)           */
/*                                                                           */
/*  Print an error message located at elt.                                   */
/*                                                                           */
/*****************************************************************************/

void KmlEltWarning(KML_ELT elt, char *file_name, char *fmt, ...)
{
  va_list args;
  if( elt != NULL )
    fprintf(stderr, "%s:%d:%d: ", file_name, KmlLineNum(elt),
      KmlColNum(elt));
  else
    fprintf(stderr, "%s: ", file_name);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KmlEltFatalError(KML_ELT elt, char *file_name, char *fmt, ...)      */
/*                                                                           */
/*  Print a fatal error message located at elt.                              */
/*                                                                           */
/*****************************************************************************/

void KmlEltFatalError(KML_ELT elt, char *file_name, char *fmt, ...)
{
  va_list args;
  if( elt != NULL )
    fprintf(stderr, "%s:%d:%d: ", file_name, KmlLineNum(elt),
      KmlColNum(elt));
  else
    fprintf(stderr, "%s: ", file_name);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(1);
}
