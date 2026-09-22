
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
/*  FILE:         url.c                                                      */
/*  MODULE:       URL encoding and decoding.                                 */
/*                                                                           */
/*  Reference:  http://www.w3.org/Addressing/rfc1808.txt.                    */
/*                                                                           */
/*  The assumption here is that every character in the range 1 to 255        */
/*  inclusive can be encoded in the URL encoding, and that this is done      */
/*  by a one-byte direct translation in the case of the characters listed    */
/*  below (taken from the reference), and by a three-byte sequence           */
/*  consisting of a % followed by two hexadecimal digits otherwise, except   */
/*  that the space character is encoded by a + sign.                         */
/*                                                                           */
/*  I call this an assumption because I have not been able to verify it,     */
/*  merely to infer it.                                                      */
/*                                                                           */
/*****************************************************************************/
#include "url.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  void URLInitialize()                                                     */
/*                                                                           */
/*  Initialize the part of this module that does encoding.                   */
/*                                                                           */
/*****************************************************************************/
#define MAX_CHAR 256

static bool initialized = false;		/* true when initialized     */
static bool ok_as_is[MAX_CHAR];			/* true if no recode needed  */

static void set_ok(char *str)
{
  int i;
  for( i = 0;  str[i] != '\0';  i++ )
    ok_as_is[ (unsigned) str[i]] = true;
}

static void URLInitializeEncode()
{
  /* by default every character needs recoding */
  int ch;
  for( ch = 0;  ch < MAX_CHAR;  ch++ )
    ok_as_is[ch] = false;

  /* the characters in these strings don't need recoding */
  set_ok("abcdefghijklmnopqrstuvwxyz");
  set_ok("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  set_ok("0123456789");
  set_ok("$-_.");
  set_ok("!*'(),");
  set_ok(";/:@=");
  initialized = true;
}


/*****************************************************************************/
/*                                                                           */
/*  int FromHex(char ch)                                                     */
/*                                                                           */
/*  Interpret ch as a hexadecimal digit and return its value.  If no         */
/*  such interpretation is possible, return -1.                              */
/*                                                                           */
/*****************************************************************************/

static int FromHex(char ch)
{
  if( ch >= '0' && ch <= '9' )
    return ch - '0';
  else if( ch >= 'A' && ch <= 'F' )
    return ch - 'A' + 10;
  else if( ch >= 'a' && ch <= 'f' )
    return ch - 'a' + 10;
  else
    return -1;
}


/*****************************************************************************/
/*                                                                           */
/*  char ToHex(int i)                                                        */
/*                                                                           */
/*  Return a one-digit hexadecimal number whose value is i.                  */
/*  Fail if this is not possible.                                            */
/*                                                                           */
/*****************************************************************************/

static char ToHex(int i)
{
  assert(i >= 0 && i < 16);
  if( i < 10 )
    return '0' + i;
  else
    return 'A' + (i - 10);
}


/*****************************************************************************/
/*                                                                           */
/*  char *URLEncode(char *str)                                               */
/*                                                                           */
/*  Encode str in the URL encoding.  Any characters that cannot be so        */
/*  encoded are skipped.  The result is returned in malloced memory.         */
/*                                                                           */
/*****************************************************************************/

#define first_four_bits(ch)	( ((ch) & 0xF0) >> 4 )
#define last_four_bits(ch)	( (ch) & 0x0F )

char *URLEncode(char *str)
{
  int i, j, len;  char *res;
  if( !initialized )
    URLInitializeEncode();
  len = strlen(str);
  res = (char *) malloc(sizeof(char) *(3*len + 1));
  j = 0;
  for( i = 0;  i < len;  i++ )
  {
    if( str[i] >= '\0' && ok_as_is[(unsigned) str[i]] )
      res[j++] = str[i];
    else if( str[i] == ' ')
      res[j++] = '+';
    else
    {
      res[j++] = '%';
      res[j++] = ToHex(first_four_bits(str[i]));
      res[j++] = ToHex(last_four_bits(str[i]));
    }
  }
  res[j++] = '\0';
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool contains(char *str, char ch)                                        */
/*                                                                           */
/*  Return true if str contains ch.                                          */
/*                                                                           */
/*****************************************************************************/

static bool contains(char *str, char ch)
{
  int i;
  for( i = 0;  str[i] != '\0';  i++ )
    if( str[i] == ch )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  char *URLDecodePartial(char *str, char *stoppers, char **stop_point)     */
/*                                                                           */
/*  Decode str up to but not including its first character in stoppers,      */
/*  returning the decoded string in malloced memory and setting *stop_point  */
/*  to point to the first character not decoded.  This will either be an     */
/*  element of stoppers or else it will be '\0'.                             */
/*                                                                           */
/*  The stopping criterion is tested on the undecoded characters, but        */
/*  really it would not be wise to have stoppers that assumed this.          */
/*                                                                           */
/*****************************************************************************/

char *URLDecodePartial(char *str, char *stoppers, char **stop_point)
{
  int i, j, d1, d2, len;  char *res;
  if( DEBUG1 )
    fprintf(stderr, "[ URLDecodePartial(%s)\n", str);

  /* first, work out the number of characters from str to the first stopper */
  for( len = 0;  str[len] != '\0' && !contains(stoppers, str[len]);  len++ );

  /* now malloc that much space and do the conversion */
  res = malloc(sizeof(char) * (len + 1));
  j = 0;
  for( i = 0;  i < len;  i++ )
  {
    switch( str[i] )
    {
      case '%':

	if( (d1 = FromHex(str[i+1])) == -1 || (d2 = FromHex(str[i+2])) == -1 )
	  res[j++] = '%';
	else
	{
	  res[j++] = (char) ((d1 << 4) + d2);
	  i += 2;
	}
	break;


      case '+':

	res[j++] = ' ';
	break;


      default:

	res[j++] = str[i];
	break;
    }
  }
  res[j++] = '\0';
  *stop_point = &str[i];
  if( DEBUG1 )
    fprintf(stderr, "] URLDecodePartial returning %s\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *URLDecode(char *str)                                               */
/*                                                                           */
/*  Decode URL-encoded string str into a Unicode string and return it.       */
/*  This operation always succeeds.  It treats any malformed escape          */
/*  sequences as though they were not escape sequences at all.               */
/*                                                                           */
/*****************************************************************************/

char *URLDecode(char *str)
{
  char *stop_point;
  return URLDecodePartial(str, "", &stop_point);
}
