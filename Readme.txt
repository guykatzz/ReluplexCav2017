*** Reluplex, May 2017 ***

This repository contains the proof-of-concept implementation of the
Reluplex algorithm, as described in the paper:

   G. Katz, C. Barrett, D. Dill, K. Julian and
   M. Kochenderfer. Reluplex: An Efficient SMT Solver for Verifying
   Deep Neural Networks. Proc. 29th Int. Conf. on Computer Aided
   Verification (CAV). Heidelberg, Germany, July 2017.

The paper (with its supplementary material) may be found at:

    https://arxiv.org/abs/1702.01135

This file contains instructions for compiling Reluplex and for running
the experiments described in the paper, and also some information on
the Reluplex code and the various folders.

Compilation Instructions
------------------------

The implementation was run and tested on Ubuntu 16.04.

Compiling GLPK:

	  cd glpk-4.60
	  ./configure_glpk.sh
	  make
	  make install

Compiling the Reluplex core:

	  cd reluplex
	  make

Compiling the experiments:

	  cd check_properties
	  make


Running the experiments
-----------------------

The paper describes 3 categories of experiments:

  (i) Experiments comparing Reluplex to the SMT solvers CVC4, Z3,
  Yices and Mathsat, and to the LP solver Gurobi.

  (ii) Using Reluplex to check 10 desirable properties of the ACAS
  Xu networks.

  (iii) Using Reluplex to evaluate the local adversarial robustness of
  one of the ACAS Xu networks.

This repository contains the code for all experiments in categories
(ii) and (iii). All experiments are run using simple scripts,
provided in the "scripts" folder. The results will appear in the
"logs" folder. In order to run an experiment:

       - Navigate to the main directory
       - Run the experiment of choice: ./scripts/run_XYZ.sh
       - Find the results under the "logs" folder.

A Reluplex execution generates two kinds of logs. The first is the
summary log, in which each query to Reluplex is summarized by a line like this:

./nnet/ACASXU_run2a_2_3_batch_2000.nnet, SAT, 12181, 00:00:12, 37, 39

The fields in each line in the summary file are:
 - The network being tested
 - Result (SAT/UNSAT/TIMEOUT/ERROR)
 - Time in milliseconds
 - Time in HH:MM:SS format
 - Maximal stack depth reached
 - Number of visited states

Summary logs will always have the word "summary" in their
names. Observe that a single experiment may involve multiple networks,
or multiple queries on the same network - so a single summary file may
contain multiple lines. Also, note that these files are only
created/updated when a query finishes, so they will likely appear
only some time after an experiment has started.

The second kind of log file that will appear under the "logs" folder
is the statistics log. These logs will have the word "stats" in their
names, and will contain statistics that Reluplex prints roughly every
500 iterations of its main loop. These logs will appear immediately
when the experiment starts. Unlike summary logs which may summarize
multiple queries to Reluplex, each statistics log describes just a
single query. Consequently, there will be many of these logs (a
single experiment may generate as many as 45 of these logs). The log
names will typically indicate the specific query that generated them;
for example, "property2_stats_3_9.txt" signifies a statistics log that
was generated for property 2, when checked on the network indexed by
(3,9).

- Using Reluplex to check 10 desirable properties of the ACAS Xu
networks:

These 10 benchmarks are organized under the "check_properties" folder,
in numbered folders. Property 6 is checked in two parts, and
so it got two folders: property6a and property6b. Within these
folders, the properties are given in Reluplex format (in main.cpp).

Recall that checking is done by negating the property. For instance,
in order to prove that x > 50, we tell Reluplex to check the
satisfiability x <= 50, and expect an UNSAT result.

Also, some properties are checked in parts. For example, the ACAS Xu
networks have 5 outputs, and we often wish to prove that one of them
is minimal. We check this by querying Reluplex 4 times, each time
asking it to check whether it is possible that one of the other
outputs is smaller than our target output. In this case, success is
indicated by 4 UNSAT answers.

To run Reluplex, execute the "run_peopertyi.sh" scripts, for i between
1 and 10. The result summaries will appear under
"logs/propertyi_summary.txt". The statistics from each individual
query to Reluplex will also appear under the logs folder.

- Using Reluplex to evaluate the local adversarial robustness of one of
the ACAS Xu networks:

These experiments include evaluating the adversarial robustness of one
of the ACAS Xu networks on 5 arbitrary points. Evaluating each point
involves invoking Reluplex 5 times, with different delta sizes.
The relevant Reluplex code appears under
"check_properties/adversarial/main.cpp". To execute it (after
compiling it), run the "scripts/run_adversarial.sh" script. The
summary of the results will appear as "logs/adversarial_summary.txt",
and the Reluplex statistics will appear under
"logs/adversarial_stats.txt" (the statistics for all queries will
appear under the same file).

Checking the adversarial robustness at point x for a fixed delta is
done as follows:

  - Identify the minimal output at point x
  - For each of the other 4 outputs, do:
  -   Invoke Reluplex to look for a point x' that is at most delta
      away from x, for which the other point is minimal

