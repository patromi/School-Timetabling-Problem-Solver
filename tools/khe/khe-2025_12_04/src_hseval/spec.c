
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
/*  FILE:         spec.c                                                     */
/*  MODULE:       HTML print of specification of the XML format              */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"

/*****************************************************************************/
/*                                                                           */
/*  void ArchiveSpec(HTML html, char *name)                                  */
/*                                                                           */
/*  Print the specification of archives onto html.                           */
/*                                                                           */
/*****************************************************************************/

static void ArchiveSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Archives");
  HTMLParagraphBegin(html);
  HTMLText(html, "An");
  HTMLTextItalic(html, "archive");
  HTMLText(html, "is a collection of instances of the high school");
  HTMLText(html, "timetable problem, together with zero or more solution");
  HTMLText(html, "groups.  Each solution group is a collection of zero or");
  HTMLText(html, "more solutions to the instances of its archive.");
  HTMLText(html, "The syntax of archives is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "HighSchoolTimetableArchive +Id");
  HTMLLiteralTextIndented(html,  8, "+MetaData");
  HTMLLiteralTextIndented(html,  8, "+Instances");
  HTMLLiteralTextIndented(html, 12, "*Instance");
  HTMLLiteralTextIndented(html,  8, "+SolutionGroups");
  HTMLLiteralTextIndented(html, 12, "*SolutionGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "In this notation, placement on the same line indicates");
  HTMLText(html, "that one category is an attribute of another, indenting");
  HTMLText(html, "indicates that one category is a child of another,");
  HTMLMonoBegin(html);
  HTMLText(html, "+");
  HTMLMonoEnd(html);
  HTMLText(html, "indicates that the immediately following category is");
  HTMLText(html, "optional, and");
  HTMLMonoBegin(html);
  HTMLText(html, "*");
  HTMLMonoEnd(html);
  HTMLText(html, "indicates that the immediately following category may");
  HTMLText(html, "appear zero or more times.");
  HTMLText(html, "It is usual in XML to allow the children of a category");
  HTMLText(html, "to appear in any order, but throughout the high school");
  HTMLText(html, "timetable format the children are required to appear in");
  HTMLText(html, "the order given by the syntax specification.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "For example, a file containing");
  HTMLText(html, "the following conforms to this syntax:");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html, 4, "<HighSchoolTimetableArchive Id=\"MyArchive\">");
  HTMLLiteralTextIndented(html, 8, "<Instances>");
  HTMLLiteralTextIndented(html, 12, "<Instance>");
  HTMLLiteralTextIndented(html, 16, "...");
  HTMLLiteralTextIndented(html, 12, "</Instance>");
  HTMLLiteralTextIndented(html, 12, "<Instance>");
  HTMLLiteralTextIndented(html, 16, "...");
  HTMLLiteralTextIndented(html, 12, "</Instance>");
  HTMLLiteralTextIndented(html, 8, "</Instances>");
  HTMLLiteralTextIndented(html, 4, "</HighSchoolTimetableArchive>");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The optional");
  HTMLTextMono(html, "MetaData");
  HTMLText(html, "category holds information describing the provenance");
  HTMLText(html, "of the archive.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "MetaData");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Contributor");
  HTMLLiteralTextIndented(html,  8, "Date");
  HTMLLiteralTextIndented(html,  8, "Description");
  HTMLLiteralTextIndented(html,  8, "+Remarks");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The child categories each hold text only.  They give");
  HTMLText(html, "the name of the archive, the name of the person who");
  HTMLText(html, "assembled it, the date it was assembled, a description");
  HTMLText(html, "which will be printed when displaying the archive, and");
  HTMLText(html, "some optional remarks.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void InstanceSpec(HTML html, char *name)                                 */
/*                                                                           */
/*  Print a specification of an instance.                                    */
/*                                                                           */
/*****************************************************************************/

static void InstanceSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Instances");
  HTMLParagraphBegin(html);
  HTMLText(html, "An");
  HTMLTextItalic(html, "instance");
  HTMLText(html, "is one occurrence of the high school timetable");
  HTMLText(html, "problem, for a particular school in a particular year (or");
  HTMLText(html, "semester, etc.).  The syntax of instances is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Instance Id");
  HTMLLiteralTextIndented(html,  8, "MetaData");
  HTMLLiteralTextIndented(html,  8, "Times");
  HTMLLiteralTextIndented(html,  8, "Resources");
  HTMLLiteralTextIndented(html,  8, "Events");
  HTMLLiteralTextIndented(html,  8, "Constraints");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  /* ***
  HTMLParagraphBegin(html);
  HTMLText(html, "The placement of");
  HTMLTextMono(html, "Id");
  HTMLText(html, "on the same line as");
  HTMLTextMono(html, "Instance");
  HTMLText(html, "indicates that");
  HTMLTextMono(html, "Id");
  HTMLText(html, "is an attribute of");
  HTMLTextMonoNoBreak(html, "Instance");
  HTMLText(html, ", not a child category.");
  HTMLParagraphEnd(html);
  *** */

  HTMLParagraphBegin(html);
  HTMLText(html, "Many categories have an");
  HTMLTextMono(html, "Id");
  HTMLText(html, "attribute.  Wherever one appears, its value is always");
  HTMLText(html, "a string, called an");
  HTMLTextItalic(html, "identifier");
  HTMLText(html, "and used to refer to its category from elsewhere within");
  HTMLText(html, "the file.  For more information about identifiers,");
  HTMLText(html, "consult the");
  HTMLJump2(html, "op", "spec", "part", "glossary", "id",
    "Identifier glossary entry");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  /* ***
  HTMLParagraphBegin(html);
  HTMLText(html, "The");
  HTMLTextMono(html, "MetaData");
  HTMLText(html, "category holds information describing the provenance");
  HTMLText(html, "of the instance.  The absence of a preceding");
  HTMLTextMono(html, "+");
  HTMLText(html, "or");
  HTMLTextMono(html, "*");
  HTMLText(html, "indicates that a single occurrence of this category");
  HTMLText(html, "is compulsory.  Its syntax is");
  HTMLParagraphEnd(html);
  *** */

  HTMLParagraphBegin(html);
  HTMLText(html, "The");
  HTMLTextMono(html, "MetaData");
  HTMLText(html, "category holds information describing the provenance");
  HTMLText(html, "of the instance.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "MetaData");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Contributor");
  HTMLLiteralTextIndented(html,  8, "Date");
  HTMLLiteralTextIndented(html,  8, "Country");
  HTMLLiteralTextIndented(html,  8, "Description");
  HTMLLiteralTextIndented(html,  8, "+Remarks");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The child categories each hold text only.  They give the");
  HTMLText(html, "name of the instance (which should identify the school");
  HTMLText(html, "and the year or semester), the name of the person who");
  HTMLText(html, "contributed it, the date it was contributed, the");
  HTMLText(html, "country in which its school is located, a description");
  HTMLText(html, "which will be printed when displaying the instance, and");
  HTMLText(html, "some optional remarks.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void TimeGroupSpec(HTML html, char *name)                                */
/*                                                                           */
/*  Print a spec of the Times part of an instance onto html.                 */
/*                                                                           */
/*****************************************************************************/

static void TimeGroupSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Times");
  HTMLParagraphBegin(html);
  HTMLText(html, "The format supports only a simple model of time, in which");
  HTMLText(html, "time is broken into intervals called times");
  HTMLText(html, "which are assumed to not overlap.  The");
  HTMLTextMono(html, "Times");
  HTMLText(html, "child category of");
  HTMLTextMono(html, "Instance");
  HTMLText(html, "defines these times.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Times");
  HTMLLiteralTextIndented(html,  8, "+TimeGroups");
  HTMLLiteralTextIndented(html,  8, "*Time");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The optional");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "category declares");
  HTMLTextItalicNoBreak(html, "time groups");
  HTMLText(html, ", which are sets of times.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "*Week");
  HTMLLiteralTextIndented(html,  8, "*Day");
  HTMLLiteralTextIndented(html,  8, "*TimeGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of");
  HTMLTextMono(html, "TimeGroup");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroup Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "It declares a time group with the given");
  HTMLTextMono(html, "Id");
  HTMLText(html, "and");
  HTMLTextMonoNoBreak(html, "Name");
  HTMLText(html, ".  For example,");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "<TimeGroup Id=\"BeforeLunch\">");
  HTMLLiteralTextIndented(html,  8, "<Name>BeforeLunch</Name>");
  HTMLLiteralTextIndented(html,  4, "</TimeGroup>");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "declares a time group which presumably will contain those");
  HTMLText(html, "times which are just before lunch.  A time group's times");
  HTMLText(html, "are added to it later, as they are declared (see below).");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Many categories contain");
  HTMLTextMono(html, "Id");
  HTMLText(html, "and");
  HTMLTextMono(html, "Name");
  HTMLText(html, "elements.  In all cases,");
  HTMLTextMono(html, "Id");
  HTMLText(html, "is an attribute whose value is a string used to refer");
  HTMLText(html, "to the category from elsewhere in the file, while");
  HTMLTextMono(html, "Name");
  HTMLText(html, "is a child category");
  HTMLText(html, "containing just text, which names the category and");
  HTMLText(html, "is used when displaying it.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Categories");
  HTMLTextMono(html, "Week");
  HTMLText(html, "and");
  HTMLTextMono(html, "Day");
  HTMLText(html, "have the same syntax as");
  HTMLTextMono(html, "TimeGroup");
  HTMLText(html, "and also define time groups.  When present, they allow");
  HTMLText(html, "display software to determine how the times of an instance");
  HTMLText(html, "are grouped into weeks and days.  They are optional, and");
  HTMLText(html, "solvers should not rely on their presence.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The");
  HTMLTextMono(html, "Time");
  HTMLText(html, "child categories of");
  HTMLTextMono(html, "Times");
  HTMLText(html, "define the times of the instance.  Their order is");
  HTMLText(html, "significant:  it reflects their ordering in time.");
  HTMLText(html, "An interval of real time has a starting moment and a");
  HTMLText(html, "duration, but these properties are not part of the time");
  HTMLText(html, "model used here.  The syntax of times is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Time Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "+Week");
  HTMLLiteralTextIndented(html,  8, "+Day");
  HTMLLiteralTextIndented(html,  8, "+TimeGroups");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where the syntax of");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "*TimeGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and the syntax of");
  HTMLTextMono(html, "TimeGroup");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextMono(html, "TimeGroup");
  HTMLText(html, "contains a");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "attribute only (no children or text).  The value of");
  HTMLText(html, "this attribute must equal the");
  HTMLTextMono(html, "Id");
  HTMLText(html, "of a time group declared earlier; the meaning is that");
  HTMLText(html, "the enclosing time is a member of that time group.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of the");
  HTMLTextMono(html, "Week");
  HTMLText(html, "and");
  HTMLTextMono(html, "Day");
  HTMLText(html, "child categories is the same as the syntax of");
  HTMLTextMonoNoBreak(html, "TimeGroup");
  HTMLText(html, ".  Their");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "attributes must contain the Ids of");
  HTMLTextMono(html, "Week");
  HTMLText(html, "and");
  HTMLTextMono(html, "Day");
  HTMLText(html, "time groups declared earlier.  Overall, the syntax");
  HTMLText(html, "allows each time to lie in one week, one day, and any");
  HTMLText(html, "number of other time groups.  For example,");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "<Time Id=\"Mon4\">");
  HTMLLiteralTextIndented(html,  8, "<Name>Mon4</Name>");
  HTMLLiteralTextIndented(html,  8, "<Day Reference=\"Monday\"/>");
  HTMLLiteralTextIndented(html,  8, "<TimeGroups>");
  HTMLLiteralTextIndented(html, 12, "<TimeGroup Reference=\"BeforeLunch\"/>");
  HTMLLiteralTextIndented(html,  8, "</TimeGroups>");
  HTMLLiteralTextIndented(html,  4, "</Time>");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "defines one time lying in the");
  HTMLTextMono(html, "Monday");
  HTMLText(html, "and");
  HTMLTextMono(html, "BeforeLunch");
  HTMLText(html, "time groups.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ResourceGroupSpec(HTML html, char *name)                            */
/*                                                                           */
/*  Print the specification of resources onto html.                          */
/*                                                                           */
/*****************************************************************************/

static void ResourceGroupSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Resources");
  HTMLParagraphBegin(html);
  HTMLTextItalic(html, "Resources");
  HTMLText(html, "are entities which attend events.  They are typically");
  HTMLText(html, "teachers, rooms, and student groups or individual");
  HTMLText(html, "students, but any types of resources may be used.  The");
  HTMLTextMono(html, "Resources");
  HTMLText(html, "child category of");
  HTMLTextMono(html, "Instance");
  HTMLText(html, "defines these resources.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resources");
  HTMLLiteralTextIndented(html,  8, "+ResourceTypes");
  HTMLLiteralTextIndented(html,  8, "+ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "*Resource");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of");
  HTMLTextMono(html, "ResourceTypes");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceTypes");
  HTMLLiteralTextIndented(html,  8, "*ResourceType");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where each");
  HTMLTextMono(html, "ResourceType");
  HTMLText(html, "category defines one");
  HTMLTextItalicNoBreak(html, "resource type");
  HTMLText(html, ", and has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceType Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "For example,");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "<ResourceTypes>");
  HTMLLiteralTextIndented(html,  8, "<ResourceType Id=\"Teacher\">");
  HTMLLiteralTextIndented(html, 12, "<Name>Teacher</Name>");
  HTMLLiteralTextIndented(html,  8, "</ResourceType>");
  HTMLLiteralTextIndented(html,  8, "<ResourceType Id=\"Room\">");
  HTMLLiteralTextIndented(html, 12, "<Name>Room</Name>");
  HTMLLiteralTextIndented(html,  8, "</ResourceType>");
  HTMLLiteralTextIndented(html,  8, "<ResourceType Id=\"Class\">");
  HTMLLiteralTextIndented(html, 12, "<Name>Class</Name>");
  HTMLLiteralTextIndented(html,  8, "</ResourceType>");
  HTMLLiteralTextIndented(html,  4, "</ResourceTypes>");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "defines three typical resource types.  Although any");
  HTMLText(html, "identifiers and names may be used, it is recommended that");
  HTMLTextMonoNoBreak(html, "Teacher");
  HTMLText(html, ",");
  HTMLTextMonoNoBreak(html, "Room");
  HTMLText(html, ",");
  HTMLTextMonoNoBreak(html, "Class");
  HTMLText(html, ", and");
  HTMLTextMono(html, "Student");
  HTMLText(html, "be used whenever appropriate.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The optional");
  HTMLTextMono(html, "ResourceGroups");
  HTMLText(html, "child category of");
  HTMLTextMono(html, "Resources");
  HTMLText(html, "defines");
  HTMLTextItalicNoBreak(html, "resource groups");
  HTMLText(html, ", which are sets of resources of the same type.  Its");
  HTMLText(html, "syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "*ResourceGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where the syntax of");
  HTMLTextMono(html, "ResourceGroup");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceGroup Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "ResourceType");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "This contains the usual");
  HTMLTextMono(html, "Id");
  HTMLText(html, "and");
  HTMLTextMono(html, "Name");
  HTMLText(html, "elements, plus a compulsory");
  HTMLTextMono(html, "ResourceType");
  HTMLText(html, "child category whose syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceType Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where the value of");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "is the identifier of a");
  HTMLTextMono(html, "ResourceType");
  HTMLText(html, "category, and indicates that the elements of the");
  HTMLText(html, "resource group must all have that type.  The resources");
  HTMLText(html, "themselves are added to the resource group later, as");
  HTMLText(html, "they are defined.  For example,");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "<ResourceGroup Id=\"ScienceLab\">");
  HTMLLiteralTextIndented(html,  8, "<Name>ScienceLab</Name>");
  HTMLLiteralTextIndented(html,  8, "<ResourceType Reference=\"Room\"/>");
  HTMLLiteralTextIndented(html,  4, "</ResourceGroup>");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "defines a");
  HTMLTextMono(html, "ScienceLab");
  HTMLText(html, "resource group whose elements are rooms.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each");
  HTMLTextMono(html, "Resource");
  HTMLText(html, "child category of");
  HTMLTextMono(html, "Resources");
  HTMLText(html, "defines one");
  HTMLTextItalicNoBreak(html, "resource");
  HTMLText(html, ", an entity that attends events.  (Times are not");
  HTMLText(html, "considered to be resources.)  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resource Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "ResourceType");
  HTMLLiteralTextIndented(html,  8, "+ResourceGroups");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where the syntax of");
  HTMLTextMono(html, "ResourceGroups");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "*ResourceGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and the syntax of");
  HTMLTextMono(html, "ResourceGroup");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Altogether, this defines a resource with the given");
  HTMLText(html, "identifier, name, and type, and specifies which resource");
  HTMLText(html, "groups the resource lies in.  For example,");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "<Resource Id=\"r03\">");
  HTMLLiteralTextIndented(html,  8, "<Name>r03</Name>");
  HTMLLiteralTextIndented(html,  8, "<ResourceType Reference=\"Room\"/>");
  HTMLLiteralTextIndented(html,  8, "<ResourceGroups>");
  HTMLLiteralTextIndented(html, 12, "<ResourceGroup Reference=\"LargeRoom\"/>");
  HTMLLiteralTextIndented(html, 12, "<ResourceGroup Reference=\"ScienceLab\"/>");
  HTMLLiteralTextIndented(html,  8, "</ResourceGroups>");
  HTMLLiteralTextIndented(html,  4, "</Resource>");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "defines a resource of type");
  HTMLTextMono(html, "Room");
  HTMLText(html, "lying in resource groups");
  HTMLTextMono(html, "LargeRoom");
  HTMLText(html, "and");
  HTMLTextMonoNoBreak(html, "ScienceLab");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void EventGroupSpec(HTML html, char *name)                               */
/*                                                                           */
/*  Print a specification of events onto html.                               */
/*                                                                           */
/*****************************************************************************/

