############################################################################
#                                                                          #
#  Make file for "hseval.cgi"                                              #
#                                                                          #
#  Jeffrey H. Kingston                                                     #
#                                                                          #
#  The program compiled by this makefile, "hseval.cgi", is an executable   #
#  file which evaluates solutions to instances of timetabling problems.    #
#  It can be placed in a cgi-bin directory and run as a web service.  It   #
#  can also be run directly from the command line.  It incorporates the    #
#  archive holding the KHE platform, but not the one holding the solvers.  #
#                                                                          #
#     make           Compile hseval.cgi and copy it to $(FINAL_DIR).       #
#                                                                          #
#     make clean     Undo "make" except for the final copying.             #
#                                                                          #
#     make restart   Undo everything ready for a fresh start               #
#                                                                          #
#  Do not run "make hseval.cgi" directly, it does not bring the archive    #
#  used by hseval.cgi (containing the KHE platform) up to date.            #
#                                                                          #
#  Mail jeff@it.usyd.edu.au if you have any problems.                      #
#                                                                          #
#  Implementation note.  It's hard to find a way to ensure that files like #
#  ../src_platform/khe_platform.a that reside in different directories and #
#  are subject to different makefiles are brought up to date before their  #
#  last changed times are tested.  But this makefile does it.              #
#                                                                          #
############################################################################

# directory of final resting place of the executable - change it!
FINAL_DIR = /usr/lib/cgi-bin

HSEVAL_RERUN = 1

HSEVAL_TIME_LIMIT = 300

CC	= gcc

CFLAGS	= -g -std=c99 -pedantic -Wall -O3 -I../src_platform	\
  -DHSEVAL_RERUN=$(HSEVAL_RERUN) -DHSEVAL_TIME_LIMIT=$(HSEVAL_TIME_LIMIT)

ARCHIVES = ../src_platform/khe_platform.a

OBJS	= command.o glossary.o html.o main.o model.o ranking.o	\
	  report.o spec.o summary.o table.o eval_table.o defect_info.o	\
	  timetable.o avail.o url.o space.o

all :
	$(MAKE) -C ../src_platform
	$(MAKE) hseval.cgi

hseval.cgi : $(OBJS) $(ARCHIVES)
	$(CC) -o hseval.cgi $(OBJS) $(ARCHIVES) -lm
	chmod a+x hseval.cgi
	cp hseval.cgi $(FINAL_DIR)/hseval.cgi;

$(OBJS) : ../src_platform/khe_platform.h command.h externs.h	\
	  html.h table.h eval_table.h url.h space.h


############################################################################
#                                                                          #
#   Clean and restart                                                      #
#                                                                          #
############################################################################

clean :
	rm -f hseval.cgi $(OBJS)
	$(MAKE) -C ../src_platform clean

restart : clean
	$(MAKE) -C ../src_platform restart
	rm -f $(FINAL_DIR)/hseval.cgi
