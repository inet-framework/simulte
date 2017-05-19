INSTALLATION INSTRUCTIONS
=========================

The SimuLTE Framework can be compiled on any platform supported by the INET Framework.


Prerequisites
-------------

You should have a 
- working OMNeT++ (v5.1) installation. (Download from http://omnetpp.org)
- working INET-Framework installation (v3.5). (Download from http://inet.omnetpp.org)

NOTE: OMNeT++ 5.0 paired with INET 3.4 is also working.

Make sure your OMNeT++ installation works OK (e.g. try running the samples)
and it is in the path (to test, try the command "which nedtool"). On
Windows, open a console with the "mingwenv.cmd" command. The PATH and other
variables will be automatically adjusted for you. Use this console to compile
and run INET and SimuLTE.

Install and test INET according to the installation instructions found in the archive.
Be sure to check if the INET examples are running fine before continuing.


Building SimuLTE from the IDE
-----------------------------

1. Extract the downloaded SimuLTE tarball next to the INET directory
   (i.e. into your workspace directory, if you are using the IDE).

2. Start the IDE, and ensure that the 'inet' project is open and correctly built.

3. Import the project using: File | Import | General | Existing projects into Workspace.
   Then select the workspace dir as the root directory, and be sure NOT to check the
   "Copy projects into workspace" box. Click Finish.

4. You can build the project by pressing CTRL-B (Project | Build all)

5. To run an example from the IDE, select the simulation example's folder under 
   'simulations', and click 'Run' on the toolbar.


Building SimuLTE from the command line
--------------------------------------

1. Extract the downloaded SimuLTE tarball next to the INET directory

2. Change to the simulte directory.

3. Type "make makefiles". This should generate the makefiles.

4. Type "make" to build the simulte executable (debug version). Use "make MODE=release"
   to build release version.

5. You can run examples by changing into a directory under 'simulations', and 
   executing "./run"


Enjoy, 
The SimuLTE Team
