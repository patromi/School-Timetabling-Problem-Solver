############################################################################
#                                                                          #
#  Make file for compiling nrconv_guide.pdf                                #
#                                                                          #
#  Jeffrey H. Kingston                                                     #
#                                                                          #
#  The PDF file "nrconv_guide.pdf" compiled by this makefile holds the     #
#  User's Guide to NRConv, a program for converting nurse rostering        #
#  instances and solutions in a variety of formats to the XESTT format.    #
#                                                                          #
#     make           Compile khe_guide.pdf                                 #
#     make clean     Undo "make" but don't remove khe_guide.pdf            #
#     make restart   Undo everything ready for a fresh start               #
#                                                                          #
#  Because compilation of the Guide requires the Lout document formatting  #
#  system, which most people would not have, KHE is distributed with       #
#  nrconv_guide.pdf already made.  And to make sure this happens, in this  #
#  case "make restart" does not remove nrconv_guide.pdf.                   #
#                                                                          #
#  Mail jeff@it.usyd.edu.au if you have any problems.                      #
#                                                                          #
############################################################################

nrconv_guide.pdf : all intro archives instances source
	lout -r4 all >op; ps2pdf op nrconv_guide.pdf

clean :
	rm -f lout.li *.ld op

restart : clean