static void EventGroupSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Events");

  HTMLParagraphBegin(html);
  HTMLText(html, "An");
  HTMLTextItalic(html, "event");
  HTMLText(html, "(also called an");
  HTMLTextItalicNoBreak(html, "instance event");
  HTMLText(html, ") is a meeting between resources.  It specifies that the");
  HTMLText(html, "resources should meet together for a given number of times.");
  HTMLText(html, "This number is an integer called the");
  HTMLTextItalic(html, "duration");
  HTMLText(html, "of the event.  The actual times and resources may be");
  HTMLText(html, "preassigned to the event, or they may be left open for");
  HTMLText(html, "a solver to assign, subject to constraints.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Events lie within the");
  HTMLTextMono(html, "Events");
  HTMLText(html, "child category of");
  HTMLTextMonoNoBreak(html, "Instance");
  HTMLText(html, ", whose syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Events");
  HTMLLiteralTextIndented(html,  8, "+EventGroups");
  HTMLLiteralTextIndented(html,  8, "*Event");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Similarly to the");
  HTMLTextMono(html, "Times");
  HTMLText(html, "and");
  HTMLTextMono(html, "Resources");
  HTMLText(html, "categories, this syntax first allows");
  HTMLTextItalic(html, "event groups");
  HTMLText(html, "(sets of events) to be defined, followed by the");
  HTMLText(html, "events themselves.  The syntax of");
  HTMLTextMono(html, "EventGroups");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventGroups");
  HTMLLiteralTextIndented(html,  8, "*Course");
  HTMLLiteralTextIndented(html,  8, "*EventGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of");
  HTMLTextMono(html, "EventGroup");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventGroup Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "This declares an event group with the given identifier");
  HTMLText(html, "and name.  As with other groups, the events themselves");
  HTMLText(html, "are added to the event group later, as they are declared.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "In the same way as");
  HTMLTextMono(html, "Week");
  HTMLText(html, "and");
  HTMLTextMono(html, "Day");
  HTMLText(html, "are alternative forms for");
  HTMLTextMonoNoBreak(html, "TimeGroup");
  HTMLText(html, ",");
  HTMLTextMono(html, "Course");
  HTMLText(html, "is an alternative form for");
  HTMLTextMonoNoBreak(html, "EventGroup");
  HTMLText(html, ".  Presumably, events declared to lie in the same");
  HTMLTextMono(html, "Course");
  HTMLText(html, "constitute a course of study in one subject for one");
  HTMLText(html, "group of students.  Courses are optional, however, and");
  HTMLText(html, "solvers should not rely on their presence.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of");
  HTMLTextMono(html, "Event");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Event Id +Color");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Duration");
  HTMLLiteralTextIndented(html,  8, "+Workload");
  HTMLLiteralTextIndented(html,  8, "+Course");
  HTMLLiteralTextIndented(html,  8, "+Time");
  HTMLLiteralTextIndented(html,  8, "+Resources");
  HTMLLiteralTextIndented(html,  8, "+ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "+EventGroups");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The");
  HTMLTextMono(html, "Id");
  HTMLText(html, "and");
  HTMLTextMono(html, "Name");
  HTMLText(html, "elements assign an identifier and name to the event in the");
  HTMLText(html, "usual way.  The optional");
  HTMLTextMono(html, "Color");
  HTMLText(html, "attribute specifies a colour to be used in the background");
  HTMLText(html, "of the event in printed timetables.  Its value is as");
  HTMLText(html, "usual for Web colours, i.e. a colour name or");
  HTMLTextMonoNoBreak(html, "#RRGGBB");
  HTMLText(html, ".  The");
  HTMLTextMono(html, "Duration");
  HTMLText(html, "child category defines the duration of the event.  It");
  HTMLText(html, "has no attributes or child categories, and its body must");
  HTMLText(html, "consist entirely of an integer whose value is at least 1.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The optional");
  HTMLTextMono(html, "Workload");
  HTMLText(html, "child category defines a workload for the event.  If");
  HTMLText(html, "absent, the workload defaults to the duration.  If present,");
  HTMLTextMono(html, "Workload");
  HTMLText(html, "has no attributes or child categories, and its body");
  HTMLText(html, "consists entirely of an integer whose value is at least 0.");
  HTMLText(html, "A full explanation of workloads appears in the");
  HTMLText(html, "specification of the limit workload constraint.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The optional");
  HTMLTextMono(html, "Course");
  HTMLText(html, "child category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Course Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and specifies that the event lies in the referenced course.");
  HTMLText(html, "Similarly, the optional");
  HTMLTextMono(html, "EventGroups");
  HTMLText(html, "child category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventGroups");
  HTMLLiteralTextIndented(html,  8, "*EventGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "EventGroup");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and specifies that the event lies in the referenced");
  HTMLText(html, "event groups.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The optional");
  HTMLTextMono(html, "Time");
  HTMLText(html, "child category is used only when the event has a");
  HTMLText(html, "preassigned time.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Time Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and it specifies that the starting time of the event");
  HTMLText(html, "is preassigned the referenced time.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Finally, the optional");
  HTMLTextMono(html, "Resources");
  HTMLText(html, "and");
  HTMLTextMono(html, "ResourceGroups");
  HTMLText(html, "child categories specify the resources that are to");
  HTMLText(html, "attend the event.  The syntax of");
  HTMLTextMono(html, "Resources");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resources");
  HTMLLiteralTextIndented(html,  8, "*Resource");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each");
  HTMLTextMono(html, "Resource");
  HTMLText(html, "child category specifies a single resource that is");
  HTMLText(html, "to attend the event.  A");
  HTMLTextMono(html, "Resource");
  HTMLText(html, "child category is often referred to informally as a");
  HTMLText(html, "resource, but in this documentation it is always called an");
  HTMLTextItalicNoBreak(html, "event resource");
  HTMLText(html, ", to distinguish it from a resource such as a teacher");
  HTMLText(html, "or room.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resource +Reference");
  HTMLLiteralTextIndented(html,  8, "+Role");
  HTMLLiteralTextIndented(html,  8, "+ResourceType");
  HTMLLiteralTextIndented(html,  8, "+Workload");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its meaning is determined by whether the");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "attribute is present or not.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "When the");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "attribute is absent, the solver is expected to assign a");
  HTMLText(html, "resource to the event resource, and both the");
  HTMLTextMono(html, "Role");
  HTMLText(html, "and");
  HTMLTextMono(html, "ResourceType");
  HTMLText(html, "child categories are compulsory.");
  HTMLTextMono(html, "Role");
  HTMLText(html, "has no attributes and no child categories; its body is");
  HTMLText(html, "used similarly to an identifier, to identify the event");
  HTMLText(html, "resource within the event.  Two event resources of one");
  HTMLText(html, "event may not have the same value for");
  HTMLTextMonoNoBreak(html, "Role");
  HTMLText(html, ", but it is normal for event resources within different");
  HTMLText(html, "events to have the same value for");
  HTMLTextMonoNoBreak(html, "Role");
  HTMLText(html, ".  The");
  HTMLTextMono(html, "ResourceType");
  HTMLText(html, "child category determines the type of resource that must");
  HTMLText(html, "be assigned here.  A solution that assigns a resource of");
  HTMLText(html, "some other type is illegal and will be rejected.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "When the");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "attribute is present, it references a resource which is to");
  HTMLText(html, "be preassigned to the event.  In that case, the");
  HTMLTextMono(html, "Role");
  HTMLText(html, "and");
  HTMLTextMono(html, "ResourceType");
  HTMLText(html, "child categories may be omitted; if present, they have");
  HTMLText(html, "the meaning just given, and the type of the preassigned");
  HTMLText(html, "resource must equal the resource type.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "Workload");
  HTMLText(html, "child category is present, it defines a workload for the");
  HTMLText(html, "event resource.  If absent, the workload defaults to the");
  HTMLText(html, "workload of the enclosing event.  If present,");
  HTMLTextMono(html, "Workload");
  HTMLText(html, "has no attributes or child categories, and its body");
  HTMLText(html, "consists entirely of an integer whose value is at least 0.");
  HTMLText(html, "A full explanation of workloads appears in the");
  HTMLText(html, "specification of the limit workload constraint.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of");
  HTMLTextMono(html, "ResourceGroups");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "*ResourceGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and the syntax of");
  HTMLTextMono(html, "ResourceGroup");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "It references a resource group.  At present, the meaning");
  HTMLText(html, "is that every resource in the resource group is to be");
  HTMLText(html, "preassigned to the event, with workload equal to the");
  HTMLText(html, "workload of the event.  However, the original plan was");
  HTMLText(html, "to use this category for student sectioning, so there");
  HTMLText(html, "may be a change to the specification here in the future.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintSpec(HTML html, char *name)                               */
/*                                                                           */
/*  Print a brief specification of constraints.                              */
/*                                                                           */
/*****************************************************************************/

static void ConstraintSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "The");
  HTMLTextMono(html, "Constraints");
  HTMLText(html, "child category of");
  HTMLTextMono(html, "Instance");
  HTMLText(html, "lists the constraints that a solution to the instance");
  HTMLText(html, "should obey.  For full details,");
  HTMLJump2(html, "op", "spec", "part", "constraints", NULL, "click here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}



/*****************************************************************************/
/*                                                                           */
/*  void ConstraintGeneralSpec(HTML html, char *name)                        */
/*                                                                           */
/*  Print a specification of constraints generally.                          */
/*                                                                           */
/*****************************************************************************/

static void ConstraintGeneralSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Constraints in general");

  HTMLParagraphBegin(html);
  HTMLText(html, "A");
  HTMLTextItalic(html, "constraint");
  HTMLText(html, "is a condition that solutions should satisfy, if possible.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each constraint has a set of");
  HTMLTextItalic(html, "points of application");
  HTMLText(html, "which are instance entities (such as resources or events).");
  HTMLText(html, "Recently, the specification was changed to require each");
  HTMLText(html, "constraint to have at least one point of application.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Given a solution, a non-negative integer");
  HTMLTextItalic(html, "cost");
  HTMLText(html, "is associated with each point of application.  Each cost");
  HTMLText(html, "is calculated independently of the others, according to a");
  HTMLText(html, "formula defined on this page.  A non-zero cost for some");
  HTMLText(html, "point of application indicates that the solution");
  HTMLTextItalic(html, "violates");
  HTMLText(html, "the constraint (fails to satisfy its condition) at");
  HTMLText(html, "that point.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Many");
  HTMLTextItalic(html, "types");
  HTMLText(html, "of constraints are defined, and more may be added in the");
  HTMLText(html, "future.  Constraints of all types appear within the");
  HTMLTextMono(html, "Constraints");
  HTMLText(html, "child category of the");
  HTMLTextMono(html, "Instance");
  HTMLText(html, "category.  Their syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Constraints");
  HTMLLiteralTextIndented(html,  8, "*AssignResourceConstraint");
  HTMLLiteralTextIndented(html,  8, "*AssignTimeConstraint");
  HTMLLiteralTextIndented(html,  8, "*SplitEventsConstraint");
  HTMLLiteralTextIndented(html,  8, "*DistributeSplitEventsConstraint");
  HTMLLiteralTextIndented(html,  8, "*PreferResourcesConstraint");
  HTMLLiteralTextIndented(html,  8, "*PreferTimesConstraint");
  HTMLLiteralTextIndented(html,  8, "*AvoidSplitAssignmentsConstraint");
  HTMLLiteralTextIndented(html,  8, "*SpreadEventsConstraint");
  HTMLLiteralTextIndented(html,  8, "*LinkEventsConstraint");
  HTMLLiteralTextIndented(html,  8, "*OrderEventsConstraint");
  HTMLLiteralTextIndented(html,  8, "*AvoidClashesConstraint");
  HTMLLiteralTextIndented(html,  8, "*AvoidUnavailableTimesConstraint");
  HTMLLiteralTextIndented(html,  8, "*LimitIdleTimesConstraint");
  HTMLLiteralTextIndented(html,  8, "*ClusterBusyTimesConstraint");
  HTMLLiteralTextIndented(html,  8, "*LimitBusyTimesConstraint");
  HTMLLiteralTextIndented(html,  8, "*LimitWorkloadConstraint");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "For each constraint type there is a section of this page");
  HTMLText(html, "which describes the specifics of that constraint type.");
  HTMLText(html, "There are also several properties common to all constraint");
  HTMLText(html, "types, and those are described in the remainder of this");
  HTMLText(html, "section.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "In general, a constraint has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AnyConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "...");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "AnyConstraint");
  HTMLText(html, "stands for any one of the child categories of");
  HTMLTextMono(html, "Constraints");
  HTMLText(html, "listed above, and");
  HTMLTextMono(html, "...");
  HTMLText(html, "stands for additional child categories which vary with");
  HTMLText(html, "the constraint type.  As usual, the");
  HTMLTextMono(html, "Id");
  HTMLText(html, "attribute is used to reference the constraint from");
  HTMLText(html, "elsewhere in the file, while the");
  HTMLTextMono(html, "Name");
  HTMLText(html, "child category is used when printing the constraint");
  HTMLText(html, "in human-readable form.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The evaluation of one constraint at one point of");
  HTMLText(html, "application (that is, the determination of a single");
  HTMLText(html, "cost) proceeds in two stages.  In the first stage, a");
  HTMLText(html, "non-negative integer");
  HTMLTextItalic(html, "deviation");
  HTMLText(html, "is calculated.  How this is done depends on the");
  HTMLText(html, "constraint type.");
  HTMLParagraphEnd(html);

  /* ***
  HTMLParagraphBegin(html);
  HTMLText(html, "The evaluation of one constraint at one point of");
  HTMLText(html, "application (that is, the determination of a single");
  HTMLText(html, "cost) proceeds in two stages.  In the first stage, a");
  HTMLText(html, "set of non-negative integer");
  HTMLTextItalic(html, "deviations");
  HTMLText(html, "is calculated.  How this is done depends on the");
  HTMLText(html, "constraint type.");
  HTMLParagraphEnd(html);
  *** */

  HTMLParagraphBegin(html);
  HTMLText(html, "The second stage is common to all constraint types.");
  HTMLText(html, "It is influenced by the");
  HTMLTextMonoNoBreak(html, "Required");
  HTMLText(html, ",");
  HTMLTextMonoNoBreak(html, "Weight");
  HTMLText(html, ", and");
  HTMLTextMono(html, "CostFunction");
  HTMLText(html, "child categories.  All three have no attributes and no");
  HTMLText(html, "child categories, merely a body.  The body of");
  HTMLTextMono(html, "Required");
  HTMLText(html, "must be either");
  HTMLTextMono(html, "true");
  HTMLText(html, "or");
  HTMLTextMonoNoBreak(html, "false");
  HTMLText(html, ".  The body of");
  HTMLTextMono(html, "Weight");
  HTMLText(html, "must be an integer in the range 0 to 1000 inclusive.");
  HTMLText(html, "The body of");
  HTMLTextMono(html, "CostFunction");
  HTMLText(html, "must be one of the three values in the table below.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The cost is calculated by the following formula:");
  HTMLParagraphEnd(html);

  HTMLCentredDisplayBegin(html);
  HTMLTextItalic(html, "Cost = Weight * CostFunction(deviation)");
  /* HTMLTextItalic(html, "Cost = Weight * CostFunction(deviations)"); */
  HTMLCentredDisplayEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "That is, the cost function is applied to the deviation,");
  HTMLText(html, "producing an integer which is multiplied by the weight to");
  HTMLText(html, "obtain the cost.  The permitted values for");
  HTMLTextMono(html, "CostFunction");
  HTMLText(html, "are shown in this table:");
  HTMLParagraphEnd(html);

  /* ***
  HTMLParagraphBegin(html);
  HTMLText(html, "That is, the cost function is applied to the deviations,");
  HTMLText(html, "producing an integer which is multiplied by the weight to");
  HTMLText(html, "obtain the cost.  The permitted values for");
  HTMLTextMono(html, "CostFunction");
  HTMLText(html, "are currently in process of being changed.  The following");
  HTMLText(html, "table shows both the old and new values.  The version of");
  HTMLText(html, "HSEval which generated this page accepts both sets of");
  HTMLText(html, "values, but soon, after new versions of the archives appear");
  HTMLText(html, "on the XHSTT web site, the old values will be withdrawn.");
  HTMLParagraphEnd(html);
  *** */

  HTMLParagraphBegin(html);
  HTMLTableBeginAttributed(html, 2, 2, 2, LightGreen);

  HTMLTableRowVAlignBegin(html, "top");
  HTMLTableEntryTextItalic(html, "CostFunction");
  HTMLTableEntryTextItalic(html, "Meaning");
  HTMLTableRowEnd(html);

  HTMLTableRowVAlignBegin(html, "top");
  HTMLTableEntryBegin(html);
  HTMLTextMono(html, "Linear");
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLText(html, "The deviation, unchanged");
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);

  HTMLTableRowVAlignBegin(html, "top");
  HTMLTableEntryBegin(html);
  HTMLTextMono(html, "Quadratic");
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLText(html, "The square of the deviation");
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);

  HTMLTableRowVAlignBegin(html, "top");
  HTMLTableEntryBegin(html);
  HTMLTextMono(html, "Step");
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLText(html, "1 if the deviation is non-zero, and 0 otherwise");
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);

  /* ***
  HTMLTableRowVAlignBegin(html, "top");
  HTMLTableEntryTextItalic(html, "Old value");
  HTMLTableEntryTextItalic(html, "New value");
  HTMLTableEntryTextItalic(html, "Meaning");
  HTMLTableRowEnd(html);

  HTMLTableRowVAlignBegin(html, "top");
  HTMLTableEntryBegin(html);
  HTMLTextMono(html, "Sum");
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLTextMono(html, "Linear");
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLText(html, "Sum the deviations");
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);

  HTMLTableRowVAlignBegin(html, "top");
  HTMLTableEntryBegin(html);
  HTMLTextMono(html, "SumSquare");
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLTextMono(html, "-");
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLText(html, "Sum the squares of the deviations");
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);

  HTMLTableRowVAlignBegin(html, "top");
  HTMLTableEntryBegin(html);
  HTMLTextMono(html, "SquareSum");
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLTextMono(html, "Quadratic");
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLText(html, "Square the sum of the deviations");
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);

  HTMLTableRowVAlignBegin(html, "top");
  HTMLTableEntryBegin(html);
  HTMLTextMono(html, "SumStep");
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLTextMono(html, "-");
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLText(html, "The number of positive deviations, irrespective of");
  HTMLText(html, "their value");
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);

  HTMLTableRowVAlignBegin(html, "top");
  HTMLTableEntryBegin(html);
  HTMLTextMono(html, "StepSum");
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLTextMono(html, "Step");
  HTMLTableEntryEnd(html);
  HTMLTableEntryBegin(html);
  HTMLText(html, "1 if there is at least one positive deviation, otherwise 0");
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);
  *** */

  HTMLTableEnd(html);
  HTMLParagraphEnd(html);

  /* ***
  HTMLParagraphBegin(html);
  HTMLText(html, "Most constraint types produce only one deviation, and in");
  HTMLText(html, "those cases");
  HTMLTextMono(html, "Sum");
  HTMLText(html, "is the identity function,");
  HTMLTextMono(html, "SumSquare");
  HTMLText(html, "and");
  HTMLTextMono(html, "SquareSum");
  HTMLText(html, "are identical, and");
  HTMLTextMono(html, "SumStep");
  HTMLText(html, "and");
  HTMLTextMono(html, "StepSum");
  HTMLText(html, "are identical.  In all cases, the cost of an empty set");
  HTMLText(html, "of deviations is 0, and adding a deviation with value 0");
  HTMLText(html, "to any set of deviations does not change the cost.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "When the old cost functions are withdrawn, those constraint");
  HTMLText(html, "types whose specification defines a set of deviations will");
  HTMLText(html, "be changed to define a single deviation whose value is the");
  HTMLText(html, "sum of the present set of deviations.  This will allow the");
  HTMLText(html, "definitions of the three cost functions above to be");
  HTMLText(html, "simplified, without changing the cost of any solution.");
  HTMLParagraphEnd(html);
  *** */

  /* ***
  HTMLParagraphBegin(html);
  HTMLText(html, "These values have changed recently.  The table shows");
  HTMLText(html, "only the new values.  The old values allowed a set of");
  HTMLText(html, "deviations to be defined at each point of application,");
  HTMLText(html, "whereas now the deviation at each point of application");
  HTMLText(html, "is a single integer, as stated above.");
  HTMLParagraphEnd(html);
  *** */

  HTMLParagraphBegin(html);
  HTMLText(html, "If");
  HTMLTextMono(html, "Required");
  HTMLText(html, "is");
  HTMLTextMonoNoBreak(html, "true");
  HTMLText(html, ", the constraint is a hard constraint and the cost is");
  HTMLText(html, "added to the infeasibility value of the solution.  If");
  HTMLTextMono(html, "Required");
  HTMLText(html, "is");
  HTMLTextMonoNoBreak(html, "false");
  HTMLText(html, ", the constraint is a soft constraint and the cost is");
  HTMLText(html, "added to the objective value of the solution.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Instances should be encoded on the understanding that");
  HTMLText(html, "violations of hard constraints are serious defects, and");
  HTMLText(html, "that solvers aim to find solutions with very few hard");
  HTMLText(html, "constraint violations.  Although the existence of such");
  HTMLText(html, "solutions is not guaranteed, realistic instances should");
  HTMLText(html, "have them.  On the other hand, violations of soft");
  HTMLText(html, "constraints are normal and to be expected.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintAssignResourceSpec(HTML html, char *name)                 */
/*                                                                           */
/*  Print a specification of assign resource constraints onto html.          */
/*                                                                           */
/*****************************************************************************/

static void ConstraintAssignResourceSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Assign resource constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "An");
  HTMLTextItalic(html, "assign resource constraint");
  HTMLText(html, "specifies that solution resources should be assigned");
  HTMLText(html, "resources.  More importantly, it defines the cost of");
  HTMLText(html, "failing to make these assignments.  It has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AssignResourceConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "Role");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+EventGroups");
  HTMLLiteralTextIndented(html,  8, "+Events");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of");
  HTMLTextMono(html, "EventGroups");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventGroups");
  HTMLLiteralTextIndented(html,  8, "*EventGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "EventGroup");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and references an event group.  The syntax of");
  HTMLTextMono(html, "Events");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Events");
  HTMLLiteralTextIndented(html,  8, "*Event");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "Event");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Event Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  /* ***
  HTMLParagraphBegin(html);
  HTMLText(html, "and references an event.  Together, these two child");
  HTMLText(html, "categories enumerate a set of events, namely the members");
  HTMLText(html, "of the event groups referenced plus the individual events");
  HTMLText(html, "referenced.  It is not an error for an event to occur");
  HTMLText(html, "more than once in this enumeration, but such events are");
  HTMLText(html, "treated as having occurred only once.  The compulsory");
  HTMLTextMono(html, "Role");
  HTMLText(html, "child category has no attributes and no child categories,");
  HTMLText(html, "only a body.");
  HTMLParagraphEnd(html);
  *** */

  HTMLParagraphBegin(html);
  HTMLText(html, "and references an event.  Together, these two child");
  HTMLText(html, "categories enumerate a set of events, namely the members");
  HTMLText(html, "of the event groups referenced plus the individual events");
  HTMLText(html, "referenced.  Multiple occurrences of events cause");
  HTMLText(html, "constraint violations to be counted multiple times.");
  HTMLText(html, "The compulsory");
  HTMLTextMono(html, "Role");
  HTMLText(html, "child category has no attributes and no child categories,");
  HTMLText(html, "only a body.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "For each event listed in the");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "section that contains an unpreassigned event resource");
  HTMLText(html, "with the given");
  HTMLTextMono(html, "Role");
  HTMLText(html, "(there can be at most one such event resource in each");
  HTMLText(html, "event), that event resource is one point of application");
  HTMLText(html, "of this constraint.  It is not an error to include events");
  HTMLText(html, "in the");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "section that do not have an event resource with the given");
  HTMLTextMonoNoBreak(html, "Role");
  HTMLText(html, ", or that have a preassigned one, but such events are");
  HTMLText(html, "skipped by this constraint.  However, as mentioned above,");
  HTMLText(html, "there must be at least one point of application.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one event resource) is the sum of the");
  HTMLText(html, "durations of the solution events that contain unassigned");
  HTMLText(html, "solution resources derived from the event resource.");
  HTMLText(html, "This deviation is converted into a cost as described");
  HTMLText(html, "above for constraints in general.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "This constraint does not require an assigned resource");
  HTMLText(html, "to come from any particular set of resources (see the");
  HTMLText(html, "prefer resources constraint for that); it merely requires");
  HTMLText(html, "that");
  HTMLTextItalic(html, "some");
  HTMLText(html, "resource be assigned.  Durations influence the cost on");
  HTMLText(html, "the principle that the longer a problem persists, the");
  HTMLText(html, "worse it is.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextItalic(html, "Note.  Before Version 1.38, this specification");
  HTMLTextItalic(html, "incorrectly stated that multiple occurrences of");
  HTMLTextItalic(html, "events were ignored.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}

/*****************************************************************************/
/*                                                                           */
/*  void ConstraintAssignTimeSpec(HTML html, char *name)                     */
/*                                                                           */
/*  Print a specification of assign time constraints onto html.              */
/*                                                                           */
/*****************************************************************************/

static void ConstraintAssignTimeSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Assign time constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "An");
  HTMLTextItalic(html, "assign time constraint");
  HTMLText(html, "specifies that solution events should be assigned times.");
  HTMLText(html, "More importantly, it defines the cost of failing to make");
  HTMLText(html, "these assignments.  It has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AssignTimeConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+EventGroups");
  HTMLLiteralTextIndented(html,  8, "+Events");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and defines a non-empty set of events in the same way");
  HTMLText(html, "as for the assign resource constraint above.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each event listed in the");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "section that does not contain a time preassignment");
  HTMLText(html, "is one point of application of this constraint.  It");
  HTMLText(html, "is not an error to include events in the");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "section that contain a time preassignment, but such events");
  HTMLText(html, "are skipped by this constraint.  However, as mentioned");
  HTMLText(html, "above, there must be at least one point of application.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one instance event) is the total duration of");
  HTMLText(html, "those solution events derived from the instance event");
  HTMLText(html, "that are not assigned a time.  This deviation is converted");
  HTMLText(html, "into a cost in the usual way.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "This constraint does not require an assigned time");
  HTMLText(html, "to come from any particular set of times (see the");
  HTMLText(html, "prefer times constraint for that); it merely requires");
  HTMLText(html, "that");
  HTMLTextItalic(html, "some");
  HTMLText(html, "time be assigned.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintSplitEventsSpec(HTML html, char *name)                    */
/*                                                                           */
/*  Print a specification of split events constraints onto html.             */
/*                                                                           */
/*****************************************************************************/

static void ConstraintSplitEventsSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Split events constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "A");
  HTMLTextItalic(html, "split events constraint");
  HTMLText(html, "places limits on the number of solution events that may");
  HTMLText(html, "be derived from a given instance event, and on their");
  HTMLText(html, "durations.  It has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "SplitEventsConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "MinimumDuration");
  HTMLLiteralTextIndented(html,  8, "MaximumDuration");
  HTMLLiteralTextIndented(html,  8, "MinimumAmount");
  HTMLLiteralTextIndented(html,  8, "MaximumAmount");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+EventGroups");
  HTMLLiteralTextIndented(html,  8, "+Events");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and defines a non-empty set of events in the same way");
  HTMLText(html, "as for the assign resource constraint above.  These events");
  HTMLText(html, "are the points of application of this constraint.  The");
  HTMLTextMonoNoBreak(html, "MinimumDuration");
  HTMLText(html, ",");
  HTMLTextMonoNoBreak(html, "MaximumDuration");
  HTMLText(html, ",");
  HTMLTextMonoNoBreak(html, "MinimumAmount");
  HTMLText(html, ", and");
  HTMLTextMono(html, "MaximumAmount");
  HTMLText(html, "child categories have no attributes and no child");
  HTMLText(html, "categories, only a body, whose value must be an integer.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one instance event) is the number of solution");
  HTMLText(html, "events derived from the instance event whose duration is");
  HTMLText(html, "less than");
  HTMLTextMono(html, "MinimumDuration");
  HTMLText(html, "or greater than");
  HTMLTextMonoNoBreak(html, "MaximumDuration");
  HTMLText(html, ", plus the amount by which the number of solution events");
  HTMLText(html, "falls short of");
  HTMLTextMono(html, "MinimumAmount");
  HTMLText(html, "or exceeds");
  HTMLTextMonoNoBreak(html, "MaximumAmount");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Further control over the durations assigned to solution");
  HTMLText(html, "events is provided by distribute split events constraints.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintDistributeSplitEventsSpec(HTML html, char *name)          */
/*                                                                           */
/*  Print a specification of distribute split events constraints onto html.  */
/*                                                                           */
/*****************************************************************************/

static void ConstraintDistributeSplitEventsSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Distribute split events constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "A");
  HTMLTextItalic(html, "distribute split events constraint");
  HTMLText(html, "places limits on the number of solution events of a");
  HTMLText(html, "particular duration that may be derived from a given");
  HTMLText(html, "instance event.  It has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AssignTimeConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "Duration");
  HTMLLiteralTextIndented(html,  8, "Minimum");
  HTMLLiteralTextIndented(html,  8, "Maximum");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+EventGroups");
  HTMLLiteralTextIndented(html,  8, "+Events");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and defines a non-empty set of events in the same way as");
  HTMLText(html, "for the assign resource constraint above.  These events");
  HTMLText(html, "are the points of application of this constaint.  As usual,");
  HTMLText(html, "there must be at least one point of application.  The");
  HTMLTextMonoNoBreak(html, "Duration");
  HTMLText(html, ",");
  HTMLTextMonoNoBreak(html, "Minimum");
  HTMLText(html, ", and");
  HTMLTextMono(html, "Maximum");
  HTMLText(html, "child categories have no attributes and no child");
  HTMLText(html, "categories, only a body, whose value must be an integer.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one instance event) is the amount by which the");
  HTMLText(html, "number of solution events derived from that instance event");
  HTMLText(html, "whose duration is");
  HTMLTextMono(html, "Duration");
  HTMLText(html, "falls short of");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "or exceeds");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Further control over the durations of solution");
  HTMLText(html, "events is provided by split events constraints.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintPreferResourcesSpec(HTML html, char *name)                */
/*                                                                           */
/*  Print a specification of prefer resources constraints onto html.         */
/*                                                                           */
/*****************************************************************************/

static void ConstraintPreferResourcesSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Prefer resources constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "A");
  HTMLTextItalic(html, "prefer resources constraint");
  HTMLText(html, "specifies that some resources are preferred for assignment");
  HTMLText(html, "to some solution resources.  It has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "PreferResourcesConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "+Resources");
  HTMLLiteralTextIndented(html,  8, "Role");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+EventGroups");
  HTMLLiteralTextIndented(html,  8, "+Events");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and defines a non-empty set of events in the same way as");
  HTMLText(html, "for the assign resource constraint above.  The compulsory");
  HTMLTextMono(html, "Role");
  HTMLText(html, "child category has no attributes and no child categories,");
  HTMLText(html, "only a body.  Exactly as for the assign resource");
  HTMLText(html, "constraint, the points of application of this constraint");
  HTMLText(html, "are those unpreassigned event resources which lie in the");
  HTMLText(html, "given events and have the given");
  HTMLTextMonoNoBreak(html, "Role");
  HTMLText(html, ".  As usual, there must be at least one point of");
  HTMLText(html, "application.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of the optional");
  HTMLTextMono(html, "ResourceGroups");
  HTMLText(html, "child category is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "*ResourceGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "ResourceGroup");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and references a resource group.  The syntax of the");
  HTMLText(html, "optional");
  HTMLTextMono(html, "Resources");
  HTMLText(html, "child category is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resources");
  HTMLLiteralTextIndented(html,  8, "*Resource");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "Resource");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resource Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and references a resource.  Together, these two child");
  HTMLText(html, "categories enumerate a set of resources, namely the");
  HTMLText(html, "members of the resource groups referenced plus the");
  HTMLText(html, "individual resources referenced.  These resources are the");
  HTMLTextItalicNoBreak(html, "preferred resources");
  HTMLText(html, ".  They must all have the same resource type.  It is not");
  HTMLText(html, "an error for a resource to occur more than once in this");
  HTMLText(html, "enumeration, but such resources are treated as having");
  HTMLText(html, "occurred only once.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one event resource) is calculated by taking");
  HTMLText(html, "all the solution resources derived from the event");
  HTMLText(html, "resource that are assigned a resource that is not one");
  HTMLText(html, "of the preferred resources, and summing the durations of");
  HTMLText(html, "the solution events that those solution resources lie in.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "This constraint ignores solution resources that are not");
  HTMLText(html, "assigned at all (the assign resource constraint handles");
  HTMLText(html, "those).  The case of an event resource for which there is");
  HTMLText(html, "one set of acceptable resources and a smaller set of");
  HTMLText(html, "preferred resources may be modelled by having two");
  HTMLText(html, "prefer resource constraints that apply to that event");
  HTMLText(html, "resource.  There is no need to use a prefer resources");
  HTMLText(html, "constraint merely to constrain assignments to be of");
  HTMLText(html, "resources of a certain type, since each event resource");
  HTMLText(html, "has a resource type, and the entire solution is rejected");
  HTMLText(html, "if a solution resource derived from it is assigned a");
  HTMLText(html, "resource of any other type.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintPreferTimesSpec(HTML html, char *name)                    */
/*                                                                           */
/*  Print a specification of prefer times constraints onto html.             */
/*                                                                           */
/*****************************************************************************/

static void ConstraintPreferTimesSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Prefer times constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "A");
  HTMLTextItalic(html, "prefer times constraint");
  HTMLText(html, "specifies that some times are preferred for assignment");
  HTMLText(html, "to some solution events.  It has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "PreferTimesConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+TimeGroups");
  HTMLLiteralTextIndented(html,  8, "+Times");
  HTMLLiteralTextIndented(html,  8, "+Duration");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+EventGroups");
  HTMLLiteralTextIndented(html,  8, "+Events");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and defines a non-empty set of events in the same way as");
  HTMLText(html, "for the assign resource constraint above.  Exactly as for");
  HTMLText(html, "the assign time constraint, the points of application of");
  HTMLText(html, "this constraint are the members of this set of events");
  HTMLText(html, "that do not have a preassigned time.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of the optional");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "child category is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "*TimeGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "TimeGroup");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and references a time group.  The syntax of the optional");
  HTMLTextMono(html, "Times");
  HTMLText(html, "child category is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Times");
  HTMLLiteralTextIndented(html,  8, "*Time");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "Time");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Time Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and references a time.  Together, these two child");
  HTMLText(html, "categories enumerate a set of times, namely the members");
  HTMLText(html, "of the time groups referenced plus the individual times");
  HTMLText(html, "referenced.  These times are the");
  HTMLTextItalicNoBreak(html, "preferred times");
  HTMLText(html, ".  It is not an error for a time to occur more than");
  HTMLText(html, "once in this enumeration, but such times are treated");
  HTMLText(html, "as having occurred only once.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The optional");
  HTMLTextMono(html, "Duration");
  HTMLText(html, "child category has no attributes and no child categories,");
  HTMLText(html, "only a body, whose value must be a positive integer.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one instance event) is the total duration of");
  HTMLText(html, "those solution events derived from that instance event");
  HTMLText(html, "that are assigned a time that is not one of the preferred");
  HTMLText(html, "times.  If the optional");
  HTMLTextMono(html, "Duration");
  HTMLText(html, "child category is present, solution events with durations");
  HTMLText(html, "other than");
  HTMLTextMono(html, "Duration");
  HTMLText(html, "are ignored.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Solution events that are not assigned any time are");
  HTMLText(html, "ignored (the assign time constraint handles those).");
  HTMLText(html, "Restricting to solution events of a given duration is a");
  HTMLText(html, "practical necessity.  For example, a solution event of");
  HTMLText(html, "duration 1 can be assigned the last time in a day, whereas");
  HTMLText(html, "a solution event of longer duration cannot.  Thus, several");
  HTMLText(html, "prefer times constraints are typically needed, one for");
  HTMLText(html, "each duration.  The prefer times constraint handles");
  HTMLText(html, "preferences arising from durations in this way, and also");
  HTMLText(html, "preferences for certain days or times of day, without");
  HTMLText(html, "distinguishing between them.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintAvoidSplitAssignmentsSpec(HTML html, char *name)          */
/*                                                                           */
/*  Print a specification of avoid split assignments constraints onto html.  */
/*                                                                           */
/*****************************************************************************/

static void ConstraintAvoidSplitAssignmentsSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Avoid split assignments constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "Each solution resource may be assigned only one resource,");
  HTMLText(html, "which attends the enclosing solution event for its full");
  HTMLText(html, "duration.  However, when an instance event is split into");
  HTMLText(html, "solution events, each of its event resources is split into");
  HTMLText(html, "several solution resources, and a different resource may");
  HTMLText(html, "be assigned to each of these solution resources.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "For a given event resource there are three cases:");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLNumberedListBegin(html);

  HTMLParagraphBegin(html);
  HTMLListItemBegin(html);
  HTMLText(html, "The event resource is preassigned.  Then the solution");
  HTMLText(html, "resources derived from it are preassigned too, and");
  HTMLText(html, "there is nothing more to say about it.  This case is");
  HTMLText(html, "typical of event resources of student group type.");
  HTMLListItemEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLListItemBegin(html);
  HTMLText(html, "The event resource is not preassigned, and it is");
  HTMLText(html, "acceptable for the value assigned to the solution");
  HTMLText(html, "resources derived from it to vary.  This case might");
  HTMLText(html, "occur with event resources of room type:  it may not");
  HTMLText(html, "matter if a class meets in a different room for each");
  HTMLText(html, "of its lessons.");
  HTMLListItemEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLListItemBegin(html);
  HTMLText(html, "The event resource is not preassigned, and it is");
  HTMLText(html, "preferable for the value assigned to the solution");
  HTMLText(html, "resources derived from it not to vary.  This requirement,");
  HTMLText(html, "which has been called");
  HTMLTextItalicNoBreak(html, "resource stability");
  HTMLText(html, ", is typical of event resources of teacher type:  a");
  HTMLText(html, "class wants to meet the same teacher for all of its");
  HTMLText(html, "lessons, not different teachers for different lessons.");
  HTMLText(html, "When resource stability is wanted but not achieved, a");
  HTMLTextItalic(html, "split assignment");
  HTMLText(html, "is said to have been made.");
  HTMLListItemEnd(html);
  HTMLParagraphEnd(html);

  HTMLNumberedListEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "An");
  HTMLTextItalic(html, "avoid split assignments constraint");
  HTMLText(html, "specifies that split assignments are undesirable");
  HTMLText(html, "for some event resources.  It is needed to obtain the");
  HTMLText(html, "third case above.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AvoidSplitAssignmentsConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "Role");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "EventGroups");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "EventGroups");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventGroups");
  HTMLLiteralTextIndented(html,  8, "*EventGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and");
  HTMLTextMono(html, "EventGroup");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each event group referenced is one point of application;");
  HTMLText(html, "the individual events of the event groups are");
  HTMLTextItalic(html, "not");
  HTMLText(html, "the points of application.  The event groups referenced");
  HTMLText(html, "may be defined as Courses.  Having event groups as points");
  HTMLText(html, "of application allows the constraint to coordinate resource");
  HTMLText(html, "assignments to a whole set of events, as needed when a");
  HTMLText(html, "course is defined as a set of separate events rather than");
  HTMLText(html, "as a single event that is expected to split.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The compulsory");
  HTMLTextMono(html, "Role");
  HTMLText(html, "child category has no attributes and no child categories,");
  HTMLText(html, "only a body.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one event group) is calculated as follows.");
  HTMLText(html, "The events of the event group must all contain an");
  HTMLText(html, "event resource with the given");
  HTMLTextMonoNoBreak(html, "Role");
  HTMLText(html, ", and the resource types of those event resources must");
  HTMLText(html, "be the same.  The constraint examines all the solution");
  HTMLText(html, "resources derived from those event resources, and");
  HTMLText(html, "calculates the number of distinct resources assigned");
  HTMLText(html, "to them, ignoring unassigned solution resources (these");
  HTMLText(html, "are handled by the assign resource constraint).  The");
  HTMLText(html, "deviation is the amount by which this number exceeds 1.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintSpreadEventsSpec(HTML html, char *name)                   */
/*                                                                           */
/*  Print a specification of spread events constraints onto html.            */
/*                                                                           */
/*****************************************************************************/

static void ConstraintSpreadEventsSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Spread events constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "A");
  HTMLTextItalic(html, "spread events constraint");
  HTMLText(html, "specifies that the solution events of an event group");
  HTMLText(html, "should be spread out in time.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "SpreadEventsConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "TimeGroups");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "EventGroups");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where the syntax of");
  HTMLTextMono(html, "EventGroups");
  HTMLText(html, "is as for the avoid split assignments constraint above.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each event group referenced is one point of application;");
  HTMLText(html, "the individual events of the event groups are");
  HTMLTextItalic(html, "not");
  HTMLText(html, "the points of application.  The event groups referenced");
  HTMLText(html, "may be defined as Courses.  Having event groups as points");
  HTMLText(html, "of application allows the constraint to evaluate the");
  HTMLText(html, "spread of times assigned to a whole set of events, as");
  HTMLText(html, "needed when a course is defined as a set of separate events");
  HTMLText(html, "rather than as a single event that is expected to split.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "TimeGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where the syntax of");
  HTMLTextMono(html, "TimeGroup");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroup Reference");
  HTMLLiteralTextIndented(html,  8, "Minimum");
  HTMLLiteralTextIndented(html,  8, "Maximum");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each");
  HTMLTextMono(html, "TimeGroup");
  HTMLText(html, "category references a time group, but it also has two");
  HTMLText(html, "child categories which both have no attributes and no");
  HTMLText(html, "child categories of their own, just a body that");
  HTMLText(html, "must consist entirely of a non-negative integer,");
  HTMLText(html, "with");
  HTMLTextMono(html, "Maximum");
  HTMLText(html, "not smaller than");
  HTMLTextMonoNoBreak(html, "Minimum");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one event group) is calculated as follows.");
  HTMLText(html, "For each time group, find the number of solution");
  HTMLText(html, "events derived from the instance events of the event");
  HTMLText(html, "group that have a starting time (possibly preassigned)");
  HTMLText(html, "lying in the time group, and the amount by which this");
  HTMLText(html, "number falls short of the minimum or exceeds the maximum");
  HTMLText(html, "for the time group.  The deviation for the event group");
  HTMLText(html, "is the sum, over all time groups, of these amounts.");
  HTMLText(html, "Solution events with no assigned time are ignored.");
  HTMLParagraphEnd(html);

  /* ***
  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviations.");
  HTMLText(html, "Unlike most constraints, when a spread events constraint");
  HTMLText(html, "is evaluated at one point of application (one event group)");
  HTMLText(html, "it produces a whole set of deviations, one for each time");
  HTMLText(html, "group, rather than a single deviation.  For each time");
  HTMLText(html, "group, calculate how many solution events derived from");
  HTMLText(html, "the instance events of the event group have a starting time");
  HTMLText(html, "(possibly preassigned) which lies in the time group.");
  HTMLText(html, "Solution events which have no assigned time are ignored.");
  HTMLText(html, "If this number is below the minimum or above the maximum");
  HTMLText(html, "for the the time group, the deviation is the amount by");
  HTMLText(html, "which it falls short of the minimum or exceeds the maximum.");
  HTMLParagraphEnd(html);
  *** */

  /* ***
  HTMLParagraphBegin(html);
  HTMLTextItalic(html, "Note.  Before Version 1.13, this constraint tested");
  HTMLTextItalic(html, "whether solution events overlapped the time groups,");
  HTMLTextItalic(html, "not whether the starting times of the solution events");
  HTMLTextItalic(html, "lay in the time groups.  The change was made because");
  HTMLTextItalic(html, "it gives the same result in practice but is faster to");
  HTMLTextItalic(html, "evaluate, an important point for solvers, which must");
  HTMLTextItalic(html, "evaluate spread events constraints many times while");
  HTMLTextItalic(html, "solving.");
  HTMLParagraphEnd(html);
  *** */

  /* ***
  HTMLParagraphBegin(html);
  HTMLTextItalic(html, "Note.  Before Version 1.29, the result of");
  HTMLTextItalic(html, "evaluating a spread events constraint at one point");
  HTMLTextItalic(html, "of application was a set of deviations, one for each");
  HTMLTextItalic(html, "time group, not a single deviation.  The single");
  HTMLTextItalic(html, "deviation used now is the sum of the old ones.  This");
  HTMLTextItalic(html, "change does not change the cost of any solution to");
  HTMLTextItalic(html, "any instance expressed using the old versions of the");
  HTMLTextItalic(html, "current cost functions, since those old functions");
  HTMLTextItalic(html, "began by summing the deviations anyway.");
  HTMLParagraphEnd(html);
  *** */

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintLinkEventsSpec(HTML html, char *name)                     */
/*                                                                           */
/*  Print a specification of link events constraints onto html.              */
/*                                                                           */
/*****************************************************************************/

