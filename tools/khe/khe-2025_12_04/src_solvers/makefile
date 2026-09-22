############################################################################
#                                                                          #
#  Make file for compiling the KHE solvers (not the platform)              #
#                                                                          #
#  Jeffrey H. Kingston                                                     #
#                                                                          #
#  The archive file "khe_solvers.a" compiled by this makefile holds        #
#  solvers built on the KHE platform.  It does not have a main program.    #
#                                                                          #
#     make           Compile khe_solvers.a                                 #
#     make clean     Undo "make" but don't remove khe_solvers.a            #
#     make restart   Undo everything ready for a fresh start               #
#                                                                          #
#  There are several places where this module calls Unix time functions.   #
#  You can remove these calls by finding line "#define KHE_USE_TIMING 1"   #
#  near the top of khe_solvers.h and changing the 1 to a 0.  This will     #
#  cause all times reported by KHE to be 0.                                #
#                                                                          #
#  This module offers parallel solve functions, implemented using the      #
#  pthread system library.  If you don't have that library, you can still  #
#  compile KHE, by carrying out these two steps first:                     #
#                                                                          #
#  (1) Remove "-pthread" from the CFLAGS line just below.                  #
#                                                                          #
#  (2) Find line "#define KHE_USE_PTHREAD 1" near the top of file          #
#      khe_solvers.h and change the 1 to a 0.                              #
#                                                                          #
#  The only difference will be that the parallel solve functions will      #
#  carry out their solves sequentially instead of in parallel.             #
#                                                                          #
#  Mail jeff@it.usyd.edu.au if you have any problems.                      #
#                                                                          #
############################################################################

CC = gcc

CFLAGS = -g -pedantic -Wall -pthread -O3 -I../src_platform


############################################################################
#                                                                          #
#   Separate files                                                         #
#                                                                          #
#   Separate files are source files with their own header files that       #
#   could be extracted from KHE and used separately:                       #
#                                                                          #
#     File               Contents                                 Guide    #
#     -----------------------------------------------------------------    #
#     khe_trie.[hc]       Generic tries                           TBC      #
#     khe_priqueue.[hc]   Updateable priority queues              App C    #
#     khe_wmatch.[hc]     Weighted bipartite matching               -      #
#     khe_mmatch.[hc]     Weighted bipartite matching (optimized)   -      #
#     khe_partition.[hc]  Partitions (e.g. 2 1 is partition of 3)   -      #
#     khe_part_graph.[hc] Graph partitioning (for two-colouring)    -      #
#     -----------------------------------------------------------------    #
#                                                                          #
############################################################################

SEPARATE_OBJS = khe_trie.o khe_priqueue.o khe_wmatch.o khe_mmatch.o	\
  khe_partition.o khe_part_graph.o


############################################################################
#                                                                          #
#   Common header files                                                    #
#                                                                          #
#   These are header files used by some (or all) solvers.                  #
#                                                                          #
############################################################################

SOLVER_COMMON_HDRS = khe_solvers.h ../src_platform/khe_platform.h	\
  ../src_platform/howard_a.h ../src_platform/howard_n.h			\
  ../src_platform/howard_p.h ../src_platform/khe_set.h


############################################################################
#                                                                          #
#   Miscellaneous solver files                                             #
#                                                                          #
#   Miscellaneous solver files contain solvers and related functions,      #
#   mainly from Chapter 8, that do not fit into other solver categories.   #
#                                                                          #
#   File                        Contents                          Guide    #
#   ---------------------------------------------------------------------  #
#   khe_sm_timer.c              Timers and timer sets              8.1     #
#   khe_sm_options.c            Options                            8.2     #
#   khe_sm_yourself.c           Do-it-yourself solving             8.3     #
#   khe_sm_general_solve.c      General solving                    8.4     #
#   khe_sm_parallel_solve.c     Parallel solving                   8.5     #
#   khe_sm_soln_adjuster.c      Solution adjustments               8.6     #
#   khe_sm_correlation.c        Monitor grouping for correlation   8.7.4   #
#   khe_sm_matching.c           Matching construction              8.8     #
#   khe_sm_stats.c              Statistics                         8.9     #
#   khe_sm_backoff.c            Exponential backoff                8.10    #
#   khe_sm_random.c             Thread-safe random numbers         8.11    #
#   khe_sm_interval.c           Sequences of integers              8.12    #
#   khe_sm_multiset_generator.c Multiset generator                 8.13    #
#   ---------------------------------------------------------------------  #
#                                                                          #
############################################################################

