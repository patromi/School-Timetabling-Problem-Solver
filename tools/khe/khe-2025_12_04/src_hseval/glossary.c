
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
/*  FILE:         glossary.c                                                 */
/*  MODULE:       Glossary web page.                                         */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryArchive(HTML html, char *name)                              */
/*                                                                           */
/*  Print the glossary definition of an Archive.                             */
/*                                                                           */
/*****************************************************************************/

static void GlossaryArchive(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Archive");

  HTMLParagraphBegin(html);
  HTMLText(html, "An archive is a collection of instances of the high");
  HTMLText(html, "school timetable problem, together with zero or more");
  HTMLText(html, "solution groups containing solutions to those instances.");
  HTMLText(html, "For more information, consult the main entry");
  HTMLJump2(html, "op", "spec", "part", "main", "archives",
    "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryConstraint(HTML html, char *name)                           */
/*                                                                           */
/*  Print the glossary definition of a constraint.                           */
/*                                                                           */
/*****************************************************************************/

static void GlossaryConstraint(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Constraint");

  HTMLParagraphBegin(html);
  HTMLText(html, "A constraint is a condition that a solution should");
  HTMLText(html, "obey; if it doesn't, then a cost is incurred.");
  HTMLText(html, "For more information, consult the");
  HTMLJump2(html, "op", "spec", "part", "constraints", NULL, "constraints");
  HTMLText(html, " page.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryCost(HTML html, char *name)                                 */
/*                                                                           */
/*  Print the glossary definition of a cost.                                 */
/*                                                                           */
/*****************************************************************************/

static void GlossaryCost(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Cost");

  HTMLParagraphBegin(html);
  HTMLText(html, "A cost is a penalty incurred for violating one or more");
  HTMLText(html, "constraints.  The total cost of all hard constraint");
  HTMLText(html, "violations is the");
  HTMLJumpInternal(html, "infeasibility_value", "infeasibility value");
  HTMLText(html, ", and the total cost of all soft constraint violations");
  HTMLText(html, "is the");
  HTMLJumpInternal(html, "objective_value", "objective value");
  HTMLText(html, ".  The aim of a solver should be to produce a solution");
  HTMLText(html, "which minimizes the infeasibility value as its first");
  HTMLText(html, "priority, and the objective value as its second priority.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryCostFunction(HTML html, char *name)                         */
/*                                                                           */
/*  Print the glossary definition of a cost function.                        */
/*                                                                           */
/*****************************************************************************/

static void GlossaryCostFunction(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Cost function");

  HTMLParagraphBegin(html);
  HTMLText(html, "A cost function is a child category of a constraint");
  HTMLText(html, "that determines which function is applied to the");
  HTMLText(html, "deviations before multiplying by the weight to give");
  HTMLText(html, "a cost.  For more information, consult the");
  HTMLJump2(html, "op", "spec", "part", "constraints", NULL, "constraints");
  HTMLText(html, " page.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryCourse(HTML html, char *name)                               */
/*                                                                           */
/*  Print the glossary definition of a workload.                             */
/*                                                                           */
/*****************************************************************************/

static void GlossaryCourse(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Course");

  HTMLParagraphBegin(html);
  HTMLText(html, "A course is a set of events.  As far as HSEval is");
  HTMLText(html, "concerned it is just an alternative name for an");
  HTMLJumpInternal(html, "event_group", "event group");
  HTMLText(html, ", but other software might treat it differently, for");
  HTMLText(html, "example when displaying timetables.  The intention is");
  HTMLText(html, "that the set of events of one course should constitute a");
  HTMLText(html, "course of study in one subject for one group of students.");
  HTMLText(html, "For more information, consult the main entry for Events");
  HTMLJump2(html, "op", "spec", "part", "main", "events", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryDay(HTML html, char *name)                                  */
/*                                                                           */
/*  Print the glossary definition of a day.                                  */
/*                                                                           */
/*****************************************************************************/

static void GlossaryDay(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Day");

  HTMLParagraphBegin(html);
  HTMLText(html, "A day is a set of times.  As far as HSEval is");
  HTMLText(html, "concerned it is just an alternative name for a");
  HTMLJumpInternal(html, "time_group", "time group");
  HTMLText(html, ", but other software might treat it differently,");
  HTMLText(html, "for example when displaying timetables.  The intention");
  HTMLText(html, "is that all the times of each day should lie in a");
  HTMLText(html, "common");
  HTMLTextMono(html, "Day");
  HTMLText(html, "category.");
  HTMLText(html, "For more information, consult the main entry for Times");
  HTMLJump2(html, "op", "spec", "part", "main", "times", "here");
  HTMLText(html, ".");
  HTMLText(html, "See also");
  HTMLJumpInternal(html, "week", "Week");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryDuration(HTML html, char *name)                             */
/*                                                                           */
/*  Print the glossary definition of a duration.                             */
/*                                                                           */
/*****************************************************************************/

static void GlossaryDuration(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Duration");

  HTMLParagraphBegin(html);
  HTMLText(html, "The duration of an instance event or solution event");
  HTMLText(html, "is the number of times that it occupies.");
  HTMLText(html, "For more information, consult the main entry for events");
  HTMLJump2(html, "op", "spec", "part", "main", "events", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryEvent(HTML html, char *name)                                */
/*                                                                           */
/*  Print the glossary definition of an event.                               */
/*                                                                           */
/*****************************************************************************/

static void GlossaryEvent(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Event");

  HTMLParagraphBegin(html);
  HTMLText(html, "An event (also called an instance event) is a meeting");
  HTMLText(html, "between resources.  It specifies that the resources");
  HTMLText(html, "should meet together for a given number of times.");
  HTMLText(html, "For more information, consult the main entry");
  HTMLJump2(html, "op", "spec", "part", "main", "events", "here");
  HTMLText(html, ".  For the distinction between instance events and");
  HTMLText(html, "solution events, consult the specification of solutions");
  HTMLJump2(html, "op", "spec", "part", "main", "solutions", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryEventGroup(HTML html, char *name)                           */
/*                                                                           */
/*  Print the glossary definition of an event group.                         */
/*                                                                           */
/*****************************************************************************/

static void GlossaryEventGroup(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Event group");

  HTMLParagraphBegin(html);
  HTMLText(html, "An event group is a set of events.");
  HTMLText(html, "For more information, consult the main entry for events");
  HTMLJump2(html, "op", "spec", "part", "main", "events", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryEventResource(HTML html, char *name)                        */
/*                                                                           */
/*  Print the glossary definition of an event resource.                      */
/*                                                                           */
/*****************************************************************************/

static void GlossaryEventResource(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Event resource");

  HTMLParagraphBegin(html);
  HTMLText(html, "An event resource is a component of an event to which");
  HTMLText(html, "one resource of a particular type may be assigned.");
  HTMLText(html, "For more information, consult the main entry for events");
  HTMLJump2(html, "op", "spec", "part", "main", "events", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryIdentifier(HTML html, char *name)                           */
/*                                                                           */
/*  Print the glossary definition of an Identifier.                          */
/*                                                                           */
/*****************************************************************************/

static void GlossaryIdentifier(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Identifier");

  HTMLParagraphBegin(html);
  HTMLText(html, "An identifier is a string used to refer to one part of");
  HTMLText(html, "the file from another part.  It is defined by an");
  HTMLTextMono(html, "Id");
  HTMLText(html, "attribute of the category it identifies, and referenced");
  HTMLText(html, "by");
  HTMLTextMono(html, "Reference");
  HTMLText(html, "attributes elsewhere in the file.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Where there is an");
  HTMLTextMono(html, "Id");
  HTMLText(html, "attribute there is often a");
  HTMLTextMono(html, "Name");
  HTMLText(html, "child category containing just text.  The identifier");
  HTMLText(html, "is used to reference its category, while the name is");
  HTMLText(html, "used when displaying the category in human-readable form.");
  HTMLText(html, "As an exception, instances are often referred to by their");
  HTMLText(html, "identifier rather than name, especially in tables where");
  HTMLText(html, "space is limited.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "It is usual in XML for the scope of an identifier");
  HTMLText(html, "to be the entire file, and for different");
  HTMLTextMono(html, "Id");
  HTMLText(html, "attributes to define distinct identifiers.  However, that");
  HTMLText(html, "would make it awkward to bring instances together from");
  HTMLText(html, "different sources, so a different rule is used here.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "The identifiers that label the instances of an archive");
  HTMLText(html, "must be distinct from each other.  They are referenced");
  HTMLText(html, "only at the start of solutions, where the references are");
  HTMLText(html, "known to be to instances, so they do not have to be");
  HTMLText(html, "distinct from other identifiers.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Within an instance, the kinds of entities that have");
  HTMLText(html, "identifiers are:  time groups (including days and weeks),");
  HTMLText(html, "times, resource types, resource groups, resources, event");
  HTMLText(html, "groups (including courses), events, and constraints.");
  HTMLText(html, "Identifiers that label entities of the same kind within");
  HTMLText(html, "the same instance must be distinct from each other, but");
  HTMLText(html, "they need not be distinct from identifiers that label");
  HTMLText(html, "entities of other kinds, or instances, or entities within");
  HTMLText(html, "other instances.  Resources of different resource types");
  HTMLText(html, "are considered to be entities of the same kind.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Each identifier defined within an instance is visible");
  HTMLText(html, "throughout its instance and throughout the solutions");
  HTMLText(html, "of that instance, but not elsewhere.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryName(HTML html, char *name)                                 */
/*                                                                           */
/*  Print the glossary definition of a Name.                                 */
/*                                                                           */
/*****************************************************************************/

static void GlossaryName(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Name");

  HTMLParagraphBegin(html);
  HTMLText(html, "A name is some text used to identify a category when");
  HTMLText(html, "displaying it in human-readable form.  See also");
  HTMLJumpInternal(html, "id", "identifier");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryInfeasibilityValue(HTML html, char *name)                   */
/*                                                                           */
/*  Print the glossary definition of an infeasibility value.                 */
/*                                                                           */
/*****************************************************************************/

static void GlossaryInfeasibilityValue(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Infeasibility value");

  HTMLParagraphBegin(html);
  HTMLText(html, "The infeasibility value of a solution is the total");
  HTMLText(html, "cost of all hard constraint violations incurred by");
  HTMLText(html, "that solution.");
  HTMLText(html, "See also");
  HTMLJumpInternal(html, "objective_value", "objective value");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryInstance(HTML html, char *name)                             */
/*                                                                           */
/*  Print the glossary definition of an instance.                            */
/*                                                                           */
/*****************************************************************************/

static void GlossaryInstance(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Instance");

  HTMLParagraphBegin(html);
  HTMLText(html, "An instance is one occurrence of the high school");
  HTMLText(html, "timetable problem, for a particular school in a");
  HTMLText(html, "particular year (or semester, etc.).");
  HTMLText(html, "For more information, consult the main entry");
  HTMLJump2(html, "op", "spec", "part", "main", "instances", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryObjectiveValue(HTML html, char *name)                       */
/*                                                                           */
/*  Print the glossary definition of an objective value.                     */
/*                                                                           */
/*****************************************************************************/

static void GlossaryObjectiveValue(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Objective value");

  HTMLParagraphBegin(html);
  HTMLText(html, "The objective value of a solution is the total");
  HTMLText(html, "cost of all soft constraint violations incurred by");
  HTMLText(html, "that solution.");
  HTMLText(html, "See also");
  HTMLJumpInternal(html, "infeasibility_value", "infeasibility value");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryResource(HTML html, char *name)                             */
/*                                                                           */
/*  Print the glossary definition of a resource.                             */
/*                                                                           */
/*****************************************************************************/

static void GlossaryResource(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Resource");

  HTMLParagraphBegin(html);
  HTMLText(html, "A resource is an entity, typically a teacher, a room, a");
  HTMLText(html, "student, or a class (group of students), which attends");
  HTMLText(html, "events.  For more information, consult the main entry");
  HTMLJump2(html, "op", "spec", "part", "main", "resources", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryResourceGroup(HTML html, char *name)                        */
/*                                                                           */
/*  Print the glossary definition of a resource group.                       */
/*                                                                           */
/*****************************************************************************/

static void GlossaryResourceGroup(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Resource group");

  HTMLParagraphBegin(html);
  HTMLText(html, "A resource group is a set of resources of the same type.");
  HTMLText(html, "For more information, consult the main entry for resources");
  HTMLJump2(html, "op", "spec", "part", "main", "resources", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryResourceType(HTML html, char *name)                         */
/*                                                                           */
/*  Print the glossary definition of a resource type.                        */
/*                                                                           */
/*****************************************************************************/

static void GlossaryResourceType(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Resource type");

  HTMLParagraphBegin(html);
  HTMLText(html, "Resource types supply a basic classification of resources:");
  HTMLText(html, "each resource has exactly one resource type.  The most");
  HTMLText(html, "frequently encountered resource types are");
  HTMLJumpInternal(html, "teacher", "Teacher");
  HTMLText(html, ",");
  HTMLJumpInternal(html, "room", "Room");
  HTMLText(html, ",");
  HTMLJumpInternal(html, "class", "Class");
  HTMLText(html, ", and");
  HTMLJumpInternal(html, "student", "Student");
  HTMLText(html, ", but arbitrary resource types may be");
  HTMLText(html, "defined.  For more information, consult the main entry for");
  HTMLJump2(html, "op", "spec", "part", "main", "resources", "resources");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossarySolution(HTML html, char *name)                             */
/*                                                                           */
/*  Print the glossary definition of a Solution.                             */
/*                                                                           */
/*****************************************************************************/

static void GlossarySolution(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Solution");

  HTMLParagraphBegin(html);
  HTMLText(html, "A solution of an instance is a description of how");
  HTMLText(html, "the events of that instance should be split into");
  HTMLText(html, "fragments and assigned times and resources so as to");
  HTMLText(html, "produce a timetable.");
  HTMLText(html, "For more information, consult the main entry");
  HTMLJump2(html, "op", "spec", "part", "main", "solutions", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossarySolutionEvent(HTML html, char *name)                        */
/*                                                                           */
/*  Print the glossary definition of a solution event.                       */
/*                                                                           */
/*****************************************************************************/

static void GlossarySolutionEvent(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Solution event");

  HTMLParagraphBegin(html);
  HTMLText(html, "A solution event, also known as a sub-event, is a");
  HTMLText(html, "component of a solution, representing one fragment of");
  HTMLText(html, "an instance event.");
  HTMLText(html, "For more information, including an explanation of the");
  HTMLText(html, "difference between solution events and instance events,");
  HTMLText(html, "consult the main entry for solutions");
  HTMLJump2(html, "op", "spec", "part", "main", "solutions", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossarySolutionGroup(HTML html, char *name)                        */
/*                                                                           */
/*  Print the glossary definition of an Archive.                             */
/*                                                                           */
/*****************************************************************************/

static void GlossarySolutionGroup(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Solution group");

  HTMLParagraphBegin(html);
  HTMLText(html, "A solution group is a collection of solutions to some");
  HTMLText(html, "instances of the high school timetabling problem.  It must");
  HTMLText(html, "lie within an archive, and the instances its solutions");
  HTMLText(html, "solve are the instances of that archive.");
  HTMLText(html, "For more information, consult the main entry");
  HTMLJump2(html, "op", "spec", "part", "main", "solution_groups", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossarySolutionResource(HTML html, char *name)                     */
/*                                                                           */
/*  Print the glossary definition of a solution resource.                    */
/*                                                                           */
/*****************************************************************************/

static void GlossarySolutionResource(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Solution resource");

  HTMLParagraphBegin(html);
  HTMLText(html, "A solution resource is a component of a solution event,");
  HTMLText(html, "capable of being assigned a resource.");
  HTMLText(html, "For more information, consult the main entry for solutions");
  HTMLJump2(html, "op", "spec", "part", "main", "solutions", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryReport(HTML html, char *name)                               */
/*                                                                           */
/*  Print the glossary definition of a report.                               */
/*                                                                           */
/*****************************************************************************/

static void GlossaryReport(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Report");

  HTMLParagraphBegin(html);
  HTMLText(html, "A report is an optional section of a solution,");
  HTMLText(html, "giving the");
  HTMLJumpInternal(html, "infeasibility_value", "infeasibility value");
  HTMLText(html, " and the");
  HTMLJumpInternal(html, "objective_value", "objective value");
  HTMLText(html, " of the solution, as well as a list of the constraint");
  HTMLText(html, "violations that contributed to these costs.");
  HTMLText(html, "For more information, consult the main entry for");
  HTMLText(html, "solutions");
  HTMLJump2(html, "op", "spec", "part", "main", "solutions", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryRequired(HTML html, char *name)                             */
/*                                                                           */
/*  Print the glossary definition of Required.                               */
/*                                                                           */
/*****************************************************************************/

static void GlossaryRequired(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Required");

  HTMLParagraphBegin(html);
  HTMLText(html, "A boolean child category of a constraint which determines");
  HTMLText(html, "whether the constraint is hard or soft.");
  HTMLText(html, "For more information, consult the");
  HTMLJump2(html, "op", "spec", "part", "constraints",NULL,"Constraints page");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryRole(HTML html, char *name)                                 */
/*                                                                           */
/*  Print the glossary definition of a role.                                 */
/*                                                                           */
/*****************************************************************************/

static void GlossaryRole(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Role");

  HTMLParagraphBegin(html);
  HTMLText(html, "A role is some text, typically in the form of an");
  HTMLText(html, "integer or identifier, which labels an");
  HTMLJump2(html, "op", "spec", "part", "glossary", "event_resource",
    "event resource");
  HTMLText(html, " so that it can be referenced from elsewhere in the");
  HTMLText(html, "file.  A role must be unique within the enclosing event,");
  HTMLText(html, "but many events may and usually do utilize the same roles.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryRoom(HTML html, char *name)                                 */
/*                                                                           */
/*  Print the glossary definition of Room.                                   */
/*                                                                           */
/*****************************************************************************/

static void GlossaryRoom(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Room");

  HTMLParagraphBegin(html);
  HTMLText(html, "A recommended name for a");
  HTMLJumpInternal(html, "resource_type", "resource type");
  HTMLText(html, " whose resources represent rooms, or (more generally)");
  HTMLText(html, "the locations where events occur.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryClass(HTML html, char *name)                                */
/*                                                                           */
/*  Print the glossary definition of Student.                                */
/*                                                                           */
/*****************************************************************************/

static void GlossaryClass(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Class");

  HTMLParagraphBegin(html);
  HTMLText(html, "A recommended name for a");
  HTMLJumpInternal(html, "resource_type", "resource type");
  HTMLText(html, " whose resources each represent one group of students who");
  HTMLText(html, "attend events together, at least for the most part.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryStudent(HTML html, char *name)                              */
/*                                                                           */
/*  Print the glossary definition of Student.                                */
/*                                                                           */
/*****************************************************************************/

static void GlossaryStudent(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Student");

  HTMLParagraphBegin(html);
  HTMLText(html, "A recommended name for a");
  HTMLJumpInternal(html, "resource_type", "resource type");
  HTMLText(html, " whose resources represent individual students.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryTeacher(HTML html, char *name)                              */
/*                                                                           */
/*  Print the glossary definition of Teacher.                                */
/*                                                                           */
/*****************************************************************************/

static void GlossaryTeacher(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Teacher");

  HTMLParagraphBegin(html);
  HTMLText(html, "A recommended name for a");
  HTMLJumpInternal(html, "resource_type", "resource type");
  HTMLText(html, " whose resources represent teachers.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryTime(HTML html, char *name)                                 */
/*                                                                           */
/*  Print the glossary definition of a time.                                 */
/*                                                                           */
/*****************************************************************************/

static void GlossaryTime(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Time");

  HTMLParagraphBegin(html);
  HTMLText(html, "A time is a component of an instance representing one");
  HTMLText(html, "indivisible interval of time in which an event may be");
  HTMLText(html, "running.  For more information, consult the main entry");
  HTMLJump2(html, "op", "spec", "part", "main", "times", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryTimeGroup(HTML html, char *name)                            */
/*                                                                           */
/*  Print the glossary definition of a time group.                           */
/*                                                                           */
/*****************************************************************************/

static void GlossaryTimeGroup(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Time group");

  HTMLParagraphBegin(html);
  HTMLText(html, "A time group is a set of times.");
  HTMLText(html, "For more information, consult the main entry");
  HTMLJump2(html, "op", "spec", "part", "main", "times", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryWeek(HTML html, char *name)                                 */
/*                                                                           */
/*  Print the glossary definition of a week.                                 */
/*                                                                           */
/*****************************************************************************/

static void GlossaryWeek(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Week");

  HTMLParagraphBegin(html);
  HTMLText(html, "A week is a set of times.  As far as HSEval is");
  HTMLText(html, "concerned it is just an alternative name for a");
  HTMLJumpInternal(html, "time_group", "time group");
  HTMLText(html, ", but other software might treat it differently,");
  HTMLText(html, "for example when displaying timetables.  The intention");
  HTMLText(html, "is that all the times of each week should lie in a");
  HTMLText(html, "common");
  HTMLTextMono(html, "Week");
  HTMLText(html, "category.");
  HTMLText(html, "For more information, consult the main entry for times");
  HTMLJump2(html, "op", "spec", "part", "main", "times", "here");
  HTMLText(html, ".");
  HTMLText(html, "See also");
  HTMLJumpInternal(html, "day", "Day");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryWeight(HTML html, char *name)                               */
/*                                                                           */
/*  Print the glossary definition of a weight.                               */
/*                                                                           */
/*****************************************************************************/

static void GlossaryWeight(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Weight");

  HTMLParagraphBegin(html);
  HTMLText(html, "The weight of a constraint is an integer which determines");
  HTMLText(html, "how heavily violations of the constraint are weighted in");
  HTMLText(html, "the total cost.  For more information, consult the");
  HTMLJump2(html, "op", "spec", "part", "constraints", NULL, "constraints");
  HTMLText(html, " page.");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void GlossaryWorkload(HTML html, char *name)                             */
/*                                                                           */
/*  Print the glossary definition of a workload.                             */
/*                                                                           */
/*****************************************************************************/

static void GlossaryWorkload(HTML html, char *name)
{
  HTMLSegmentBegin(html, name, "Workload");

  HTMLParagraphBegin(html);
  HTMLText(html, "The workload of an instance event or solution event");
  HTMLText(html, "is an integer representing how much work the event");
  HTMLText(html, "entails.");
  HTMLText(html, "For more information, consult the main entry for");
  HTMLText(html, "limit workload constraints");
  HTMLJump2(html, "op", "spec", "part", "constraints", "lwc", "here");
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void Glossary(void)                                                      */
/*                                                                           */
/*  Print the glossary section of the specification of the XML file format.  */
/*                                                                           */
/*****************************************************************************/

void Glossary(void)
{
  HTML html; char *hd;
  hd = "High School Timetable Data Format Specification: Glossary";
  html = PageBegin(hd);
  HTMLBigHeading(html, hd);

  HTMLParagraphBegin(html);
  HTMLText(html, "This page defines terms used by the specification of");
  HTMLText(html, "the data format of high school timetable files.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLHSpace(html, 4);
  HTMLJumpFront(html);
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLTableBeginAttributed(html, 0, 0, 0, NULL);
  HTMLTableRowVAlignBegin(html, "top");
  HTMLTableEntryBegin(html);
  HTMLJumpInternalIndented(html,  4, "archive", "Archive");
  HTMLJumpInternalIndented(html,  4, "class", "Class");
  HTMLJumpInternalIndented(html,  4, "constraint", "Constraint");
  HTMLJumpInternalIndented(html,  4, "cost", "Cost");
  HTMLJumpInternalIndented(html,  4, "cost_function", "Cost function");
  HTMLJumpInternalIndented(html,  4, "course", "Course");
  HTMLJumpInternalIndented(html,  4, "day", "Day");
  HTMLJumpInternalIndented(html,  4, "duration", "Duration");
  HTMLJumpInternalIndented(html,  4, "event", "Event");
  HTMLJumpInternalIndented(html,  4, "event_group", "Event group");
  HTMLJumpInternalIndented(html,  4, "event_resource", "Event resource");
  HTMLJumpInternalIndented(html,  4, "id", "Identifier");
  HTMLTableEntryEnd(html);

  HTMLTableEntryBegin(html);
  HTMLJumpInternalIndented(html, 12, "infeasibility_value",
    "Infeasibility value");
  HTMLJumpInternalIndented(html, 12, "instance", "Instance");
  HTMLJumpInternalIndented(html, 12, "event", "Instance event");
  HTMLJumpInternalIndented(html, 12, "name", "Name");
  HTMLJumpInternalIndented(html, 12, "objective_value", "Objective value");
  HTMLJumpInternalIndented(html, 12, "resource", "Resource");
  HTMLJumpInternalIndented(html, 12, "report", "Report");
  HTMLJumpInternalIndented(html, 12, "required", "Required");
  HTMLJumpInternalIndented(html, 12, "resource_group", "Resource group");
  HTMLJumpInternalIndented(html, 12, "resource_type", "Resource type");
  HTMLJumpInternalIndented(html, 12, "role", "Role");
  HTMLJumpInternalIndented(html, 12, "room", "Room");
  HTMLTableEntryEnd(html);

  HTMLTableEntryBegin(html);
  HTMLJumpInternalIndented(html, 12, "solution", "Solution");
  HTMLJumpInternalIndented(html, 12, "solution_event", "Solution event");
  HTMLJumpInternalIndented(html, 12, "solution_group", "Solution group");
  HTMLJumpInternalIndented(html, 12, "solution_resource", "Solution resource");
  HTMLJumpInternalIndented(html, 12, "student", "Student");
  HTMLJumpInternalIndented(html, 12, "solution_event", "Sub-event");
  HTMLJumpInternalIndented(html, 12, "teacher", "Teacher");
  HTMLJumpInternalIndented(html, 12, "time", "Time");
  HTMLJumpInternalIndented(html, 12, "time_group", "Time group");
  HTMLJumpInternalIndented(html, 12, "week", "Week");
  HTMLJumpInternalIndented(html, 12, "weight", "Weight");
  HTMLJumpInternalIndented(html, 12, "workload", "Workload");
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);

  HTMLTableEnd(html);
  HTMLParagraphEnd(html);
  HTMLHorizontalRule(html);

  GlossaryArchive(html, "archive");
  GlossaryClass(html, "class");
  GlossaryConstraint(html, "constraint");
  GlossaryCost(html, "cost");
  GlossaryCostFunction(html, "cost_function");
  GlossaryCourse(html, "course");
  GlossaryDay(html, "day");
  GlossaryDuration(html, "duration");
  GlossaryEvent(html, "event");
  GlossaryEventGroup(html, "event_group");
  GlossaryEventResource(html, "event_resource");
  GlossaryIdentifier(html, "id");
  GlossaryInfeasibilityValue(html, "infeasibility_value");
  GlossaryInstance(html, "instance");
  GlossaryName(html, "name");
  GlossaryObjectiveValue(html, "objective_value");
  GlossaryReport(html, "report");
  GlossaryRequired(html, "required");
  GlossaryResource(html, "resource");
  GlossaryResourceGroup(html, "resource_group");
  GlossaryResourceType(html, "resource_type");
  GlossaryRole(html, "role");
  GlossaryRoom(html, "room");
  GlossarySolution(html, "solution");
  GlossarySolutionEvent(html, "solution_event");
  GlossarySolutionGroup(html, "solution_group");
  GlossarySolutionResource(html, "solution_resource");
  GlossaryStudent(html, "student");
  GlossaryTeacher(html, "teacher");
  GlossaryTime(html, "time");
  GlossaryTimeGroup(html, "time_group");
  GlossaryWeek(html, "week");
  GlossaryWeight(html, "weight");
  GlossaryWorkload(html, "workload");

  PageEnd(html);
}
