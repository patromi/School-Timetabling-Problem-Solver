############################################################################
#                                                                          #
#  Make file for compiling the entire KHE suite                            #
#                                                                          #
#  Jeffrey H. Kingston                                                     #
#                                                                          #
#  This makefile compiles the entire KHE suite, except for the             #
#  documentation directories.  Before running it, however, you need to     #
#  say where you want the three executables, khe, hseval.cgi, and nrconv,  #
#  to be copied to.  You do that by editing                                #
#                                                                          #
#     src_khe/makefile                                                     #
#     src_hseval/makefile                                                  #
#     src_nrconv/makefile                                                  #
#                                                                          #
#  to change the value of symbol FINAL_DIR to the path name of the         #
#  directory where each executable is to go (~/bin, /usr/lib/cgi-bin, or   #
#  whatever).                                                              #
#                                                                          #
#     make           Make everything and copy the executables              #
#     make clean     Undo "make" except for the final results              #
#     make restart   Undo everything ready for a fresh start               #
#                                                                          #
#  For those who have Lout there is also                                   #
#                                                                          #
#     make doc       Compile the PDF guides and clean their directories    #
#                                                                          #
#  Mail jeff@it.usyd.edu.au if you have any problems.                      #
#                                                                          #
############################################################################

all :
	$(MAKE) -C src_platform
	$(MAKE) -C src_solvers
	$(MAKE) -C src_khe
	$(MAKE) -C src_hseval
	$(MAKE) -C src_nrc
	$(MAKE) -C src_nrconv
	echo "" >&2

doc :
	$(MAKE) -C doc_khe
	$(MAKE) -C doc_khe clean
	$(MAKE) -C doc_nrconv
	$(MAKE) -C doc_nrconv clean


############################################################################
#                                                                          #
#   Clean and restart                                                      #
#                                                                          #
############################################################################

clean :
	$(MAKE) -C src_platform clean
	$(MAKE) -C src_solvers clean
	$(MAKE) -C src_khe clean
	$(MAKE) -C src_hseval clean
	$(MAKE) -C src_nrc clean
	$(MAKE) -C src_nrconv clean

restart :
	$(MAKE) -C src_platform restart
	$(MAKE) -C src_solvers restart
	$(MAKE) -C src_khe restart
	$(MAKE) -C src_hseval restart
	$(MAKE) -C src_nrc restart
	$(MAKE) -C src_nrconv restart