If the test fails for all 4 other outputs (4 UNSAT results), the
network is robust at x; otherwise, it is not. As soon as one SAT
result is found we stop the test.


Information regarding the Reluplex code
---------------------------------------

The main components of the tool are:

1. reluplex/Reluplex.h:
   The main class, implementing the core Reluplex algorithm.

2. glpk-4.60: (folder)
   The glpk open-source LP solver, plus some modifications.
   The patches applied to the pristine GLPK appear under the "glpk_patch"
   folder (no need to re-apply them).

3. reluplex/SmtCore.h:
   A simple SmtCore, in charge of the search stack.

4. nnet/:
   This folder contains the ACAS Xu neural networks that we used, and
   code for loading and accessing them.

Below are additional details about the Reluplex core.

An example of how to use Reluplex.h is provided in
reluplex/RunReluplex.h. This example is the one described in the
paper. It shows how a client specifies the number of variables for the
Reluplex class, which variables are auxiliary variables (via the
markBasic() method), which are ReLU pairs (via setReluPair()), the
lower and upper bounds and the initial tableau values. A call to the
solve() method then starts Reluplex.

Inside the Reluplex class, the main loop is implemented in the
progress() method. This loop:

  A. Invokes GLPK in order to fix any out-of-bounds variables
  B. If all variables are within bounds, attempts to fix a broken ReLU
     constraint.

If progress() is successful (i.e., progress is made), it returns
true. Otherwise the problem is infeasible, and progress() returns
false. In this case the caller function, solve(), will have the SmtCore
pop a previous decision, or return UNSAT if there are none to pop.


Important member variables of the Reluplex class:
  _tableau: the tableau.

  _preprocessedTableau: the original tableau (after some
   preprocessing), used for restoring the tableau in certain cases.

  _upperBounds, _lowerBounds: the current variable bounds.

  _assignment: the current assignment.

  _basicVariables: the set of variables that are currently basic.

  _reluPairs: all pairs of variables specified as ReLU pairs.

  _dissolvedReluVariables: ReLU pairs that have been eliminated.


Some of the notable methods within the Reluplex class:
  - pivot():
    A method for pivoting two variables in the tableau.

  - update():
    A method for updating the current assignment.

  - updateUpperBound(), updateLowerBound():
    Methods for tightening the upper and lower bounds of a variable.
    These methods eliminate ReLU connections when certain bounds are
    discovered (e.g., a strictly positive lower bound for a ReLU
    variable), as discussed in the paper.

  - unifyReluPair():
    A method for eliminating a ReLU pair by fixing it to the active
    state. This is performed by merging the two variables in the
    tableau: the method ensures that both variables are equal and
    non-basic, and then eliminates the b variable from the tableau and
    replaces it with the f variable.

  - fixOutOfBounds():
    A method for invoking GLPK in order to fix all out-of-bound
    variables. The method translates the current tableau into a GLPK
    instance, invokes GLPK, and then extracts the solution tableau and
    assignment from GLPK.

  - fixBrokenRelu():
    A method for fixing a specific ReLU constraint that is currently
    broken. We first try to fix the b variable to agree
    with the f variable, and only if that fails attempt to fix f to
    agree with b. If both fail, the problem is infeasible, and the
    method returns false. Otherwise, the pair is fixed, and the method
    returns true.

  - storeGlpkBoundTightening():
    As GLPK searches for feasible solutions, certain rows of the
    tableaus that it explores are used to derive tighter variable bounds.
    (see paper). These new bounds are stored (but not applied) as GLPK
    runs. When GLPK terminates, all the new bounds are applied.

  - performGlpkBoundTightening():
    The method that actually performs bound tightening, as stored by
    storeGlpkBoundTightening().

  - findPivotCandidate():
    A method for finding variables that afford slack for a certain
    basic variable that needs to be increased or decreased.

  - restoreTableauFromBackup():
    A method for restoring the current tableau from the original
    tableau. This is done, for example, when the round-off degradation
    is discovered to be too great (see paper).


Other points of interest in the Reluplex class:
  - The under-approximation technique discussed in the paper has been
    implemented, although it is turned off by default. To turn it on,
    call toggleAlmostBrokenReluEliminiation( true ) when setting up
    Reluplex. This affects the way ReLUs are eliminated within the
    updateLowerBound() and updateUpperBound() methods.

  - Conflict analysis (see paper) is performed as part of bound
    tightening operations. Specifically, when bound tightening leads
    to a lower bound becoming greater than an upper bound, an
    InvariantViolationError exception is raised. The parameter to that
    error, i.e. the "violatingStackLevel", indicates the last split
    that led to the current violation. This indicates how many
    decisions in the stack need to be undone by the SmtCore.


Additional classes under the "reluplex" folder:

  - FloatUtils: utilities for comparing floating point numbers.

  - Tableau: a linked-list implementation of Reluplex's tableau.

  - ReluPairs: a simple data structure for keeping information about ReLU pairs.

  - RunReluplex: a small test-harness for Reluplex. Contains 2 small examples.

The "common" folder contains general utility classes.
