############################################################################
#                                                                          #
#  Make file for compiling NRC                                             #
#                                                                          #
#  Jeffrey H. Kingston                                                     #
#                                                                          #
#  The archive file "nrc.a" compiled by this makefile holds the object     #
#  files that implement the NRC nurse rostering platform.  It does not     #
#  have a main program.                                                    #
#                                                                          #
#     make           Compile nrc.a                                         #
#     make clean     Undo "make" but don't remove nrc.a                    #
#     make restart   Undo everything ready for a fresh start               #
#                                                                          #
#  NRC calls the Unix mktime() function to handle dates in year-mon-day    #
#  format.  You can avoid this function by setting flag USE_MKTIME in      #
#  nrc.h to 0 - but then you will get an exit with error message if        #
#  year-mon-day values have to be converted.                               #
#                                                                          #
#  Mail jeff@it.usyd.edu.au if you have any problems.                      #
#                                                                          #
############################################################################

CC = gcc

CFLAGS = -g -std=c99 -pedantic -Wall -O3 -I../src_platform

HDRS = nrc.h nrc_interns.h ../src_platform/khe_platform.h		\
  ../src_platform/howard_a.h ../src_platform/howard_n.h

ARCHIVES = ../src_platform/khe_platform.a

OBJS = nrc_archive.o nrc_soln_group.o nrc_soln.o nrc_instance.o		\
	nrc_day.o nrc_day_set.o nrc_day_set_set.o			\
	nrc_time_interval.o nrc_shift_type.o nrc_shift_type_set.o	\
	nrc_shift.o nrc_shift_set.o nrc_shift_set_set.o			\
	nrc_worker.o nrc_worker_set.o nrc_worker_set_set.o		\
	nrc_worker_set_tree.o nrc_penalty.o nrc_bound.o 		\
	nrc_demand.o nrc_demand_set.o nrc_demand_constraint.o		\
	nrc_dc_converter.o nrc_pattern.o nrc_pattern_set.o		\
	nrc_constraint.o nrc_condensed.o


############################################################################
#                                                                          #
#   Main rule, for producing archive file "nrc.a"                          #
#                                                                          #
############################################################################

nrc.a : $(OBJS)
	ar rcs nrc.a $(OBJS)


############################################################################
#                                                                          #
#   Rules for separate files                                               #
#                                                                          #
############################################################################

$(OBJS) : $(HDRS)


############################################################################
#                                                                          #
#   Clean and restart                                                      #
#                                                                          #
############################################################################

clean :
	rm -f *.o

restart : clean
	rm -f nrc.a