SOLVER_MISC_HDRS = $(SOLVER_COMMON_HDRS)

SOLVER_MISC_OBJS = khe_sm_timer.o khe_sm_options.o khe_sm_yourself.o	\
  khe_sm_general_solve.o khe_sm_parallel_solve.o khe_sm_soln_adjuster.o	\
  khe_sm_correlation.o khe_sm_matching.o khe_sm_stats.o			\
  khe_sm_backoff.o khe_sm_random.o khe_sm_interval.o			\
  khe_sm_multiset_generator.o


############################################################################
#                                                                          #
#   Layer tree construction solver files                                   #
#                                                                          #
#   Layer tree construction solver files contain the implementation of     #
#   the layer tree construction algorithm (Section 9.1).  Their names      #
#   have the form khe_sl_*.c.  Like all solver files they depend on        #
#   khe_platform.h.  They also have their own private header file,         #
#   khe_sl_layer_tree.h.                                                   #
#                                                                          #
############################################################################

SOLVER_LAYER_TREE_HDRS = $(SOLVER_COMMON_HDRS) khe_sl_layer_tree.h	\
  khe_partition.h

SOLVER_LAYER_TREE_OBJS = khe_sl_layer_tree.o khe_sl_split_forest.o	\
  khe_sl_split_class.o khe_sl_split_layer.o khe_sl_split_job.o		\
  khe_sl_avoid_clashes_split_job.o khe_sl_pack_split_job.o		\
  khe_sl_preassigned_split_job.o khe_sl_assigned_split_job.o		\
  khe_sl_link_split_job.o khe_sl_split_split_job.o			\
  khe_sl_distribute_split_job.o khe_sl_spread_split_job.o		\
  khe_sl_domain_split_job.o


############################################################################
#                                                                          #
#   Time-structural solver files                                           #
#                                                                          #
#   Time-structural solver files contain time-structural solvers           #
#   (Chapter 9), excluding those that implement layer tree construction.   #
#                                                                          #
#   File                        Contents                          Guide    #
#   ---------------------------------------------------------------------  #
#   khe_ss_time_equiv.c         Time-equivalence                   9.2     #
#   khe_ss_layers.c             Layers                             9.3     #
#   khe_ss_runarounds.c         Runarounds                         9.4     #
#   khe_ss_nodes.c              Rearranging nodes                  9.6     #
#   khe_ss_zones.c              Zones                              9.6     #
#   khe_ss_splits.c             Analysing split defects            9.7.1   #
#   khe_ss_misc.c               Merging adjacent meets             9.7.2   #
#   khe_ss_grouping.c           Monitor attachment and grouping    9.8     #
#   ---------------------------------------------------------------------  #
#                                                                          #
############################################################################

SOLVER_TIME_STRUCTURAL_HDRS = $(SOLVER_COMMON_HDRS)

SOLVER_TIME_STRUCTURAL_OBJS = khe_ss_time_equiv.o khe_ss_layers.o	\
  khe_ss_runarounds.o khe_ss_nodes.o khe_ss_zones.o khe_ss_splits.o	\
  khe_ss_misc.o khe_ss_grouping.o


############################################################################
#                                                                          #
#   Time solver files                                                      #
#                                                                          #
#   Time solver files contain time solvers (Chapter 10).                   #
#                                                                          #
#   File                        Contents                          Guide    #
#   ---------------------------------------------------------------------  #
#   khe_st_helper.c             Node assignment functions, etc.   10.2     #
#   khe_st_kempe.c              Kempe and ejecting meet moves     10.2.3   #
#   khe_st_meet_bound_group.c   Meet bound groups                 10.3.1   #
#   khe_st_domain.c             Domain reduction                  10.3.2   #
#   khe_st_cluster_and_limit.c  Domain reduction                  10.3.3   #
#   khe_st_basic.c              Basic time solvers                10.4     #
#   khe_st_runaround.c          Runarounds                        10.5     #
#   khe_st_elm_core.c           Elm - core module and main fn     10.6.2   #
#   khe_st_elm_split.c          Elm - splitting supplies          10.6.3   #
#   khe_st_elm_node.c           Elm - improving node regularity   10.6.4   #
#   khe_st_elm_irregular.c      Elm - handling irregular monitors 10.6.5   #
#   khe_st_layer_node_match.c   Layer node matching               10.7.1   #
#   khe_st_tree_repair.c        Tree search layer time repair     10.7.3   #
#   khe_st_meet_set_repair.c    Meet set time repair              10.7.4   #
#   khe_st_layer_asst.c         Layer assignments                 10.8.1   #
#   khe_st_layered.c            A solver for layered time asst    10.8.2   #
#   khe_st_combined.c           A complete time solver            10.8.3   #
#   ---------------------------------------------------------------------  #
#                                                                          #
############################################################################

