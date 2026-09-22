############################################################################
#                                                                          #
#  Make file for compiling khe_guide.pdf                                   #
#                                                                          #
#  Jeffrey H. Kingston                                                     #
#                                                                          #
#  The PDF file "khe_guide.pdf" compiled by this makefile holds the User's #
#  Guide to the KHE platform and solvers.                                  #
#                                                                          #
#     make           Compile khe_guide.pdf                                 #
#     make clean     Undo "make" but don't remove khe_guide.pdf            #
#     make restart   Undo everything ready for a fresh start               #
#                                                                          #
#  Because compilation of the Guide requires the Lout document formatting  #
#  system, which most people would not have, KHE is distributed with       #
#  khe_guide.pdf already made.  And to make sure this happens, in this     #
#  case "make restart" does not remove khe_guide.pdf.                      #
#                                                                          #
#  Mail jeff@it.usyd.edu.au if you have any problems.                      #
#                                                                          #
############################################################################

khe_guide.pdf : all intro archives instances solutions extras		\
    		  monitoring matchings general_solvers time_structural	\
		  time_solvers resource_structural resource_solvers	\
		  eject modules impl ha hw dynamic_theory dynamic_impl
	lout -r5 all >op; ps2pdf op khe_guide.pdf

clean :
	rm -f lout.li *.ld op

restart : clean
