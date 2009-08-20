#!/usr/bin/python
# Example demonstrating how to use the configuration/commandline system
# for object setup.

# This parses the given config file in 'ISC' style where the sections
# represent object instances and shows how to iterate over the sections.
# Pass it the sample apt-ftparchive configuration,
# doc/examples/ftp-archive.conf
# or a bind8 config file..

import apt_pkg,sys,posixpath;

ConfigFile = apt_pkg.ParseCommandLine(apt_pkg.Config,[],sys.argv);

if len(ConfigFile) != 1:
   print "Must have exactly 1 file name";
   sys.exit(0);

Cnf = apt_pkg.newConfiguration();
apt_pkg.ReadConfigFileISC(Cnf,ConfigFile[0]);

# Print the configuration space
#print "The Configuration space looks like:";
#for I in Cnf.keys():
#   print "%s \"%s\";"%(I,Cnf[I]);

# bind8 config file..
if Cnf.has_key("Zone"):
   print "Zones: ",Cnf.SubTree("zone").List();
   for I in Cnf.List("zone"):
      SubCnf = Cnf.SubTree(I);
      if SubCnf.Find("type") == "slave":
         print "Masters for %s: %s"%(SubCnf.MyTag(),SubCnf.ValueList("masters"));
else:
   print "Tree definitions:";
   for I in Cnf.List("tree"):
      SubCnf = Cnf.SubTree(I);
      # This could use Find which would eliminate the possibility of exceptions.
      print "Subtree %s with sections '%s' and architectures '%s'"%(SubCnf.MyTag(),SubCnf["Sections"],SubCnf["Architectures"]);