SOLVER_TIME_HDRS = $(SOLVER_COMMON_HDRS) khe_wmatch.h khe_elm.h

SOLVER_TIME_OBJS =  khe_st_helper.o khe_st_kempe.o			\
  khe_st_meet_bound_group.o khe_st_domain.o khe_st_cluster_and_limit.o	\
  khe_st_basic.o khe_st_runaround.o khe_st_elm_core.o			\
  khe_st_elm_split.o khe_st_elm_node.o khe_st_elm_irregular.o		\
  khe_st_layer_node_match.o khe_st_tree_repair.o			\
  khe_st_meet_set_repair.o khe_st_layer_asst.o khe_st_layered.o		\
  khe_st_combined.o


############################################################################
#                                                                          #
#   Resource structural files                                              #
#                                                                          #
#   These files contain resource-structural solvers (Chapter 11).          #
#                                                                          #
#   File                        Contents                          Guide    #
#   ---------------------------------------------------------------------  #
#   khe_sr_task_tree.c          Task tree construction            11.1.2   #
#   khe_sr_classify.c           Classify resources by workload    11.2.1   #
#   khe_sr_consec_solver.c      Limits on consecutive days        11.2.2   #
#   khe_sr_partition.c          Tighten to partition              11.2.3   #
#   khe_sr_balance.c            Supply and demand balance         11.2.4   #
#   khe_sr_resource_flow.c      Resource flow                     11.2.5   #
#   khe_sr_workload.c           Workload packing                  11.2.6   #
#   khe_sr_adjustments.c        Miscellaneous adjustments         11.3     #
#   khe_sr_cluster_minimum.c    Cluster minimum limits            11.3.4   #
#   khe_sr_structure.c          Split assignments; task domains   11.3.5-6 #
#   khe_sr_constraint_classes.c Constraint classes                11.4     #
#   khe_sr_task_finder.c        Task finder objects               11.5     #
#   khe_sr_mtask_finder.c       Multi-tasks                       11.6     #
#   khe_sr_task_group_domain.c  Task group domain finder          11.7.1   #
#   khe_sr_task_grouper.c       Task grouper                      11.7.2   #
#   khe_sr_mtask_grouper.c      MTask grouper                     11.7.3   #
#   khe_sr_simple_grouping.c    Simple grouping                   11.7.4   #
#   khe_sr_comb_grouping.c      Combinatorial grouping            11.7.5   #
#   khe_sr_weekend_grouping.c   Weekend grouping                  11.7.6   #
#   khe_sr_interval_grouping.c  Interval grouping                 11.7.7   #
#   khe_sr_grouped_tasks_display.c  Grouped tasks display         11.7.8   #
#   ---------------------------------------------------------------------  #
#                                                                          #
#   Obsolete                                                               #
#   khe_sr_tgrc.h	        Task grouping by resource cons             #
#   khe_sr_tgrc_main.c          TGRC - main functions             11.7     #
#   khe_sr_tgrc_comb.c          TGRC - combinatorial grouping     -        #
#   khe_sr_tgrc_elim.c          TGRC - combination elimination    -        #
#   khe_sr_tgrc_group.c         TGRC - mtask grouping             -        #
#   khe_sr_group_by_resource.c  Group by resource                 11.6.3   #
#   Khe_sr_task_resource_grouper.c Task resource grouper          11.6.4   #
#   khe_sr_task_bound_group.c   Task bound groups                 11.1     #
#   khe_sr_comb_grouper.c       Combinatorial grouping            11.10.1  #
#   khe_sr_comb_finder.c        Grouping by resource constraints  11.10.2  #
#   khe_sr_profile_grouper.c    Profile grouping                  11.10.3  #
#   khe_sr_task_group.c         Task groups                       11.10    #
#   khe_sr_task_class.c         Task classes                      11.11    #
#                                                                          #
#   khe_sr_tasker.c [old]       Tasker objects                    11.6     #
#   khe_sr_comb_solver.c [old]  Combinatorial grouping solver     11.6     #
#   khe_sr_group_by_rc.c [old]  Group by resource constraints     11.6     #
#   khe_sr_tgrc_profile.c       TGRC - profile grouping           -        #
#                                                                          #
############################################################################
# khe_sr_tasker.o khe_sr_comb_solver.o khe_sr_group_by_rc.o