static void ConstraintLinkEventsSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Link events constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "A");
  HTMLTextItalic(html, "link events constraint");
  HTMLText(html, "specifies that certain events");
  HTMLText(html, "should be assigned the same times.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "LinkEventsConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "EventGroups");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where the syntax of");
  HTMLTextMono(html, "EventGroups");
  HTMLText(html, "is as for the avoid split events constraint above.");
  HTMLText(html, "Each event group referenced is one point of application;");
  HTMLText(html, "the individual events of the event groups are");
  HTMLTextItalic(html, "not");
  HTMLText(html, "the points of application.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one event group) is calculated as follows.");
  HTMLText(html, "For each instance event of the event group, build the");
  HTMLText(html, "set of times that the solution events derived from that");
  HTMLText(html, "instance event are running (all their times, not just their");
  HTMLText(html, "starting times).  Solution events with no assigned time are");
  HTMLText(html, "ignored.  The deviation is the number of times that appear");
  HTMLText(html, "in at least one of these sets but not in all of them.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintOrderEventsSpec(HTML html, char *name)                    */
/*                                                                           */
/*  Print a specification of order events constraints onto html.             */
/*                                                                           */
/*****************************************************************************/

static void ConstraintOrderEventsSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Order events constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "An");
  HTMLTextItalic(html, "order events constraint");
  HTMLText(html, "specifies that the times of two events");
  HTMLText(html, "should be constrained so that the first event ends");
  HTMLText(html, "before the second begins.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "OrderEventsConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "EventPairs");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where the syntax of");
  HTMLTextMono(html, "EventPairs");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventPairs");
  HTMLLiteralTextIndented(html,  8, "*EventPair");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each event pair is one point of application;");
  HTMLText(html, "its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventPair");
  HTMLLiteralTextIndented(html,  8, "FirstEvent Reference");
  HTMLLiteralTextIndented(html,  8, "SecondEvent Reference");
  HTMLLiteralTextIndented(html,  8, "+MinSeparation");
  HTMLLiteralTextIndented(html,  8, "+MaxSeparation");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextMono(html, "FirstEvent");
  HTMLText(html, "and");
  HTMLTextMono(html, "SecondEvent");
  HTMLText(html, "contain references to the two instance events whose times");
  HTMLText(html, "are to be constrained.");
  HTMLTextMono(html, "MinSeparation");
  HTMLText(html, "and");
  HTMLTextMono(html, "MaxSeparation");
  HTMLText(html, "have no attributes and no children, only a body,");
  HTMLText(html, "which must contain a single non-negative integer.");
  HTMLTextMono(html, "MinSeparation");
  HTMLText(html, "is the minimum number of times that may separate the");
  HTMLText(html, "two events without incurring a cost.  Its default value");
  HTMLText(html, "is 0.");
  HTMLTextMono(html, "MaxSeparation");
  HTMLText(html, "is the maximum number of times that may separate the");
  HTMLText(html, "two events without incurring a cost.  Its default value");
  HTMLText(html, "is infinity.  For example, omitting both");
  HTMLTextMono(html, "MinSeparation");
  HTMLText(html, "and");
  HTMLTextMono(html, "MaxSeparation");
  HTMLText(html, "constrains");
  HTMLTextMono(html, "FirstEvent");
  HTMLText(html, "to end before");
  HTMLTextMono(html, "SecondEvent");
  HTMLText(html, "begins, nothing more.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one pair of instance events) is calculated");
  HTMLText(html, "as follows.  If for");
  HTMLText(html, "either instance event there are no solution events, or");
  HTMLText(html, "there is a solution event whose time is unassigned, then");
  HTMLText(html, "the deviation is 0.  Otherwise, find the maximum over all");
  HTMLText(html, "solution events derived from the first instance event of");
  HTMLText(html, "the time (considered here to be an integer, the position of");
  HTMLText(html, "the time in the chronological ordering) plus duration of");
  HTMLText(html, "that solution event, and find the minimum over all solution");
  HTMLText(html, "events derived from the second instance event of the time");
  HTMLText(html, "of that solution event.  The amount by which the second");
  HTMLText(html, "of these two numbers minus the first exceeds");
  HTMLTextMono(html, "MaxSeparation");
  HTMLText(html, "or falls short of");
  HTMLTextMono(html, "MinSeparation");
  HTMLText(html, "is the deviation.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintAvoidClashesSpec(HTML html, char *name)                   */
/*                                                                           */
/*  Print a specification of avoid clashes constraints onto html.            */
/*                                                                           */
/*****************************************************************************/

static void ConstraintAvoidClashesSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Avoid clashes constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "An");
  HTMLTextItalic(html, "avoid clashes constraint");
  HTMLText(html, "specifies that certain resources should have no clashes;");
  HTMLText(html, "that is, they should not attend two or more events");
  HTMLText(html, "simultaneously.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AvoidClashesConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "+Resources");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of");
  HTMLTextMono(html, "ResourceGroups");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "*ResourceGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "ResourceGroup");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of");
  HTMLTextMono(html, "Resources");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resources");
  HTMLLiteralTextIndented(html,  8, "*Resource");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "Resource");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resource Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Together, these two child categories enumerate a set of");
  HTMLText(html, "resources, namely the members of the resource groups");
  HTMLText(html, "referenced plus the individual resources referenced.");
  HTMLText(html, "These resources are the points of application of this");
  HTMLText(html, "constraint.  Multiple occurrences of resources cause");
  HTMLText(html, "constraint violations to be counted multiple times.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one resource) is the sum, over all times");
  HTMLText(html, "when the resource is preassigned or assigned to two or");
  HTMLText(html, "more solution resources, of the number of solution");
  HTMLText(html, "resources it is preassigned or assigned to minus one.");
  HTMLText(html, "All times when the solution resources' solution events");
  HTMLText(html, "are running are included, not just their starting times.");
  HTMLParagraphEnd(html);

  /* ***
  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviations.");
  HTMLText(html, "The avoid clashes constraint produces a set of deviations");
  HTMLText(html, "at each point of application (each resource), not just");
  HTMLText(html, "one deviation.  For each time when the resource is");
  HTMLText(html, "preassigned or assigned to two or more solution resources,");
  HTMLText(html, "there is one deviation whose value is the number of");
  HTMLText(html, "solution resources minus one.");
  HTMLParagraphEnd(html);
  *** */

  /* ***
  HTMLParagraphBegin(html);
  HTMLTextItalic(html, "Note.  Before Version 1.29, the result of");
  HTMLTextItalic(html, "evaluating an avoid clashes constraint at one point");
  HTMLTextItalic(html, "of application was a set of deviations, one for each");
  HTMLTextItalic(html, "clashing time, not a single deviation.  The single");
  HTMLTextItalic(html, "deviation used now is the sum of the old ones.  This");
  HTMLTextItalic(html, "change does not change the cost of any solution to");
  HTMLTextItalic(html, "any instance expressed using the old versions of the");
  HTMLTextItalic(html, "current cost functions, since those old functions");
  HTMLTextItalic(html, "began by summing the deviations anyway.");
  HTMLParagraphEnd(html);
  *** */

  HTMLParagraphBegin(html);
  HTMLTextItalic(html, "Note.  Before Version 1.38, this specification");
  HTMLTextItalic(html, "incorrectly stated that multiple occurrences of");
  HTMLTextItalic(html, "resources were ignored.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintAvoidUnavailableTimesSpec(HTML html, char *name)          */
/*                                                                           */
/*  Print a specification of avoid unavailable times constraints onto html.  */
/*                                                                           */
/*****************************************************************************/

static void ConstraintAvoidUnavailableTimesSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Avoid unavailable times constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "An");
  HTMLTextItalic(html, "avoid unavailable times constraint");
  HTMLText(html, "specifies that certain resources are unavailable to");
  HTMLText(html, "attend any events at certain times.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AvoidUnavailableTimesConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+TimeGroups");
  HTMLLiteralTextIndented(html,  8, "+Times");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "+Resources");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "They define a set of resources in the same way as for");
  HTMLText(html, "the avoid clashes constraint above.  The syntax of");
  HTMLText(html, "the optional");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "child category is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "*TimeGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "TimeGroup");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and references a time group, which could be");
  HTMLText(html, "a Day or Week.  The syntax of the optional");
  HTMLTextMono(html, "Times");
  HTMLText(html, "child category is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Times");
  HTMLLiteralTextIndented(html,  8, "*Time");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "Time");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Time Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and references a time.  Together, these two child");
  HTMLText(html, "categories enumerate a set of times, namely the members");
  HTMLText(html, "of the time groups referenced plus the individual times");
  HTMLText(html, "referenced.  These times are the");
  HTMLTextItalicNoBreak(html, "unavailable times");
  HTMLText(html, ".  It is not an error for a time to occur more than");
  HTMLText(html, "once in this enumeration, but such times are treated");
  HTMLText(html, "as having occurred only once.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one resource) is the number of unavailable");
  HTMLText(html, "times during which the resource attends at least one");
  HTMLText(html, "solution event.  Cases where the resource attends two");
  HTMLText(html, "or more solution events receive no special treatment");
  HTMLText(html, "(the avoid clashes constraint handles those).");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintLimitIdleTimesSpec(HTML html, char *name)                 */
