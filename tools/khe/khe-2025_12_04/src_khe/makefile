############################################################################
#                                                                          #
#  Make file for compiling the "khe" executable                            #
#                                                                          #
#  Jeffrey H. Kingston                                                     #
#                                                                          #
#  The program "khe" compiled by this makefile is an executable file for   #
#  solving instances of timetabling problems.  It incorporates archives    #
#  holding the KHE platform and solvers with a main program of its own.    #
#                                                                          #
#     make           Compile khe and copy it to $(FINAL_DIR).              #
#     make clean     Undo "make" except for the final copy                 #
#     make restart   Undo everything ready for a fresh start               #
#                                                                          #
#  Do not run "make khe" directly, it does not bring the archives          #
#  used by khe (containing the KHE platform and solvers) up to date.       #
#                                                                          #
#  Mail jeff@it.usyd.edu.au if you have any problems.                      #
#                                                                          #
#  Implementation note.  It's hard to find a way to ensure that files      #
#  like ../src_solvers/khe_solvers.a and ../src_platform/khe_platform.a    #
#  that reside in different directories and are subject to different       #
#  makefiles are brought up to date before their last changed times        #
#  are tested.  But this makefile does it.                                 #
#                                                                          #
############################################################################

# final resting place of the executable - change it!
FINAL_DIR = ~/bin

CC = gcc

CFLAGS = -g -std=c99 -pedantic -Wall -pthread -O3 			\
  -I../src_platform -I../src_solvers

ARCHIVES = ../src_solvers/khe_solvers.a ../src_platform/khe_platform.a

OBJS = khe_main.o

all :
	$(MAKE) -C ../src_platform
	$(MAKE) -C ../src_solvers
	$(MAKE) khe

khe : $(OBJS) $(ARCHIVES)
	$(CC) -o khe $(OBJS) $(ARCHIVES) -lm -pthread
	chmod a+x khe
	cp khe $(FINAL_DIR)/khe

$(OBJS) : ../src_platform/khe_platform.h ../src_platform/sset.h		\
  ../src_solvers/khe_partition.h ../src_solvers/khe_solvers.h


############################################################################
#                                                                          #
#   Clean and restart                                                      #
#                                                                          #
############################################################################

clean :
	rm -f khe_main.o khe
	$(MAKE) -C ../src_platform clean
	$(MAKE) -C ../src_solvers clean

restart : clean
	$(MAKE) -C ../src_platform restart
	$(MAKE) -C ../src_solvers restart
	rm -f $(FINAL_DIR)/khe