SOLVER_RESOURCE_STRUCTURAL_HDRS = $(SOLVER_COMMON_HDRS) khe_mmatch.h	\
  khe_part_graph.h

SOLVER_RESOURCE_STRUCTURAL_OBJS = khe_sr_task_tree.o khe_sr_classify.o	\
  khe_sr_consec_solver.o khe_sr_partition.o khe_sr_balance.o		\
  khe_sr_workload.o khe_sr_resource_flow.o khe_sr_adjustments.o		\
  khe_sr_cluster_minimum.o khe_sr_structure.o				\
  khe_sr_constraint_classes.o khe_sr_task_finder.o			\
  khe_sr_mtask_finder.o khe_sr_task_group_domain.o			\
  khe_sr_task_grouper.o khe_sr_mtask_grouper.o				\
  khe_sr_simple_grouping.o khe_sr_comb_grouping.o			\
  khe_sr_weekend_grouping.o khe_sr_interval_grouping.o			\
  khe_sr_grouped_tasks_display.o
# khe_sr_tgrc_group.o
# khe_sr_task_resource_grouper.o
# khe_sr_group_by_resource.o
# khe_sr_task_bound_group.o
# khe_sr_tgrc_profile.o
# khe_sr_comb_grouper.o khe_sr_comb_finder.o khe_sr_profile_grouper.o


############################################################################
#                                                                          #
#   Resource solver files                                                  #
#                                                                          #
#   Resource solver files contain resource solvers (Chapter 11).           #
#                                                                          #
#   File                        Contents                          Guide    #
#   ---------------------------------------------------------------------  #
#   khe_sr_invariant.c          Resource assignment invariant     12.2     #
#   khe_sr_kempe.c              Kempe and ejecting task moves     12.3     #
#   khe_sr_requested.c          Requested assignments             12.4.1   #
#   khe_sr_assign_by_history.c  Resource assignment by history    12.4.2   #
#   khe_sr_first_resource.c     Most-constrained-first            12.4.3   #
#   khe_sr_pack_resource.c      Resource packing                  12.4.4   #
#   khe_sr_split_resource.c     Split assignments                 12.4.5   #
#   khe_sr_single_resource.c    Single resource solving           12.5     #
#   khe_sr_dynamic_resource.c   Dynamic resource solving          12.6     #
#   khe_sr_dynamic_vlsn.c       Dynamic resource VLSN solving     12.6     #
#   khe_sr_dynamic_workload.c   Dynamic workload balancing        12.6.8   #
#   khe_sr_resource_matching.c  Resource matching                 12.7.1   #
#   khe_sr_time_sweep.c         Time sweep                        12.7.3   #
#   khe_sr_rematch.c            Resource rematching               12.7.5   #
#   khe_sr_run_homogenize.c     Run homogenization                12.9     #
#   khe_sr_unnecessary.c        Moving unnecessary assignments    12.10    #
#   khe_sr_global_load.c        Global load balancing             12.11    #
#   khe_sr_resource_pair.c      Resource pair repair              12.12    #
#   khe_sr_reassign.c           Resource reassign repair          12.13    #
#   khe_sr_combined.c           Trying unassignments              12.14    #
#   khe_sr_combined.c           Putting it all together           12.15    #
#   ---------------------------------------------------------------------  #
#                                                                          #
#   Obsolete                                                               #
#   khe_sr_fix.c                Fixes and fix makers              12.7     #
#                                                                          #
############################################################################

