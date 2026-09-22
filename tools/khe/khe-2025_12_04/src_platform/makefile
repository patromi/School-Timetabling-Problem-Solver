############################################################################
#                                                                          #
#  Make file for compiling the KHE platform (not the solvers)              #
#                                                                          #
#  Jeffrey H. Kingston                                                     #
#                                                                          #
#  The archive file "khe_platform.a" compiled by this makefile holds       #
#  the basic module of KHE, which reads and writes XML files, supports     #
#  incremental solving, etc.  It does not have a main program.             #
#                                                                          #
#     make                  Compile khe_platform.a                         #
#     make clean            Undo "make" but don't remove khe_platform.a    #
#     make restart          Undo everything ready for a fresh start        #
#                                                                          #
#  The result file, "khe_platform.a", is not copied to anywhere else;      #
#  other software is expected to obtain it directly from this directory.   #
#                                                                          #
#  Mail jeff@it.usyd.edu.au if you have any problems.                      #
#                                                                          #
############################################################################

CC = gcc

CFLAGS = -g -std=c99 -pedantic -Wall -pthread -O3


############################################################################
#                                                                          #
#   Separate files                                                         #
#                                                                          #
#   Separate files are source files with their own header files that       #
#   could be extracted from KHE and used separately:                       #
#                                                                          #
#     File               Contents                                 Guide    #
#     -----------------------------------------------------------------    #
#     ha_all.c           Arenas and extensible arrays             App A    #
#     hn_all.c           Narrow strings and symbol tables         App A    #
#     hp_all.c           Pointer symbol tables                    App A    #
#     m.[hc]             Extensible arrays and symbol tables      App A    #
#     khe_set.[hc]       Sorted sets of integers                  App ?    #
#     sset.[hc]          Shiftable sorted sets of integers        App ?    #
#     kml*.[hc]          XML reading and writing                  App D    #
#     -----------------------------------------------------------------    #
#                                                                          #
############################################################################

SEPARATE_OBJS = ha_all.o hn_all.o hp_all.o sset.o khe_set.o kml.o kml_read.o


############################################################################
#                                                                          #
#  Platform files                                                          #
#                                                                          #
#  Platform files implement KHE's platform.  They #include file            #
#  khe_interns.h, which includes khe_platform.h and the header             #
#  files of all the separate modules.                                      #
#                                                                          #
#     Files              Contents                                 Guide    #
#     -----------------------------------------------------------------    #
#     INSTANCE_OBJS      Archives, soln groups, instances        Ch 2-3    #
#     CONSTRAINT_OBJS    Constraints                             Sc 3.7    #
#     SOLN_OBJS          Solns                                   Ch 4-5    #
#     MONITOR_OBJS       Monitors                                Ch 6-7    #
#     -----------------------------------------------------------------    #
#                                                                          #
############################################################################

PLATFORM_HDRS = khe_interns.h khe_platform.h kml.h howard_a.h howard_n.h \
  howard_p.h khe_set.h sset.h khe_matching.h

INSTANCE_OBJS = khe_archive.o khe_soln_group.o khe_soln_set.o		\
  khe_instance.o khe_time_group.o khe_time.o khe_resource_type.o	\
  khe_resource_group.o khe_resource.o khe_event_group.o khe_event.o	\
  khe_event_resource.o khe_event_resource_group.o

CONSTRAINT_OBJS = khe_constraint.o					  \
  khe_assign_resource_constraint.o khe_assign_time_constraint.o		  \
  khe_split_events_constraint.o khe_distribute_split_events_constraint.o  \
  khe_prefer_resources_constraint.o khe_prefer_times_constraint.o	  \
  khe_avoid_split_assignments_constraint.o khe_spread_events_constraint.o \
  khe_link_events_constraint.o khe_order_events_constraint.o		  \
  khe_avoid_clashes_constraint.o khe_avoid_unavailable_times_constraint.o \
  khe_limit_idle_times_constraint.o khe_cluster_busy_times_constraint.o	  \
  khe_limit_busy_times_constraint.o khe_limit_workload_constraint.o	  \
  khe_limit_active_intervals_constraint.o khe_limit_resources_constraint.o

SOLN_OBJS = khe_soln.o khe_soln_write_only.o khe_evenness_handler.o	\
  khe_redundancy_handler.o khe_meet.o khe_meet_bound.o khe_cycle_meet.o	\
  khe_task.o khe_task_bound.o khe_avail.o khe_mark.o khe_path.o		\
  khe_trace.o khe_node.o khe_layer.o khe_zone.o 			\
  khe_task_set.o khe_meet_set.o khe_time_set.o khe_resource_set.o	\
  khe_frame.o khe_event_in_soln.o khe_event_resource_in_soln.o		\
  khe_resource_in_soln.o khe_matching.o

MONITOR_OBJS = khe_monitor.o						\
  khe_assign_resource_monitor.o khe_assign_time_monitor.o		\
  khe_split_events_monitor.o khe_distribute_split_events_monitor.o	\
  khe_prefer_resources_monitor.o khe_prefer_times_monitor.o		\
  khe_avoid_split_assignments_monitor.o khe_spread_events_monitor.o	\
  khe_link_events_monitor.o khe_order_events_monitor.o			\
  khe_avoid_clashes_monitor.o khe_avoid_unavailable_times_monitor.o	\
  khe_limit_idle_times_monitor.o khe_cluster_busy_times_monitor.o	\
  khe_limit_busy_times_monitor.o khe_limit_workload_monitor.o		\
  khe_limit_active_intervals_monitor.o khe_limit_resources_monitor.o	\
  khe_event_timetable_monitor.o khe_resource_timetable_monitor.o	\
  khe_monitored_time_group.o khe_group_monitor.o			\
  khe_ordinary_demand_monitor.o khe_workload_demand_monitor.o		\
  khe_evenness_monitor.o

PLATFORM_OBJS = $(INSTANCE_OBJS) $(CONSTRAINT_OBJS) $(SOLN_OBJS) $(MONITOR_OBJS)


############################################################################
#                                                                          #
#   Main rule, for producing archive file "khe_platform.a"                 #
#                                                                          #
############################################################################

khe_platform.a : $(SEPARATE_OBJS) $(PLATFORM_OBJS)
	rm -f khe_platform.a
	ar rcs khe_platform.a $(SEPARATE_OBJS) $(PLATFORM_OBJS)


############################################################################
#                                                                          #
#   Rules for object files                                                 #
#                                                                          #
############################################################################

ha_all.o: howard_a.h
hn_all.o: howard_a.h howard_n.h
hp_all.o: howard_a.h howard_p.h
kml.o kml_read.o : kml.h howard_a.h
khe_set.o : khe_set.h
sset.o : sset.h
$(PLATFORM_OBJS) : $(PLATFORM_HDRS)


############################################################################
#                                                                          #
#   Clean and restart                                                      #
#                                                                          #
############################################################################

clean :
	rm -f *.o

restart : clean
	rm -f khe_platform.a
