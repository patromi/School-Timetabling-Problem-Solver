############################################################################
#                                                                          #
#  Make file for "nrconv"                                                  #
#                                                                          #
#  Jeffrey H. Kingston                                                     #
#                                                                          #
#  The program compiled by this makefile, "nrconv", is an executable file  #
#  which converts instances and solutions of nurse rostering problems      #
#  from several formats to XESTT.  It incorporates archives "nrc.a" and    #
#  "khe_platform.a".                                                       #
#                                                                          #
#     make           Compile nrconv and copy it to $(FINAL_DIR)            #
#     make clean     Undo "make" except for the final copy                 #
#     make restart   Undo everything ready for a fresh start               #
#                                                                          #
#  Do not run "make nrconv" directly, it does not bring the archives used  #
#  by nrconv up to date.                                                   #
#                                                                          #
#  Mail jeff@it.usyd.edu.au if you have any problems.                      #
#                                                                          #
#  Implementation note.  It's hard to find a way to ensure that files      #
#  like ../src_platform/khe_platform.a and ../src_nrc/nrc.a that reside    #
#  in different directories and are subject to different makefiles are     #
#  brought up to date before their last changed times are tested.  But     #
#  this makefile does it.                                                  #
#                                                                          #
############################################################################

# final resting place of the executable - change it!
FINAL_DIR = ~/bin

CC	= gcc

CFLAGS	= -g -std=c99 -pedantic -Wall -pthread -O3			\
  -I../src_nrc -I../src_platform

ARCHIVES = ../src_nrc/nrc.a ../src_platform/khe_platform.a

OBJS	= error.o map.o ins_model.o soln_model.o			\
	  coi_bound.o coi_cover.o coi.o inrc1.o inrc2.o cq14.o main.o

all :
	$(MAKE) -C ../src_platform
	$(MAKE) -C ../src_nrc
	$(MAKE) nrconv

nrconv : $(OBJS) $(ARCHIVES)
	$(CC) -o nrconv $(OBJS) $(ARCHIVES) -lm
	chmod a+x nrconv
	cp nrconv $(FINAL_DIR)/nrconv

$(OBJS) : externs.h ../src_nrc/nrc.h ../src_platform/khe_platform.h \
  ../src_platform/howard_a.h ../src_platform/howard_n.h ../src_platform/kml.h


############################################################################
#                                                                          #
#   Clean and restart                                                      #
#                                                                          #
############################################################################

clean :
	rm -f nrconv $(OBJS)
	$(MAKE) -C ../src_platform clean
	$(MAKE) -C ../src_nrc clean

restart : clean
	$(MAKE) -C ../src_platform restart
	$(MAKE) -C ../src_nrc restart
	rm -f $(FINAL_DIR)/nrconv