/*                                                                           */
/*  Print a specification of limit idle times constraints onto html.         */
/*                                                                           */
/*****************************************************************************/

static void ConstraintLimitIdleTimesSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Limit idle times constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "A resource is");
  HTMLTextItalic(html, "busy");
  HTMLText(html, "at some time when it attends at least one solution event");
  HTMLText(html, "at that time.  A resource is");
  HTMLTextItalic(html, "idle");
  HTMLText(html, "at some time (more precisely,");
  HTMLTextItalicNoBreak(html, "idle with respect to a given time group");
  HTMLText(html, ") when it is not busy at that time but it is busy at");
  HTMLText(html, "some earlier time of the time group, and at some later");
  HTMLText(html, "time, taking the time group's times in the order in which");
  HTMLText(html, "they are defined in the file (chronological order).");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "For example, suppose the time group is the Wednesday");
  HTMLText(html, "times, and the resource is busy at the third and seventh");
  HTMLText(html, "times of that time group, and not busy at its other times.");
  HTMLText(html, "Then it is idle at the fourth, fifth, and sixth times of");
  HTMLText(html, "that time group, but not at its first or second times or");
  HTMLText(html, "after the seventh time.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "A");
  HTMLTextItalic(html, "limit idle times constraint");
  HTMLText(html, "places limits on the number of times that resources");
  HTMLText(html, "may be idle.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "LimitIdleTimesConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "Minimum");
  HTMLLiteralTextIndented(html,  8, "Maximum");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "+Resources");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "They define a set of resources in the same way as for");
  HTMLText(html, "the avoid clashes constraint above.  The syntax of the");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "child category is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "*TimeGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "TimeGroup");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and references a time group, which could be");
  HTMLText(html, "a Day or Week.  Each time group must be");
  HTMLTextItalicNoBreak(html, "compact");
  HTMLText(html, ":  that is, if it is non-empty then every time between");
  HTMLText(html, "the chronologically first and the chronologically last");
  HTMLText(html, "must be present.  The");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "and");
  HTMLTextMono(html, "Maximum");
  HTMLText(html, "child categories have no attributes and no child");
  HTMLText(html, "categories of their own, just a body which must contain");
  HTMLText(html, "a single non-negative integer, with");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "not exceeding");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  /* *** weird and wrong, as discovered by Matias Sorensen
  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "For each point of application (each resource), a single");
  HTMLText(html, "deviation is calculated as follows.  For each time group,");
  HTMLText(html, "find the amount by which the number of times that the");
  HTMLText(html, "resource is idle with respect to that time group exceeds");
  HTMLTextMono(html, "Maximum");
  HTMLText(html, "or falls short of");
  HTMLTextMonoNoBreak(html, "Minimum");
  HTMLText(html, ".  The deviation is the sum of these amounts.");
  HTMLParagraphEnd(html);
  *** */

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one resource) is calculated as follows.");
  HTMLText(html, "For each time group, find the number of idle times");
  HTMLText(html, "with respect to that time group.  The deviation is the");
  HTMLText(html, "amount by which the sum of these numbers exceeds");
  HTMLTextMono(html, "Maximum");
  HTMLText(html, "or falls short of");
  HTMLTextMonoNoBreak(html, "Minimum");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextItalic(html, "Note.  This definition of the deviation has always");
  HTMLTextItalic(html, "been what was intended and what was implemented.");
  HTMLTextItalic(html, "But before Version 1.26, this documentation gave");
  HTMLTextItalic(html, "a different and therefore wrong definition of it.");
  HTMLTextItalic(html, "The compactness requirement was added recently,");
  HTMLTextItalic(html, "partly as a sanity measure and partly to keep things");
  HTMLTextItalic(html, "manageable for solvers.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintClusterBusyTimesSpec(HTML html, char *name)               */
/*                                                                           */
/*  Print a specification of cluster busy times constraints onto html.       */
/*                                                                           */
/*****************************************************************************/

static void ConstraintClusterBusyTimesSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Cluster busy times constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "A resource is");
  HTMLTextItalic(html, "busy");
  HTMLText(html, "at some time when it attends at least one solution event");
  HTMLText(html, "at that time, and busy during some time group if it is");
  HTMLText(html, "busy at one or more times of that time group.  A");
  HTMLTextItalic(html, "cluster busy times constraint");
  HTMLText(html, "places limits on the number of time groups during which");
  HTMLText(html, "a resource may be busy.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ClusterBusyTimesConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "Minimum");
  HTMLLiteralTextIndented(html,  8, "Maximum");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "+Resources");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "They define a set of resources in the same way as for");
  HTMLText(html, "the avoid clashes constraint above.  The syntax of the");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "child category is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "*TimeGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "TimeGroup");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and references a time group, which could be");
  HTMLText(html, "a Day or Week.  The");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "and");
  HTMLTextMono(html, "Maximum");
  HTMLText(html, "child categories have no attributes and no child");
  HTMLText(html, "categories of their own, just a body which must contain");
  HTMLText(html, "a single non-negative integer, with");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "not exceeding");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one resource) is the amount by which the");
  HTMLText(html, "number of given time groups during which the resource");
  HTMLText(html, "is busy falls short of");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "or exceeds");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintLimitBusyTimesSpec(HTML html, char *name)                  */