SOLVER_RESOURCE_HDRS = $(SOLVER_COMMON_HDRS) khe_mmatch.h khe_part_graph.h

SOLVER_RESOURCE_OBJS = khe_sr_invariant.o khe_sr_kempe.o		\
  khe_sr_requested.o khe_sr_assign_by_history.o khe_sr_first_resource.o	\
  khe_sr_pack_resource.o khe_sr_split_resource.o			\
  khe_sr_single_resource.o khe_sr_dynamic_resource.o			\
  khe_sr_dynamic_vlsn.o khe_sr_dynamic_workload.o			\
  khe_sr_resource_matching.o khe_sr_time_sweep.o khe_sr_rematch.o	\
  khe_sr_run_homogenize.o khe_sr_unnecessary.o khe_sr_global_load.o	\
  khe_sr_resource_pair.o khe_sr_reassign.o khe_sr_combined.o


############################################################################
#                                                                          #
#  Ejection chain files                                                    #
#                                                                          #
#  Ejectors and the solvers that use them (Chapter 12).                    #
#                                                                          #
#   File                        Contents                          Guide    #
#   ---------------------------------------------------------------------  #
#   khe_se_ejector.c            Ejectors                          12.2-6   #
#   khe_se_solvers.c            Ejection chain solvers            12.7     #
#   khe_se_focus.c              Focus groupings for ejectors      12.7.2   #
#   ---------------------------------------------------------------------  #
#                                                                          #
############################################################################

SOLVER_EJECTION_HDRS = $(SOLVER_COMMON_HDRS)

SOLVER_EJECTION_OBJS = khe_se_ejector.o khe_se_solvers.o		\
  khe_se_focus.o


############################################################################
#                                                                          #
#   Main rule, for producing archive file "khe_solvers.a"                  #
#                                                                          #
############################################################################

SOLVER_OBJS = $(SOLVER_MISC_OBJS) $(SOLVER_LAYER_TREE_OBJS)		\
  $(SOLVER_TIME_STRUCTURAL_OBJS) $(SOLVER_TIME_OBJS)			\
  $(SOLVER_RESOURCE_STRUCTURAL_OBJS) $(SOLVER_RESOURCE_OBJS)		\
  $(SOLVER_EJECTION_OBJS)

khe_solvers.a : $(SEPARATE_OBJS) $(SOLVER_OBJS)
	rm -f khe_solvers.a
	@ar rcs khe_solvers.a $(SEPARATE_OBJS) $(SOLVER_OBJS)
	@echo "ar rcs khe_solvers.a *.o"


############################################################################
#                                                                          #
#   Rules for object files                                                 #
#                                                                          #
#   Each of these can be compiled separately from the others and from      #
#   KHE generally, except that some might use howard_a.h and howard_n.h.   #
#                                                                          #
############################################################################

HOWARD = ../src_platform/howard_a.h ../src_platform/howard_n.h

khe_wmatch.o : khe_wmatch.h $(HOWARD)
khe_mmatch.o : khe_mmatch.h $(HOWARD)
khe_partition.o : khe_partition.h $(HOWARD)
khe_trie.o : khe_trie.h $(HOWARD)
khe_priqueue.o : khe_priqueue.h $(HOWARD)
khe_part_graph.o : khe_part_graph.h $(HOWARD)

$(SOLVER_LAYER_TREE_OBJS) : $(SOLVER_LAYER_TREE_HDRS)
$(SOLVER_TIME_STRUCTURAL_OBJS) : $(SOLVER_TIME_STRUCTURAL_HDRS)
$(SOLVER_TIME_OBJS) : $(SOLVER_TIME_HDRS)
$(SOLVER_RESOURCE_STRUCTURAL_OBJS) : $(SOLVER_RESOURCE_STRUCTURAL_HDRS)
$(SOLVER_RESOURCE_OBJS) : $(SOLVER_RESOURCE_HDRS)
$(SOLVER_EJECTION_OBJS) : $(SOLVER_EJECTION_HDRS)
$(SOLVER_MISC_OBJS) : $(SOLVER_MISC_HDRS)


############################################################################
#                                                                          #
#   Clean and restart                                                      #
#                                                                          #
############################################################################

clean :
	rm -f *.o

restart : clean
	rm -f khe_solvers.a