/*                                                                           */
/*  Print a specification of limit workload constraints onto html.           */
/*                                                                           */
/*****************************************************************************/

static void ConstraintLimitBusyTimesSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Limit busy times constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "A resource is");
  HTMLTextItalic(html, "busy");
  HTMLText(html, "at some time when it attends at least one solution event");
  HTMLText(html, "at that time.  A");
  HTMLTextItalic(html, "limit busy times constraint");
  HTMLText(html, "places limits on the number of times during certain");
  HTMLText(html, "time groups that a resource may be busy.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "LimitBusyTimesConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "Minimum");
  HTMLLiteralTextIndented(html,  8, "Maximum");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "+Resources");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "They define a set of resources in the same way as for");
  HTMLText(html, "the avoid clashes constraint above.  The syntax of the");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "child category is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "*TimeGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "TimeGroup");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and references a time group, which could be");
  HTMLText(html, "a Day or Week.  The");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "and");
  HTMLTextMono(html, "Maximum");
  HTMLText(html, "child categories have no attributes and no child");
  HTMLText(html, "categories of their own, just a body which must contain");
  HTMLText(html, "a single non-negative integer, with");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "not exceeding");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one resource) is the sum, over all time");
  HTMLText(html, "groups, of the amount by which the number of times that");
  HTMLText(html, "the resource is busy during that time group falls short of");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "or exceeds");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".  As a special case, when the number of times that the");
  HTMLText(html, "resource is busy is 0, the contribution to the deviation");
  HTMLText(html, "is 0.  To enforce limits on the number of days when a");
  HTMLText(html, "resource is busy, use a cluster busy times constraint.");
  HTMLParagraphEnd(html);

  /* ***
  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviations.");
  HTMLText(html, "For each point of application (for each resource), the");
  HTMLText(html, "limit busy times constraint produces a set of deviations,");
  HTMLText(html, "one for each of the given time groups, not just one");
  HTMLText(html, "deviation.  Each deviation is the amount by which the");
  HTMLText(html, "number of times of its time group during which the resource");
  HTMLText(html, "is busy falls short of");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "or exceeds");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".  As a special case, when the number of times that the");
  HTMLText(html, "resource is busy is 0, the deviation is 0.  To enforce");
  HTMLText(html, "limits on the number of days when a resource is busy,");
  HTMLText(html, "use the cluster busy times constraint.");
  HTMLParagraphEnd(html);
  *** */

  /* ***
  HTMLParagraphBegin(html);
  HTMLTextItalic(html, "Note.  Before Version 1.29, the result of");
  HTMLTextItalic(html, "evaluating a limit busy times constraint at one point");
  HTMLTextItalic(html, "of application was a set of deviations, one for each");
  HTMLTextItalic(html, "time group, not a single deviation.  The single");
  HTMLTextItalic(html, "deviation used now is the sum of the old ones.  This");
  HTMLTextItalic(html, "change does not change the cost of any solution to");
  HTMLTextItalic(html, "any instance expressed using the old versions of the");
  HTMLTextItalic(html, "current cost functions, since those old functions");
  HTMLTextItalic(html, "began by summing the deviations anyway.");
  HTMLParagraphEnd(html);
  *** */

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintLimitWorkloadSpec(HTML html, char *name)                  */
/*                                                                           */
/*  Print a specification of limit workload constraints onto html.           */
/*                                                                           */
/*****************************************************************************/

static void ConstraintLimitWorkloadSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Limit workload constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "The");
  HTMLTextItalic(html, "workload");
  HTMLText(html, "of an instance event is the value of its");
  HTMLTextMono(html, "Workload");
  HTMLText(html, "child category if it has one, or its duration if not.");
  HTMLText(html, "The workload of an event resource is the value of its");
  HTMLTextMono(html, "Workload");
  HTMLText(html, "child category if it has one, or the workload of the");
  HTMLText(html, "enclosing instance event if not.");
  HTMLText(html, "It is an integer.  The workload of a solution resource");
  HTMLTextItalic(html, "sr");
  HTMLText(html, "lying in solution event");
  HTMLTextItalic(html, "se");
  HTMLText(html, "and derived from event resource");
  HTMLTextItalic(html, "er");
  HTMLText(html, "of event");
  HTMLTextItalic(html, "e");
  HTMLText(html, "is defined by the formula");
  HTMLParagraphEnd(html);

  HTMLCentredDisplayBegin(html);
  HTMLTextItalic(html, "Workload(sr) = Duration(se) * Workload(er) / Duration(e)");
  HTMLCentredDisplayEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextItalic(html, "Duration(se)");
  HTMLText(html, "is the duration of");
  HTMLTextItalicNoBreak(html, "se");
  HTMLText(html, ",");
  HTMLTextItalic(html, "Workload(er)");
  HTMLText(html, "is the workload of");
  HTMLTextItalicNoBreak(html, "er");
  HTMLText(html, ", and");
  HTMLTextItalic(html, "Duration(e)");
  HTMLText(html, "is the duration of");
  HTMLTextItalicNoBreak(html, "e");
  HTMLText(html, ".  This value is a floating-point number.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "A");
  HTMLTextItalic(html, "limit workload constraint");
  HTMLText(html, "places limits on the total workload of solution resources");
  HTMLText(html, "that certain resources are assigned to.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "LimitWorkloadConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "Minimum");
  HTMLLiteralTextIndented(html,  8, "Maximum");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Its");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "+Resources");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "They define a set of resources in the same way as for");
  HTMLText(html, "the avoid clashes constraint above.  The");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "and");
  HTMLTextMono(html, "Maximum");
  HTMLText(html, "child categories have no attributes and no child");
  HTMLText(html, "categories of their own, just a body which must contain");
  HTMLText(html, "a single non-negative integer, with");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "not exceeding");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one resource) is the amount by which the");
  HTMLText(html, "total workload of the solution resources assigned that");
  HTMLText(html, "resource falls short of");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "or exceeds");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".  Each amount is rounded up to the next integer before");
  HTMLText(html, "being added to the sum.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextItalic(html, "Note.  Before Version 1.13, this constraint was");
  HTMLTextItalic(html, "defined differently, in a way that avoided");
  HTMLTextItalic(html, "non-integral workloads.  The new specification");
  HTMLTextItalic(html, "has been adopted because it is faster to evaluate,");
  HTMLTextItalic(html, "an important point for solvers, which must evaluate");
  HTMLTextItalic(html, "limit workload constraints many times while solving.");
  HTMLTextItalic(html, "Before Version 1.15, only events had workloads, not");
  HTMLTextItalic(html, "event resources.  The change allows the workload");
  HTMLTextItalic(html, "associated with two event resources from the same");
  HTMLTextItalic(html, "event to differ.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ConstraintSpecFull(void)                                            */
/*                                                                           */
/*  Print a detailed specification of constraints on a separate web page.    */
/*                                                                           */
/*****************************************************************************/

void ConstraintSpecFull(void)
{
  HTML html;
  char *hd = "High School Timetable File Format Specification: Constraints";
  html = PageBegin(hd);
  HTMLBigHeading(html, hd);

  HTMLParagraphBegin(html);
  HTMLText(html, "This page specifies the constraints appearing in the high");
  HTMLText(html, "school timetabling files used by HSEval.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLJumpInternalIndented(html, 4, "all",  "Constraints in general");
  HTMLJumpInternalIndented(html, 8, "arc",  "Assign resource constraints");
  HTMLJumpInternalIndented(html, 8, "atc",  "Assign time constraints");
  HTMLJumpInternalIndented(html, 8, "sec",  "Split events constraints");
  HTMLJumpInternalIndented(html, 8, "dsec",
    "Distribute split events constraints");
  HTMLJumpInternalIndented(html, 8, "prc",  "Prefer resources constraints");
  HTMLJumpInternalIndented(html, 8, "ptc",  "Prefer times constraints");
  HTMLJumpInternalIndented(html, 8, "asac",
    "Avoid split assignments constraints");
  HTMLJumpInternalIndented(html, 8, "spec", "Spread events constraints");
  HTMLJumpInternalIndented(html, 8, "lec",  "Link events constraints");
  HTMLJumpInternalIndented(html, 8, "ord",  "Order events constraints");
  HTMLJumpInternalIndented(html, 8, "acc",  "Avoid clashes constraints");
  HTMLJumpInternalIndented(html, 8, "autc",
    "Avoid unavailable times constraints");
  HTMLJumpInternalIndented(html, 8, "litc", "Limit idle times constraints");
  HTMLJumpInternalIndented(html, 8, "cbtc", "Cluster busy times constraints");
  HTMLJumpInternalIndented(html, 8, "lbtc",  "Limit busy times constraints");
  HTMLJumpInternalIndented(html, 8, "lwc",  "Limit workload constraints");
  HTMLParagraphEnd(html);
  HTMLHorizontalRule(html);

  ConstraintGeneralSpec(html, "all");
  ConstraintAssignResourceSpec(html, "arc");
  ConstraintAssignTimeSpec(html, "atc");
  ConstraintSplitEventsSpec(html, "sec");
  ConstraintDistributeSplitEventsSpec(html, "dsec");
  ConstraintPreferResourcesSpec(html, "prc");
  ConstraintPreferTimesSpec(html, "ptc");
  ConstraintAvoidSplitAssignmentsSpec(html, "asac");
  ConstraintSpreadEventsSpec(html, "spec");
  ConstraintLinkEventsSpec(html, "lec");
  ConstraintOrderEventsSpec(html, "ord");
  ConstraintAvoidClashesSpec(html, "acc");
  ConstraintAvoidUnavailableTimesSpec(html, "autc");
  ConstraintLimitIdleTimesSpec(html, "litc");
  ConstraintClusterBusyTimesSpec(html, "cbtc");
  ConstraintLimitBusyTimesSpec(html, "lbtc");
  ConstraintLimitWorkloadSpec(html, "lwc");

  HTMLParagraphBegin(html);
  HTMLText(html, "Return to the ");
  HTMLJumpFront(html);
  HTMLText(html, ".");
  HTMLParagraphEnd(html);
  PageEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void EmployeeArchiveSpec(HTML html, char *name)                          */
/*                                                                           */
/*  Print a detailed specification of employee scheduling archives.          */
/*                                                                           */
/*****************************************************************************/

static void EmployeeArchiveSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Employee scheduling archive files");

  HTMLParagraphBegin(html);
  HTMLText(html, "Employee scheduling archive files have almost the same");
  HTMLText(html, "format as high school timetabling archive files.  The");
  HTMLText(html, "outermost tag is");
  HTMLTextMono(html, "EmployeeScheduleArchive");
  HTMLText(html, "instead of");
  HTMLTextMonoNoBreak(html, "HighSchoolTimetableArchive");
  HTMLText(html, ".  The overall syntax is otherwise the same:");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EmployeeScheduleArchive +Id");
  HTMLLiteralTextIndented(html,  8, "+MetaData");
  HTMLLiteralTextIndented(html,  8, "+Instances");
  HTMLLiteralTextIndented(html, 12, "*Instance");
  HTMLLiteralTextIndented(html,  8, "+SolutionGroups");
  HTMLLiteralTextIndented(html, 12, "*SolutionGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "with the same meaning as for high school timetabling,");
  HTMLText(html, "except that the extensions defined on this page are");
  HTMLText(html, "available in");
  HTMLText(html, "employee scheduling archives but not in high school");
  HTMLText(html, "timetabling archives.  HSEval accepts both kinds of");
  HTMLText(html, "archives without having to be told which kind to expect.");
  HTMLParagraphEnd(html);
  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void EmployeeAbbreviatedEventResources(HTML html, char *name)            */
/*                                                                           */
/*  Print a detailed specification of employee scheduling abbreviated        */
/*  event resources.                                                         */
/*                                                                           */
/*****************************************************************************/

static void EmployeeAbbreviatedEventResources(HTML html, char *name)
{
  HTMLSegmentBegin(html, name,
    "Employee scheduling abbreviated event resources");

  HTMLParagraphBegin(html);
  HTMLText(html, "Employee scheduling instances can be very large.  Much");
  HTMLText(html, "of the length is owing to event resources.  Accordingly,");
  HTMLText(html, "XESTT offers the option of abbreviating their syntax.");
  HTMLText(html, "Instead of XHSTT's");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resources");
  HTMLLiteralTextIndented(html,  8, "*Resource");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resource +Reference");
  HTMLLiteralTextIndented(html,  8, "+Role");
  HTMLLiteralTextIndented(html,  8, "+ResourceType");
  HTMLLiteralTextIndented(html,  8, "+Workload");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "XESTT offers");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resources");
  HTMLLiteralTextIndented(html,  8, "*Resource");
  HTMLLiteralTextIndented(html,  8, "*R");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "R");
  HTMLText(html, "is an abbreviated form of");
  HTMLTextMonoNoBreak(html, "Resource");
  HTMLText(html, ":");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "R +Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "As shown above, a");
  HTMLTextMono(html, "Resource");
  HTMLText(html, "has four attributes, all optional:  a preassignment,");
  HTMLText(html, "referenced by");
  HTMLTextMonoNoBreak(html, "Reference");
  HTMLText(html, ", a role, a resource type, and a workload.  In");
  HTMLText(html, "the abbreviated ");
  HTMLTextMono(html, "R");
  HTMLText(html, "form, ");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "is as before;");
  HTMLTextMono(html, "Role");
  HTMLText(html, "occupies the text body of");
  HTMLTextMonoNoBreak(html, "R");
  HTMLText(html, ";");
  HTMLTextMono(html, "ResourceType");
  HTMLText(html, "is omitted and is taken to be the first of");
  HTMLText(html, "the resource types of the instance; and");
  HTMLTextMono(html, "Workload");
  HTMLText(html, "is omitted and is taken to be the workload of");
  HTMLText(html, "the enclosing event.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void JumpToHistoryPaper(HTML html)                                       */
/*                                                                           */
/*  Jump to my history paper.                                                */
/*                                                                           */
/*****************************************************************************/

static void JumpToHistoryPaper(HTML html)
{
  HTMLJumpToDocument(html, "http://jeffreykingston.id.au/tt_papers",
    NULL, "Jeff Kingston's paper on modelling history in nurse rostering");
}


/*****************************************************************************/
/*                                                                           */
/*  void EmployeeClusterBusyTimesSpec(HTML html, char *name)                 */
/*                                                                           */
/*  Print a detailed specification of employee scheduling cluster busy times */
/*  constraints.                                                             */
/*                                                                           */
/*****************************************************************************/

static void EmployeeClusterBusyTimesSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name,
    "Employee scheduling cluster busy times constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "The employee scheduling cluster busy times constraint has");
  HTMLText(html, "syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ClusterBusyTimesConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+ResourceHistory");
  HTMLLiteralTextIndented(html,  8, "+AppliesToTimeGroup");
  HTMLLiteralTextIndented(html,  8, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "Minimum");
  HTMLLiteralTextIndented(html,  8, "Maximum");
  HTMLLiteralTextIndented(html,  8, "+AllowZero");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "This is the high school timetabling cluster busy times");
  HTMLText(html, "constraint with four extensions.  First, an optional");
  HTMLTextMono(html, "ResourceHistory");
  HTMLText(html, "category has been added, with syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ResourceHistory before after");
  HTMLLiteralTextIndented(html,  8, "*Resource");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where the");
  HTMLTextMono(html, "before");
  HTMLText(html, "and");
  HTMLTextMono(html, "after");
  HTMLText(html, "attributes each contain a non-negative integer, and");
  HTMLTextMono(html, "Resource");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resource Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "with a body containing a non-negative integer not");
  HTMLText(html, "larger than the");
  HTMLTextMono(html, "before");
  HTMLText(html, "attribute of");
  HTMLTextMonoNoBreak(html, "ResourceHistory");
  HTMLText(html, ".  The reference may be to any resource, but only");
  HTMLText(html, "references to resources that are points of application");
  HTMLText(html, "of the constraint have any effect.  A resource may not");
  HTMLText(html, "appear more than once in the");
  HTMLTextMono(html, "ResourceHistory");
  HTMLText(html, "part of one constraint.");
  HTMLParagraphEnd(html);

  /* ***
  HTMLText(html, "part of one constraint.  In addition to its");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "attribute, the");
  HTMLTextMono(html, "Resource");
  HTMLText(html, "category has a body containing a non-negative");
  HTMLText(html, "integer called the resource's history.");
  HTMLParagraphEnd(html);
  *** */

  HTMLParagraphBegin(html);
  HTMLText(html, "The second extension is that an optional");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "category has been added, which references a time group:");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesToTimeGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The third extension is that the");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "category has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "*TimeGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "as before, but the syntax of");
  HTMLTextMono(html, "TimeGroup");
  HTMLText(html, "has been extended to");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroup Reference +Polarity");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The new element is the optional");
  HTMLTextMono(html, "Polarity");
  HTMLText(html, "attribute, which must have value");
  HTMLTextMono(html, "positive");
  HTMLText(html, "or");
  HTMLTextMonoNoBreak(html, "negative");
  HTMLText(html, "; its default value is");
  HTMLTextMonoNoBreak(html, "positive");
  HTMLText(html, ".  A time group is said to be positive or negative,");
  HTMLText(html, "depending on the value of this attribute.");
  HTMLParagraphEnd(html);

  HTMLParagraphEnd(html);
  HTMLText(html, "The fourth extension is the addition of the optional");
  HTMLTextMono(html, "AllowZero");
  HTMLText(html, "child category.  This has no attributes and no child");
  HTMLText(html, "categories of its own, just a body which must contain");
  HTMLText(html, "the value");
  HTMLTextMono(html, "true");
  HTMLText(html, "or");
  HTMLTextMonoNoBreak(html, "false");
  HTMLText(html, ".  Its default value is");
  HTMLTextMonoNoBreak(html, "false");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "ResourceHistory");
  HTMLText(html, "and");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "child categories are not present, the deviation at one");
  HTMLText(html, "point of application of this constraint (one resource)");
  HTMLText(html, "is the amount by which the number of active time groups");
  HTMLText(html, "falls short of");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "or exceeds");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".  A time group is active if it is a positive time group");
  HTMLText(html, "and the resource is busy during that time group, or it is");
  HTMLText(html, "a negative time group and the resource is not busy during");
  HTMLText(html, "that time group.  A resource is busy during a time group if");
  HTMLText(html, "it is busy at one or more of the times of the time group.");
  HTMLText(html, "Exception:  if");
  HTMLTextMono(html, "AllowZero");
  HTMLText(html, "is present with value");
  HTMLTextMono(html, "true");
  HTMLText(html, "and the number of active time groups is 0, then the");
  HTMLText(html, "deviation is 0.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "It is probably better if each time group is non-empty.");
  HTMLText(html, "However, the rules just given are valid for empty time");
  HTMLText(html, "groups:  they specify that a positive empty time group is");
  HTMLText(html, "always inactive, and a negative empty time group is always");
  HTMLText(html, "active.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "ResourceHistory");
  HTMLText(html, "child category is present, an");
  HTMLTextItalic(html, "xi");
  HTMLText(html, "value is found for each resource, which is the value of the");
  HTMLText(html, "body of the resource's entry in");
  HTMLTextMonoNoBreak(html, "ResourceHistory");
  HTMLText(html, ", or if none, 0.  And");
  HTMLTextItalic(html, "ai");
  HTMLText(html, "and");
  HTMLTextItalic(html, "ci");
  HTMLText(html, "values are found, which are the values");
  HTMLText(html, "of the");
  HTMLTextMono(html, "before");
  HTMLText(html, "and");
  HTMLTextMono(html, "after");
  HTMLText(html, "attributes of");
  HTMLTextMono(html, "ResourceHistory");
  HTMLText(html, "(the same for each resource).  Then, when comparing");
  HTMLText(html, "with a maximum limit and, when");
  HTMLTextMono(html, "AllowZero");
  HTMLText(html, "is true, with zero,");
  HTMLTextItalic(html, "xi");
  HTMLText(html, "is added to the number of active time groups; and");
  HTMLText(html, "when comparing with a minimum limit, both");
  HTMLTextItalic(html, "xi");
  HTMLText(html, "and");
  HTMLTextItalic(html, "ci");
  HTMLText(html, "are added to the number of active time groups.");
  HTMLText(html, "Furthermore, an adjustment, which depends on");
  HTMLTextItalicNoBreak(html, "ai");
  HTMLText(html, ",");
  HTMLTextItalicNoBreak(html, "ci");
  HTMLText(html, ", and");
  HTMLTextItalicNoBreak(html, "xi");
  HTMLText(html, ", is made to any reported cost of violation, to remove any");
  HTMLText(html, "double counting.  Full details appear in");
  JumpToHistoryPaper(html);
  HTMLText(html, ".  In the literature,");
  HTMLTextItalic(html, "xi");
  HTMLText(html, "and");
  HTMLTextItalic(html, "ci");
  HTMLText(html, "are usually called the counter start value and the");
  HTMLText(html, "counter remainder value.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "child category is present, for each time");
  HTMLTextMono(html, "ti");
  HTMLText(html, "in the applies-to time group, the constraint applies as");
  HTMLText(html, "defined without the applies-to time group, except that");
  HTMLText(html, "each time");
  HTMLTextMono(html, "t");
  HTMLText(html, "in the time groups of");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "is changed to the time");
  HTMLTextMono(html, "index(ti) - index(t1)");
  HTMLText(html, "places further along the cycle, where");
  HTMLTextMono(html, "t1");
  HTMLText(html, "is the first time in the applies-to time group.  If any of");
  HTMLText(html, "these times don't exist, the constraint does not apply at");
  HTMLTextMonoNoBreak(html, "ti");
  HTMLText(html, ".  One point of application of the constraint then becomes");
  HTMLText(html, "a pair consisting of a resource from");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "and an applicable time from");
  HTMLTextMonoNoBreak(html, "AppliesToTimeGroup");
  HTMLText(html, ".  It is possible to include all these clones of the");
  HTMLText(html, "constraint directly, and not use");
  HTMLTextMonoNoBreak(html, "AppliesToTimeGroup");
  HTMLText(html, ", but that can become very verbose.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "This definition is compatible with the");
  HTMLText(html, "high school timetabling definition:  every high school");
  HTMLText(html, "timetabling cluster busy times constraint is also a");
  HTMLText(html, "legal employee scheduling cluster busy times constraint,");
  HTMLText(html, "and when considered as such, its meaning does not change.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void EmployeeClusterConsecutiveBusyTimesSpec(HTML html, char *name)      */
/*                                                                           */
/*  Print a detailed specification of employee scheduling cluster consective */
/*  busy times constraints.                                                  */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
static void EmployeeClusterConsecutiveBusyTimesSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name,
    "Employee scheduling cluster consecutive busy times constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "The employee scheduling cluster consecutive busy times");
  HTMLText(html, "constraint has almost exactly the same syntax as the");
  HTMLText(html, "employee scheduling cluster busy times constraint:");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "ClusterConsecutiveBusyTimesConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+AppliesToTimeGroup");
  HTMLLiteralTextIndented(html,  8, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "Minimum");
  HTMLLiteralTextIndented(html,  8, "Maximum");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The only differences are the change of name, from");
  HTMLTextMono(html, "ClusterBusyTimesConstraint");
  HTMLText(html, "to");
  HTMLTextMonoNoBreak(html, "ClusterConsecutiveBusyTimesConstraint");
  HTMLText(html, ", and the absence of");
  HTMLTextMonoNoBreak(html, "AllowZero");
  HTMLText(html, ".  Each of the child attributes shown has the same syntax");
  HTMLText(html, "as it does for the employee scheduling cluster busy");
  HTMLText(html, "times constraint.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "child category is not present, the deviation at one point");
  HTMLText(html, "of application of this constraint (one resource) is the");
  HTMLText(html, "sum, over all maximal non-empty subsequences of active");
  HTMLText(html, "time groups (taken in the order they are listed in the");
  HTMLText(html, "constraint), of the amount by which the length of the");
  HTMLText(html, "subsequence falls short of");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "or exceeds");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".  For the meaning of `active time group' consult the");
  HTMLText(html, "specification of the employee scheduling cluster busy");
  HTMLText(html, "times constraint.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "child category is present, its effect is the same as");
  HTMLText(html, "for the employee scheduling cluster busy times constraint.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "In summmary, the cluster consecutive busy times constraint");
  HTMLText(html, "is the same as the cluster busy times constraint except");
  HTMLText(html, "that the limits apply to the lengths of the maximal");
  HTMLText(html, "non-empty subsequences of active time groups, instead of to");
  HTMLText(html, "the total number of active time groups.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void EmployeeLimitBusyTimesSpec(HTML html, char *name)                   */
/*                                                                           */
/*  Print a detailed specification of employee scheduling limit busy times   */
/*  constraints.                                                             */
/*                                                                           */
/*****************************************************************************/

static void EmployeeLimitBusyTimesSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name,
    "Employee scheduling limit busy times constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "The employee scheduling limit busy times constraint has");
  HTMLText(html, "syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "LimitBusyTimesConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+AppliesToTimeGroup");
  HTMLLiteralTextIndented(html,  8, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "Minimum");
  HTMLLiteralTextIndented(html,  8, "Maximum");
  HTMLLiteralTextIndented(html,  8, "+AllowZero");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "This is the same as the high school timetabling limit");
  HTMLText(html, "busy times constraint, with two extensions.  First,");
  HTMLText(html, "an optional");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "category has been added, which references a time group:");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesToTimeGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphEnd(html);
  HTMLText(html, "The second change is the addition of the optional");
  HTMLTextMono(html, "AllowZero");
  HTMLText(html, "child category.  This has no attributes and no child");
  HTMLText(html, "categories of its own, just a body which must contain");
  HTMLText(html, "the value");
  HTMLTextMono(html, "true");
  HTMLText(html, "or");
  HTMLTextMonoNoBreak(html, "false");
  HTMLText(html, ".  Its default value is");
  HTMLTextMonoNoBreak(html, "true");
  HTMLText(html, ".  (For compatibility with the high school model, this");
  HTMLText(html, "default value is different from the default value of the");
  HTMLTextMono(html, "AllowZero");
  HTMLText(html, "child category of the cluster busy times constraint.");
  HTMLText(html, "What is new here is not the ability to allow zero, but");
  HTMLText(html, "the ability to disallow it.)");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "child category is not present, the deviation at one point");
  HTMLText(html, "of application of this");
  HTMLText(html, "constraint (one resource) is the sum, over all time");
  HTMLText(html, "groups, of the amount by which the number of times that");
  HTMLText(html, "the resource is busy during that time group falls short of");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "or exceeds");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".  Exception:  if");
  HTMLTextMono(html, "AllowZero");
  HTMLText(html, "is absent or present with value");
  HTMLTextMonoNoBreak(html, "true");
  HTMLText(html, ", and the number of times that the resource is busy in");
  HTMLText(html, "some time group is 0, then the contribution of that time");
  HTMLText(html, "group to the deviation is 0.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "child category is present, for each time");
  HTMLTextMono(html, "ti");
  HTMLText(html, "in the applies-to time group, the constraint applies as");
  HTMLText(html, "defined without the applies-to time group, except that");
  HTMLText(html, "each time");
  HTMLTextMono(html, "t");
  HTMLText(html, "in the time groups of");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "is changed to the time");
  HTMLTextMono(html, "index(ti) - index(t1)");
  HTMLText(html, "places further along the cycle from");
  HTMLTextMonoNoBreak(html, "t");
  HTMLText(html, ", where");
  HTMLTextMono(html, "t1");
  HTMLText(html, "is the first time in the applies-to time group.  If any");
  HTMLText(html, "of these new times don't exist, the constraint does not");
  HTMLText(html, "apply at");
  HTMLTextMonoNoBreak(html, "ti");
  HTMLText(html, ".  One point of application of the constraint then becomes");
  HTMLText(html, "a pair consisting of a resource from");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "and an applicable time");
  HTMLTextMono(html, "ti");
  HTMLText(html, "from");
  HTMLTextMonoNoBreak(html, "AppliesToTimeGroup");
  HTMLText(html, ".  It is possible to include all these clones of the");
  HTMLText(html, "constraint directly, and not use");
  HTMLTextMonoNoBreak(html, "AppliesToTimeGroup");
  HTMLText(html, ", but that can become very verbose.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "This definition is compatible with the");
  HTMLText(html, "high school timetabling definition:  every high school");
  HTMLText(html, "timetabling limit busy times constraint is also a");
  HTMLText(html, "legal employee scheduling limit busy times constraint,");
  HTMLText(html, "and when considered as such, its meaning does not change.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void EmployeeLimitWorkloadSpec(HTML html, char *name)                    */
/*                                                                           */
/*  Print a detailed specification of employee scheduling limit workload     */
/*  constraints.                                                             */
/*                                                                           */
/*****************************************************************************/

static void EmployeeLimitWorkloadSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name,
    "Employee scheduling limit workload constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "The employee scheduling limit workload constraint has");
  HTMLText(html, "syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "LimitWorkloadConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+AppliesToTimeGroup");
  HTMLLiteralTextIndented(html,  8, "+TimeGroups");
  HTMLLiteralTextIndented(html,  8, "Minimum");
  HTMLLiteralTextIndented(html,  8, "Maximum");
  HTMLLiteralTextIndented(html,  8, "+AllowZero");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "This is the same as the high school timetabling limit");
  HTMLText(html, "workload constraint, with three extensions.  First,");
  HTMLText(html, "an optional");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "category has been added, which references a time group:");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesToTimeGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The second change is the addition of the optional");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "child category, with syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "*TimeGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "TimeGroup");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "TimeGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and references a time group, which could be a");
  HTMLText(html, "Day or Week.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The third change is the addition of the optional");
  HTMLTextMono(html, "AllowZero");
  HTMLText(html, "child category.  This has no attributes and no child");
  HTMLText(html, "categories of its own, just a body which must contain");
  HTMLText(html, "the value");
  HTMLTextMono(html, "true");
  HTMLText(html, "or");
  HTMLTextMonoNoBreak(html, "false");
  HTMLText(html, ".  Its default value is");
  HTMLTextMonoNoBreak(html, "false");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "child category is not present, the deviation at one point");
  HTMLText(html, "of application of this");
  HTMLText(html, "constraint (one resource) is the sum, over all time");
  HTMLText(html, "groups, of the amount by which the total workload of");
  HTMLText(html, "the solution resources assigned that");
  HTMLText(html, "resource during that time group falls short of");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "or exceeds");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".  Each amount is rounded up to the next integer before");
  HTMLText(html, "being added to the sum.  If there are no time");
  HTMLText(html, "groups, a single time group containing all the times of");
  HTMLText(html, "the cycle is used instead.  Exception:  if");
  HTMLTextMono(html, "AllowZero");
  HTMLText(html, "is present with value");
  HTMLTextMonoNoBreak(html, "true");
  HTMLText(html, ", and the total workload of the resource during some");
  HTMLText(html, "time group is 0.0, then the contribution of that time");
  HTMLText(html, "group to the deviation is 0.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The contribution of one solution resource to the workload");
  HTMLText(html, "during one time group is calculated as follows.  First,");
  HTMLText(html, "divide the workload of the solution resource's event");
  HTMLText(html, "resource by the duration of that event resource's event");
  HTMLText(html, "to obtain the workload per time (a floating-point number).");
  HTMLText(html, "Then the contribution is the workload per time multiplied");
  HTMLText(html, "by the number of times that the solution resource shares");
  HTMLText(html, "with the time group.  When the workload per time is not");
  HTMLText(html, "an integer, the usual roundoff errors associated with");
  HTMLText(html, "floating-point numbers may occur, especially during");
  HTMLText(html, "solving when the total workload undergoes many incremental");
  HTMLText(html, "adjustments.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "child category is present, for each time");
  HTMLTextMono(html, "ti");
  HTMLText(html, "in the applies-to time group, the constraint applies as");
  HTMLText(html, "defined without the applies-to time group, except that");
  HTMLText(html, "each time");
  HTMLTextMono(html, "t");
  HTMLText(html, "in the time groups of");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "is changed to the time");
  HTMLTextMono(html, "index(ti) - index(t1)");
  HTMLText(html, "places further along the cycle from");
  HTMLTextMonoNoBreak(html, "t");
  HTMLText(html, ", where");
  HTMLTextMono(html, "t1");
  HTMLText(html, "is the first time in the applies-to time group.  If any");
  HTMLText(html, "of these new times don't exist, the constraint does not");
  HTMLText(html, "apply at");
  HTMLTextMonoNoBreak(html, "ti");
  HTMLText(html, ".  One point of application of the constraint then becomes");
  HTMLText(html, "a pair consisting of a resource from");
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "and an applicable time");
  HTMLTextMono(html, "ti");
  HTMLText(html, "from");
  HTMLTextMonoNoBreak(html, "AppliesToTimeGroup");
  HTMLText(html, ".  It is possible to include all these clones of the");
  HTMLText(html, "constraint directly, and not use");
  HTMLTextMonoNoBreak(html, "AppliesToTimeGroup");
  HTMLText(html, ", but that can become very verbose.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "This definition is compatible with the");
  HTMLText(html, "high school timetabling definition:  every high school");
  HTMLText(html, "timetabling limit workload constraint is also a legal");
  HTMLText(html, "employee scheduling limit workload constraint, and");
  HTMLText(html, "when considered as such, its meaning does not change.");
  HTMLText(html, "It is identical to the employee scheduling limit busy");
  HTMLText(html, "times constraint except that");
  HTMLTextMono(html, "TimeGroups");
  HTMLText(html, "is optional, the default value of");
  HTMLTextMono(html, "AllowZero");
  HTMLText(html, "is different, and workload is limited rather");
  HTMLText(html, "than the number of busy times.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "For the limit busy times constraint and other resource");
  HTMLText(html, "constraints, a clash at some time only counts as one busy");
  HTMLText(html, "time.  However, for the limit workload constraint, the");
  HTMLText(html, "contributions of all clashing solution resources are");
  HTMLText(html, "included.  This is more or less unavoidable, since counting");
  HTMLText(html, "just one would raise the question of which one to choose.");
  HTMLText(html, "The choice matters when workloads per unit time differ.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The limit workload constraint has no");
  HTMLTextMono(html, "ResourceHistory");
  HTMLText(html, "attribute.  It ought to have, but the algebra to support");
  HTMLText(html, "it has not yet been done.  The very similar limit busy");
  HTMLText(html, "times constraint also has no");
  HTMLTextMono(html, "ResourceHistory");
  HTMLText(html, "attribute, but in that case a workaround is available");
  HTMLText(html, "(not in every case, but usually), namely conversion to a");
  HTMLText(html, "cluster busy times constraint.  No workaround is available");
  HTMLText(html, "for the limit workload constraint.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void EmployeeLimitActiveIntervalsSpec(HTML html, char *name)             */
/*                                                                           */
/*  Print a detailed specification of employee scheduling limit active       */
/*  intervals constraints.                                                   */
/*                                                                           */
/*****************************************************************************/

static void EmployeeLimitActiveIntervalsSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name,
    "Limit active intervals constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "The limit active intervals constraint is a whole new");
  HTMLText(html, "constraint, not present in the high school model.  In");
  HTMLText(html, "the global list of all constraints, limit active intervals");
  HTMLText(html, "constraints come after limit workload constraints.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The limit active intervals constraint has almost the same");
  HTMLText(html, "syntax as the employee scheduling cluster busy times");
  HTMLText(html, "constraint:");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "LimitActiveIntervalsConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+ResourceHistory");
  HTMLLiteralTextIndented(html,  8, "+AppliesToTimeGroup");
  HTMLLiteralTextIndented(html,  8, "TimeGroups");
  HTMLLiteralTextIndented(html,  8, "Minimum");
  HTMLLiteralTextIndented(html,  8, "Maximum");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The only syntax differences are the change of name, from");
  HTMLTextMono(html, "ClusterBusyTimesConstraint");
  HTMLText(html, "to");
  HTMLTextMonoNoBreak(html, "LimitActiveIntervalsConstraint");
  HTMLText(html, ", and the absence of");
  HTMLTextMonoNoBreak(html, "AllowZero");
  HTMLText(html, ".  Each of the child categories shown has the same syntax");
  HTMLText(html, "and the same validity rules as it does for the employee");
  HTMLText(html, "scheduling cluster busy times constraint.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "ResourceHistory");
  HTMLText(html, "and");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "child categories are not present, the cost at one");
  HTMLText(html, "point of application of this constraint (one resource)");
  HTMLText(html, "is the sum, over all active intervals");
  HTMLTextItalicNoBreak(html, "I");
  HTMLText(html, ", of the cost of the deviation of");
  HTMLTextItalicNoBreak(html, "I");
  HTMLText(html, ".  The deviation of");
  HTMLTextItalic(html, "I");
  HTMLText(html, "is the amount by which the length of");
  HTMLTextItalic(html, "I");
  HTMLText(html, "falls short of");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "or exceeds");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".  An active interval is a non-empty sequence of");
  HTMLText(html, "consecutive active time groups (consecutive with respect");
  HTMLText(html, "to the order that the time groups appear in the constraint)");
  HTMLText(html, "of maximal length.  For the meaning of `active time group',");
  HTMLText(html, "consult the specification of the employee scheduling");
  HTMLText(html, "cluster busy times constraint.");
  HTMLParagraphEnd(html);

  /* ***
  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "ResourceHistory");
  HTMLText(html, "and");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "child categories are not present, the deviation at one");
  HTMLText(html, "point of application of this constraint (one resource)");
  HTMLText(html, "is the sum, over all active intervals, of the amount by");
  HTMLText(html, "which the length of the interval falls short of");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "or exceeds");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".  An active interval is a non-empty sequence of");
  HTMLText(html, "consecutive active time groups (consecutive with respect");
  HTMLText(html, "to the order that the time groups appear in the constraint)");
  HTMLText(html, "of maximal length.  For the meaning of `active time group',");
  HTMLText(html, "consult the specification of the employee scheduling");
  HTMLText(html, "cluster busy times constraint.");
  HTMLParagraphEnd(html);
  *** */

  HTMLParagraphBegin(html);
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "ResourceHistory");
  HTMLText(html, "child category is present,");
  HTMLTextItalicNoBreak(html, "ai");
  HTMLText(html, ",");
  HTMLTextItalicNoBreak(html, "ci");
  HTMLText(html, ", and");
  HTMLTextItalic(html, "xi");
  HTMLText(html, "values are found for each resource, as described for");
  HTMLText(html, "the employee scheduling cluster busy times constraint.");
  HTMLText(html, "Then, if");
  HTMLTextItalic(html, "xi");
  HTMLText(html, "is non-zero, a virtual active interval of length");
  HTMLTextItalic(html, "xi");
  HTMLText(html, "is added just before the first time group.  Its cost");
  HTMLText(html, "is included in the total cost, and it merges with the");
  HTMLText(html, "first real active interval when that interval includes");
  HTMLText(html, "the first time group.");
  HTMLText(html, "An active interval whose last time group is the");
  HTMLText(html, "constraint's last time group has its length increased by");
  HTMLTextItalicNoBreak(html, "ci");
  HTMLText(html, ", although only when comparing with a minimum limit.");
  HTMLText(html, "Furthermore, an adjustment, which depends on");
  HTMLTextItalicNoBreak(html, "ai");
  HTMLText(html, ",");
  HTMLTextItalicNoBreak(html, "ci");
  HTMLText(html, ", and");
  HTMLTextItalicNoBreak(html, "xi");
  HTMLText(html, ", is made to any reported cost of violation, to remove any");
  HTMLText(html, "double counting.  Full details appear in");
  JumpToHistoryPaper(html);
  HTMLText(html, ".  In the literature,");
  HTMLTextItalic(html, "xi");
  HTMLText(html, "and");
  HTMLTextItalic(html, "ci");
  HTMLText(html, "are usually called the counter start value and the");
  HTMLText(html, "counter remainder value.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "When the optional");
  HTMLTextMono(html, "AppliesToTimeGroup");
  HTMLText(html, "child category is present, its effect is the same as");
  HTMLText(html, "for the employee scheduling cluster busy times constraint.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "In summary, the limit active intervals constraint is the");
  HTMLText(html, "same as the cluster busy times constraint except that");
  HTMLText(html, "the limits apply to the lengths of the active intervals");
  HTMLText(html, "instead of to the total number of active time groups, and");
  HTMLText(html, "history only affects the lengths of active intervals");
  HTMLText(html, "at the ends of the monitored range.");
  HTMLTextMono(html, "AllowZero");
  HTMLText(html, "is omitted because an active interval cannot have length 0.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The limit active intervals constraint is an exception to");
  HTMLText(html, "the rule that cost is calculated by first calculating one");
  HTMLText(html, "deviation and then applying the cost function to it.  That");
  HTMLText(html, "interacts badly with history.  Instead, as explained above,");
  HTMLText(html, "a deviation is calculated for each active interval, and");
  HTMLText(html, "the cost is the sum of the costs of those deviations.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void EmployeeLimitResourcesSpec(HTML html, char *name)                   */
/*                                                                           */
/*  Print a detailed specification of employee scheduling limit resources    */
/*  constraints.                                                             */
/*                                                                           */
/*****************************************************************************/

static void EmployeeLimitResourcesSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Limit resources constraints");

  HTMLParagraphBegin(html);
  HTMLText(html, "The limit resources constraint is a whole new constraint,");
  HTMLText(html, "not present in the high school model.  It generalizes the");
  HTMLText(html, "assign resource and prefer resources constraints.  In the");
  HTMLText(html, "global list of all constraints, limit resources constraints");
  HTMLText(html, "come after limit active intervals constraints.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The limit resources constraint has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "LimitResourcesConstraint Id");
  HTMLLiteralTextIndented(html,  8, "Name");
  HTMLLiteralTextIndented(html,  8, "Required");
  HTMLLiteralTextIndented(html,  8, "Weight");
  HTMLLiteralTextIndented(html,  8, "CostFunction");
  HTMLLiteralTextIndented(html,  8, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+ResourceGroups");
  HTMLLiteralTextIndented(html,  8, "+Resources");
  HTMLLiteralTextIndented(html,  8, "+Minimum");
  HTMLLiteralTextIndented(html,  8, "+Maximum");
  HTMLLiteralTextIndented(html,  8, "Roles");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextMono(html, "AppliesTo");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "AppliesTo");
  HTMLLiteralTextIndented(html,  8, "+EventGroups");
  HTMLLiteralTextIndented(html,  8, "+Events");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextMono(html, "EventGroups");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventGroups");
  HTMLLiteralTextIndented(html,  8, "*EventGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and");
  HTMLTextMono(html, "EventGroup");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventGroup Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextMono(html, "Events");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Events");
  HTMLLiteralTextIndented(html,  8, "*Event");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "and");
  HTMLTextMono(html, "Event");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Event Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextMono(html, "ResourceGroups");
  HTMLText(html, "and");
  HTMLTextMono(html, "Resources");
  HTMLText(html, "are as for the prefer resources constraint.");
  HTMLTextMono(html, "Maximum");
  HTMLText(html, "and");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "each have a body only (no attributes or children),");
  HTMLText(html, "containing a non-negative integer.  When both are present,");
  HTMLText(html, "the value of");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "may not exceed the value of");
  HTMLTextMonoNoBreak(html, "Maximum");
  HTMLText(html, ".");
  HTMLTextMono(html, "Roles");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Roles");
  HTMLLiteralTextIndented(html,  8, "*Role");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each");
  HTMLTextMono(html, "Role");
  HTMLText(html, "has a body only.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each");
  HTMLTextMono(html, "Event");
  HTMLText(html, "is taken to specify an event group containing just that");
  HTMLText(html, "one event.  Each event group is one point of application;");
  HTMLText(html, "the individual events of the event groups are");
  HTMLTextItalic(html, "not");
  HTMLText(html, "the points of application.  The event groups referenced");
  HTMLText(html, "may be defined as Courses.  Having event groups as points");
  HTMLText(html, "of application allows the constraint to coordinate resource");
  HTMLText(html, "assignments to a whole set of events.");
  HTMLParagraphEnd(html);

  HTMLTextBold(html, "Deviation.");
  HTMLText(html, "The deviation at one point of application of this");
  HTMLText(html, "constraint (one event group) is calculated as follows.");
  HTMLText(html, "Form the set of all solution event resources whose");
  HTMLText(html, "corresponding instance event resources lie in one of the");
  HTMLText(html, "events of the event group and have one of the given");
  HTMLText(html, "roles, and which are assigned a resource from");
  HTMLTextMono(html, "ResourceGroups");
  HTMLText(html, "or");
  HTMLTextMonoNoBreak(html, "Resources");
  HTMLText(html, ".  The deviation is the amount by which");
  HTMLText(html, "the total duration of these solution event resources");
  HTMLText(html, "falls short of");
  HTMLTextMono(html, "Minimum");
  HTMLText(html, "(if present) or exceeds");
  HTMLTextMono(html, "Maximum");
  HTMLText(html, "(if present).");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "An assign resource constraint can be expressed as a limit");
  HTMLText(html, "resources constraint, whose resources are all resources of");
  HTMLText(html, "the appropriate type, with minimum limit equal to the total");
  HTMLText(html, "duration of the event resources which are the points of");
  HTMLText(html, "application.  A prefer resources constraint can also be");
  HTMLText(html, "expressed as a limit resources constraint, whose resources");
  HTMLText(html, "are all the resources of the appropriate type which are");
  HTMLText(html, "not preferred in the prefer resources constraint, with");
  HTMLText(html, "maximum limit 0.  These conversions shed a useful light");
  HTMLText(html, "on the relationships between these constraints, but assign");
  HTMLText(html, "resource and prefer resources constraints are preferred in");
  HTMLText(html, "practice, because they treat the event resources they apply");
  HTMLText(html, "to independently, and so they are fundamentally simpler.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void EmployeeSpecFull(void)                                              */
/*                                                                           */
/*  Print a detailed specification of employee scheduling on a separate      */
/*  web page.                                                                */
/*                                                                           */
/*****************************************************************************/

void EmployeeSpecFull(void)
{
  HTML html;
  char *hd = "Employee Scheduling File Format Specification";
  html = PageBegin(hd);
  HTMLBigHeading(html, hd);

  HTMLParagraphBegin(html);
  HTMLText(html, "This page specifies an extended version of the XHSTT");
  HTMLText(html, "format called XESTT, which is used for employee");
  HTMLText(html, "scheduling.  It is very similar to");
  HTMLJump1(html, "op", "spec", NULL,
    "XHSTT, the format for high school timetabling");
  HTMLText(html, ", so only the differences between the two are given here.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLJumpInternalIndented(html, 8, "earch",
    "Employee scheduling archive files");
  HTMLJumpInternalIndented(html, 8, "eer",
    "Employee scheduling abbreviated event resources");
  HTMLJumpInternalIndented(html, 8, "ecbtm",
    "Employee scheduling cluster busy times constraints");
  HTMLJumpInternalIndented(html, 8, "elbtm",
    "Employee scheduling limit busy times constraints");
  HTMLJumpInternalIndented(html, 8, "elwm",
    "Employee scheduling limit workload constraints");
  HTMLJumpInternalIndented(html, 8, "elbim",
    "Limit active intervals constraints");
  HTMLJumpInternalIndented(html, 8, "lrm",
    "Limit resources constraints");
  HTMLParagraphEnd(html);
  HTMLHorizontalRule(html);

  EmployeeArchiveSpec(html, "earch");
  EmployeeAbbreviatedEventResources(html, "eer");
  EmployeeClusterBusyTimesSpec(html, "ecbtm");
  EmployeeLimitBusyTimesSpec(html, "elbtm");
  EmployeeLimitWorkloadSpec(html, "elwm");
  EmployeeLimitActiveIntervalsSpec(html, "elbim");
  EmployeeLimitResourcesSpec(html, "lrm");

  HTMLParagraphBegin(html);
  HTMLText(html, "Return to the ");
  HTMLJumpFront(html);
  HTMLText(html, ".");
  HTMLParagraphEnd(html);
  PageEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void SolutionGroupSpec(HTML html, char *name)                            */
/*                                                                           */
/*  Print a specification of solution groups onto html.                      */
/*                                                                           */
/*****************************************************************************/

static void SolutionGroupSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Solution Groups");

  HTMLParagraphBegin(html);
  HTMLText(html, "A");
  HTMLTextItalic(html, "solution group");
  HTMLText(html, "is a collection of solutions to the instances of the");
  HTMLText(html, "enclosing archive, constructed by the same version of");
  HTMLText(html, "the same solver.  A solution group may contain any number");
  HTMLText(html, "of solutions (including none) to each instance.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of the");
  HTMLTextMono(html, "SolutionGroups");
  HTMLText(html, "child category of");
  HTMLTextMono(html, "HighSchoolTimetableArchive");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "SolutionGroups");
  HTMLLiteralTextIndented(html,  8, "*SolutionGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of");
  HTMLTextMono(html, "SolutionGroup");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "SolutionGroup Id");
  HTMLLiteralTextIndented(html,  8, "MetaData");
  HTMLLiteralTextIndented(html,  8, "*Solution");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextMono(html, "Id");
  HTMLText(html, "identifies the solution group as usual.");
  HTMLTextMono(html, "MetaData");
  HTMLText(html, "holds information describing the provenance of the");
  HTMLText(html, "solution group.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "MetaData");
  HTMLLiteralTextIndented(html,  8, "Contributor");
  HTMLLiteralTextIndented(html,  8, "Date");
  HTMLLiteralTextIndented(html,  8, "Description");
  HTMLLiteralTextIndented(html,  8, "+Publication");
  HTMLLiteralTextIndented(html,  8, "+Remarks");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The child categories each hold plain text only.  They give");
  HTMLText(html, "the name of the person who contributed the solution group,");
  HTMLText(html, "the date it was contributed, a description that may be");
  HTMLText(html, "printed when displaying the solution group, an optional");
  HTMLText(html, "citation of a publication describing how the solution");
  HTMLText(html, "group was produced, and optional remarks.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void SolutionSpec(HTML html, char *name)                                 */
/*                                                                           */
/*  Print a specification of solutions onto html.                            */
/*                                                                           */
/*****************************************************************************/

static void SolutionSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Solutions");

  HTMLParagraphBegin(html);
  HTMLText(html, "In addition to assigning times and resources to events, a");
  HTMLText(html, "solver is expected to");
  HTMLTextItalic(html, "split");
  HTMLText(html, "some of the events into smaller pieces.");
  HTMLText(html, "This allows courses to spread their lessons through");
  HTMLText(html, "the cycle, without forcing the durations of those lessons");
  HTMLText(html, "to be fixed in advance, as would be the case if each");
  HTMLText(html, "lesson was modelled as a distinct event.  For example,");
  HTMLText(html, "a solver might split an event of duration 6 into fragments");
  HTMLText(html, "of durations 2, 2, 1, and 1.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "It is possible to imagine a solver taking an initial");
  HTMLText(html, "instance and modifying it by splitting its events and");
  HTMLText(html, "assigning times and resources to the resulting fragments.");
  HTMLText(html, "However, this documentation takes a different view.  It");
  HTMLText(html, "considers instances to be immutable.  A solution is a");
  HTMLText(html, "separate entity, derived from an instance but distinct");
  HTMLText(html, "from it, and capable of representing how the instance");
  HTMLText(html, "events are split and which times and resources are");
  HTMLText(html, "assigned to them.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "For each instance event, a solution contains some");
  HTMLTextItalic(html, "sub-events");
  HTMLText(html, "or");
  HTMLTextItalic(html, "solution events");
  HTMLText(html, "derived from that instance event.  These are the fragments");
  HTMLText(html, "referred to above.  A solution event has a duration, a");
  HTMLText(html, "workload, and a set of");
  HTMLTextItalicNoBreak(html, "solution resources");
  HTMLText(html, ", one for each event resource in the instance event it is");
  HTMLText(html, "derived from.  A solution event may be assigned a time,");
  HTMLText(html, "which is its starting time, and a solution resource may");
  HTMLText(html, "be assigned a resource.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of solutions is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Solution Reference");
  HTMLLiteralTextIndented(html,  8, "+Description");
  HTMLLiteralTextIndented(html,  8, "+RunningTime");
  HTMLLiteralTextIndented(html,  8, "+Events");
  HTMLLiteralTextIndented(html,  8, "+Report");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "here is to the instance that the solution solves.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The optional");
  HTMLTextMono(html, "Description");
  HTMLText(html, "contains text recording how the solution was created.");
  HTMLText(html, "This is particularly useful when the enclosing solution");
  HTMLText(html, "group contains several solutions to one instance.  Although");
  HTMLText(html, "the value of");
  HTMLTextMono(html, "Description");
  HTMLText(html, "is formally arbitrary, a convention is emerging to use it");
  HTMLText(html, "to record the solver parameters which produced the");
  HTMLText(html, "solution, as in this example:");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "<Description>randseed=12345678</Description>");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Multiple parameters may be separated by commas.  The");
  HTMLText(html, "metadata of the enclosing solution group describes the");
  HTMLText(html, "solver which produced all the solutions of the solution");
  HTMLText(html, "group; the solution");
  HTMLTextMono(html, "Description");
  HTMLText(html, "describes only what is");
  HTMLText(html, "distinctive about this particular solution.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The optional");
  HTMLTextMono(html, "RunningTime");
  HTMLText(html, "contains the wall clock running time of the process");
  HTMLText(html, "which produced this solution, in seconds, to the");
  HTMLText(html, "nearest one-tenth of one second, as in this example:");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "<RunningTime>356.7</RunningTime>");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The hardware and software platform is not specified,");
  HTMLText(html, "beyond an expectation that the hardware should be");
  HTMLText(html, "widely available and reasonably affordable.  There is");
  HTMLText(html, "no fixed limit to the number of parallel processes.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of");
  HTMLTextMono(html, "Events");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Events");
  HTMLLiteralTextIndented(html,  8, "*Event");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each");
  HTMLTextMono(html, "Event");
  HTMLText(html, "category defines one solution event and optionally assigns");
  HTMLText(html, "a starting time and resources to it, using syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Event Reference");
  HTMLLiteralTextIndented(html,  8, "+Duration");
  HTMLLiteralTextIndented(html,  8, "+Time");
  HTMLLiteralTextIndented(html,  8, "+Resources");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "attribute identifies the instance event that this");
  HTMLText(html, "solution event is derived from.  The optional");
  HTMLTextMono(html, "Duration");
  HTMLText(html, "child category defines the duration of the solution event.");
  HTMLText(html, "It has no attributes and no child categories, and its body");
  HTMLText(html, "consists of a single integer whose value must be at least");
  HTMLText(html, "1.  If omitted, the duration is taken to be the duration");
  HTMLText(html, "of the corresponding instance event.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The");
  HTMLTextMono(html, "Time");
  HTMLText(html, "child category, when present, assigns a starting time");
  HTMLText(html, "to the solution event, using syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Time Reference");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "to reference one of the times of the instance.  Assigning");
  HTMLText(html, "a time is not compulsory, but omitting to do so always");
  HTMLText(html, "attracts a cost (a violation of an assign times constraint)");
  HTMLText(html, "in practice.  If the instance event has a preassigned time,");
  HTMLText(html, "then any assigned time must be the same as the preassigned");
  HTMLText(html, "time.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The assigned time and duration may not cause the solution");
  HTMLText(html, "event to run past the last time of the instance.  The");
  HTMLText(html, "entire solution will be rejected in that case.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The");
  HTMLTextMono(html, "Resources");
  HTMLText(html, "child category, when present, assigns resources to the");
  HTMLText(html, "solution resources of the solution event using syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resources");
  HTMLLiteralTextIndented(html,  8, "*Resource");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "Resource");
  HTMLText(html, "has syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resource Reference");
  HTMLLiteralTextIndented(html,  8, "Role");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "This specifies that the solution resource whose");
  HTMLText(html, "corresponding event resource has the given");
  HTMLTextMono(html, "Role");
  HTMLText(html, "is to be assigned the referenced resource.  (It does not");
  HTMLTextItalic(html, "define");
  HTMLText(html, "the solution resource; a solution event always contains");
  HTMLText(html, "one solution resource for each event resource of the");
  HTMLText(html, "corresponding instance event.)  The");
  HTMLText(html, "assigned resource must have the same type as the event");
  HTMLText(html, "resource, and if the event resource has a preassigned");
  HTMLText(html, "resource, then the assigned resource must be the same as");
  HTMLText(html, "the preassigned resource.  It is illegal to mention the");
  HTMLText(html, "same role twice within one solution event.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "A resource may be preassigned or assigned more than");
  HTMLText(html, "once to a single solution event in different solution");
  HTMLText(html, "resources.  However, that will yield a penalty if the");
  HTMLText(html, "resource in question is a point of application of an");
  HTMLText(html, "AvoidClashes constraint.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Here is an example of a solution event:");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "<Event Reference=\"x7C_English_1\">");
  HTMLLiteralTextIndented(html,  8, "<Duration>2</Duration>");
  HTMLLiteralTextIndented(html,  8, "<Time Reference=\"Wed5\"/>");
  HTMLLiteralTextIndented(html,  8, "<Resources>");
  HTMLLiteralTextIndented(html, 12, "<Resource Reference=\"English05\">");
  HTMLLiteralTextIndented(html, 16, "<Role>1</Role>");
  HTMLLiteralTextIndented(html, 12, "</Resource>");
  HTMLLiteralTextIndented(html, 12, "<Resource Reference=\"r42\">");
  HTMLLiteralTextIndented(html, 16, "<Role>2</Role>");
  HTMLLiteralTextIndented(html, 12, "</Resource>");
  HTMLLiteralTextIndented(html,  8, "</Resources>");
  HTMLLiteralTextIndented(html,  4, "</Event>");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "It is derived from the instance event with identifier");
  HTMLTextMonoNoBreak(html, "x7C_English_1");
  HTMLText(html, ", its duration is 2, and its starting time is");
  HTMLTextMonoNoBreak(html, "Wed5");
  HTMLText(html, ".  It assigns resources");
  HTMLTextMono(html, "English05");
  HTMLText(html, "and");
  HTMLTextMono(html, "r42");
  HTMLText(html, "to two of the solution resources of the solution event.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "When a solution is read from an XML file, some");
  HTMLText(html, "elements are added to it automatically, as follows:");
  HTMLNumberedListBegin(html);

  HTMLListItemBegin(html);
  HTMLText(html, "For each instance event for which there are no");
  HTMLText(html, "corresponding solution events in the solution, one");
  HTMLText(html, "solution event corresponding to the instance event is");
  HTMLText(html, "added, whose duration is the duration of the instance");
  HTMLText(html, "event.  Initially this solution event has no assigned times");
  HTMLText(html, "or resources, but that may be changed by later steps.");
  HTMLListItemEnd(html);

  HTMLListItemBegin(html);
  HTMLText(html, "If now there is an instance event such that the total");
  HTMLText(html, "duration of the solution events derived from that instance");
  HTMLText(html, "event is not equal to the duration of the instance event,");
  HTMLText(html, "it is an error.");
  HTMLListItemEnd(html);

  HTMLListItemBegin(html);
  HTMLText(html, "For each solution event, whether read from the file");
  HTMLText(html, "or inserted automatically by the first step, if the");
  HTMLText(html, "solution event has no assigned time and the corresponding");
  HTMLText(html, "instance event has a preassigned time, then assign that");
  HTMLText(html, "preassigned time to the solution event.");
  HTMLListItemEnd(html);

  HTMLListItemBegin(html);
  HTMLText(html, "For each solution resource of each solution event, whether");
  HTMLText(html, "read from the file or inserted automatically by the first");
  HTMLText(html, "step, if the solution resource has no assigned resource");
  HTMLText(html, "and the corresponding event resource has a preassigned");
  HTMLText(html, "resource, then assign that preassigned resource to the");
  HTMLText(html, "solution resource.");
  HTMLListItemEnd(html);

  HTMLNumberedListEnd(html);
  HTMLText(html, "What this mainly amounts to is that it is acceptable");
  HTMLText(html, "to omit preassigned times and resources from a");
  HTMLText(html, "solution, since they will be added automatically.");
  HTMLText(html, "It also guarantees that the total duration of the");
  HTMLText(html, "solution events derived from an instance event equals");
  HTMLText(html, "the duration of that instance event, avoiding the need");
  HTMLText(html, "for a constraint penalizing solutions where this is not");
  HTMLText(html, "the case.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void SolutionReportSpec(HTML html, char *name)                           */
/*                                                                           */
/*  Print a specification of solution reports onto html.                     */
/*                                                                           */
/*****************************************************************************/

static void SolutionReportSpec(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Reports");

  HTMLParagraphBegin(html);
  HTMLText(html, "The optional");
  HTMLTextMono(html, "Report");
  HTMLText(html, "child category of");
  HTMLTextMono(html, "Solution");
  HTMLText(html, "is a report containing the total infeasibility");
  HTMLText(html, "value and objective function value of the solution,");
  HTMLText(html, "plus a detailed list of all the constraint violations");
  HTMLText(html, "that contribute to these values.  HSEval has a function");
  HTMLText(html, "that adds a report to each solution of the archive it is");
  HTMLText(html, "given, clearing away any existing reports.  It is probably");
  HTMLText(html, "best to omit reports when generating solutions, leaving");
  HTMLText(html, "it to HSEval to add independent and trusted reports, but");
  HTMLText(html, "reports are not prohibited.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The syntax of reports is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Report");
  HTMLLiteralTextIndented(html,  8, "InfeasibilityValue");
  HTMLLiteralTextIndented(html,  8, "ObjectiveValue");
  HTMLLiteralTextIndented(html,  8, "+Resources");
  HTMLLiteralTextIndented(html,  8, "+Events");
  HTMLLiteralTextIndented(html,  8, "+EventGroups");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTextMono(html, "InfeasibilityValue");
  HTMLText(html, "has no attributes and no child categories.  Its body");
  HTMLText(html, "consists entirely of a single non-negative integer");
  HTMLText(html, "whose value is the total cost of all violations of");
  HTMLText(html, "required (that is, hard) constraints.");
  HTMLTextMono(html, "ObjectiveValue");
  HTMLText(html, "is similar, except that it contains the total cost of all");
  HTMLText(html, "violations of non-required (that is, soft) constraints.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The optional");
  HTMLTextMono(html, "Resources");
  HTMLText(html, "child category of");
  HTMLTextMono(html, "Report");
  HTMLText(html, "reports violations of constraints for which");
  HTMLText(html, "one point of application is one resource, grouped by");
  HTMLText(html, "resource.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resources");
  HTMLLiteralTextIndented(html,  8, "*Resource");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each");
  HTMLTextMono(html, "Resource");
  HTMLText(html, "category reports constraint violations for one");
  HTMLText(html, "resource, using syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Resource Reference");
  HTMLLiteralTextIndented(html,  8, "*Constraint");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "identifies the resource, and the syntax of");
  HTMLTextMono(html, "Constraint");
  HTMLText(html, "is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Constraint Reference");
  HTMLLiteralTextIndented(html,  8, "Cost");
  HTMLLiteralTextIndented(html,  8, "+Description");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The reference here is to the violated constraint, which");
  HTMLText(html, "must have the enclosing resource as one point of");
  HTMLText(html, "application.  The");
  HTMLTextMono(html, "Cost");
  HTMLText(html, "child category has no attributes or child categories,");
  HTMLText(html, "just a body whose value is an integer giving the cost");
  HTMLText(html, "of the violation of the referenced constraint at the");
  HTMLText(html, "enclosing resource.  Since only violations are reported,");
  HTMLText(html, "this cost should have value at least 1.  The optional");
  HTMLTextMono(html, "Description");
  HTMLText(html, "child category contains a description of the violation");
  HTMLText(html, "which could be printed when displaying the report.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Here is an example of a report for one resource:");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "<Resource Reference=\"Languages03\">");
  HTMLLiteralTextIndented(html,  8, "<Constraint Reference=\"LimitBusyTimes_3\">");
  HTMLLiteralTextIndented(html, 12, "<Cost>2</Cost>");
  HTMLLiteralTextIndented(html, 12, "<Description>Tuesday, Friday</Description>");
  HTMLLiteralTextIndented(html,  8, "</Constraint>");
  HTMLLiteralTextIndented(html,  4, "</Resource>");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The timetable of teacher resource");
  HTMLTextMono(html, "Languages03");
  HTMLText(html, "violates a limit busy times constraint applicable to");
  HTMLText(html, "that resource.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The optional");
  HTMLTextMono(html, "Events");
  HTMLText(html, "child category of");
  HTMLTextMono(html, "Report");
  HTMLText(html, "reports violations of constraints for which one point");
  HTMLText(html, "of application is one event.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Events");
  HTMLLiteralTextIndented(html,  8, "*Event");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each");
  HTMLTextMono(html, "Event");
  HTMLText(html, "category reports constraint violations for one");
  HTMLText(html, "event, using syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "Event Reference");
  HTMLLiteralTextIndented(html,  8, "*Constraint");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "identifies the event, and");
  HTMLTextMono(html, "Constraint");
  HTMLText(html, "is as before.  For example,");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "<Event Reference=\"x8S_Science_1\">");
  HTMLLiteralTextIndented(html,  8,
    "<Constraint Reference=\"SplitEventsConstraint_2\">");
  HTMLLiteralTextIndented(html, 12, "<Cost>1</Cost>");
  HTMLLiteralTextIndented(html,  8, "</Constraint>");
  HTMLLiteralTextIndented(html,  8,
    "<Constraint Reference=\"DistributeSplitEventsConstraint_2\">");
  HTMLLiteralTextIndented(html, 12, "<Cost>2</Cost>");
  HTMLLiteralTextIndented(html,  8, "</Constraint>");
  HTMLLiteralTextIndented(html,  8,
    "<Constraint Reference=\"DistributeSplitEventsConstraint_7\">");
  HTMLLiteralTextIndented(html, 12, "<Cost>1</Cost>");
  HTMLLiteralTextIndented(html,  8, "</Constraint>");
  HTMLLiteralTextIndented(html,  8,
    "<Constraint Reference=\"AssignResourceConstraint_2\">");
  HTMLLiteralTextIndented(html, 12, "<Cost>1</Cost>");
  HTMLLiteralTextIndented(html,  8, "</Constraint>");
  HTMLLiteralTextIndented(html,  4, "</Event>");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "reports that the timetable of event");
  HTMLTextMono(html, "x8S_Science_1");
  HTMLText(html, "violates several split events and distribute split events");
  HTMLText(html, "constraints, and that not all the solution resources of its");
  HTMLText(html, "solution events received resource assignments.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The optional");
  HTMLTextMono(html, "EventGroups");
  HTMLText(html, "child category of");
  HTMLTextMono(html, "Report");
  HTMLText(html, "reports violations of constraints for which one point");
  HTMLText(html, "of application is one event group.  Its syntax is");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventGroups");
  HTMLLiteralTextIndented(html,  8, "*EventGroup");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each");
  HTMLTextMono(html, "EventGroup");
  HTMLText(html, "category reports constraint violations for one");
  HTMLText(html, "event group, using syntax");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "EventGroup Reference");
  HTMLLiteralTextIndented(html,  8, "*Constraint");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "where");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "identifies the event group, and");
  HTMLTextMono(html, "Constraint");
  HTMLText(html, "is as before.  For example,");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLMonoBegin(html);
  HTMLLiteralTextIndented(html,  4, "<EventGroup Reference=\"x9_English_4\">");
  HTMLLiteralTextIndented(html,  8,
    "<Constraint Reference=\"SpreadEventsConstraint_1\">");
  HTMLLiteralTextIndented(html, 12, "<Cost>1</Cost>");
  HTMLLiteralTextIndented(html,  8, "</Constraint>");
  HTMLLiteralTextIndented(html,  8,
    "<Constraint Reference=\"AvoidSplitAssignmentsConstraint_Soft_1\">");
  HTMLLiteralTextIndented(html, 12, "<Cost>10</Cost>");
  HTMLLiteralTextIndented(html,  8, "</Constraint>");
  HTMLLiteralTextIndented(html,  4, "</EventGroup>");
  HTMLMonoEnd(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "reports that the solution events of the events of event group");
  HTMLTextMono(html, "x9_English_4");
  HTMLText(html, "are imperfectly spread through the cycle, and also that");
  HTMLText(html, "there is a split assignment within the event group.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void SpecMain(void)                                                      */
/*                                                                           */
/*  Print the main section of the specification of the XML file format.      */
/*                                                                           */
/*****************************************************************************/

void SpecMain(void)
{
  HTML html;
  html = PageBegin("High School Timetable Data Format Specification");
  HTMLBigHeading(html, "High School Timetable Data Format Specification");

  HTMLParagraphBegin(html);
  HTMLText(html, "This page contains the official specification of XHSTT, the");
  HTMLText(html, "format of the high school timetable files used by HSEval.");
  HTMLText(html, "(For employee scheduling,");
  HTMLJump2(html, "op", "spec", "part", "employee", NULL, "click here");
  HTMLText(html, ".)");
  HTMLText(html, "Each file contains one archive, specified in XML as");
  HTMLText(html, "defined below.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLJumpInternalIndented(html,  4, "archives", "Archives");
  HTMLJumpInternalIndented(html,  8, "instances", "Instances");
  HTMLJumpInternalIndented(html, 12, "times", "Times");
  HTMLJumpInternalIndented(html, 12, "resources", "Resources");
  HTMLJumpInternalIndented(html, 12, "events", "Events");
  HTMLJumpInternalIndented(html, 12, "constraints", "Constraints");
  HTMLJumpInternalIndented(html,  8, "solution_groups", "Solution Groups");
  HTMLJumpInternalIndented(html, 12, "solutions", "Solutions");
  HTMLJumpInternalIndented(html, 16, "reports", "Reports");
  HTMLHSpace(html, 4);
  HTMLJump2(html, "op", "spec", "part", "glossary", NULL, "Glossary");
  /* ***
  HTMLNewLine(html);
  HTMLHSpace(html, 4);
  HTMLJump2(html, "op", "spec", "part", "avail ability", NULL,
    "How HSEval calculates avai lability");
  HTMLNewLine(html);
  *** */
  HTMLParagraphEnd(html);
  HTMLHorizontalRule(html);

  ArchiveSpec(html, "archives");
  InstanceSpec(html, "instances");
  TimeGroupSpec(html, "times");
  ResourceGroupSpec(html, "resources");
  EventGroupSpec(html, "events");
  ConstraintSpec(html, "constraints");
  SolutionGroupSpec(html, "solution_groups");
  SolutionSpec(html, "solutions");
  SolutionReportSpec(html, "reports");

  HTMLParagraphBegin(html);
  HTMLText(html, "Return to the ");
  HTMLJumpFront(html);
  HTMLText(html, ".");
  HTMLParagraphEnd(html);
  PageEnd(html);
}
