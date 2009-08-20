#!/usr/bin/python

# This is a simple clone of tests/versiontest.cc
import apt_pkg,sys,re,string;
apt_pkg.InitConfig();
apt_pkg.InitSystem();

TestFile = apt_pkg.ParseCommandLine(apt_pkg.Config,[],sys.argv);
if len(TestFile) != 1:
   print "Must have exactly 1 file name";
   sys.exit(0);

# Go over the file.. 
List = open(TestFile[0],"r");
CurLine = 0;
while(1):
   Line = List.readline();
   CurLine = CurLine + 1;
   if Line == "":
      break;
   Line = string.strip(Line);
   if len(Line) == 0 or Line[0] == '#':
      continue;
   
   Split = re.split("[ \n]",Line);

   # Check forward   
   if apt_pkg.VersionCompare(Split[0],Split[1]) != int(Split[2]):
     print "Comparision failed on line %u. '%s' ? '%s' %i != %i"%(CurLine,
             Split[0],Split[1],apt_pkg.VersionCompare(Split[0],Split[1]),
             int(Split[2]));
   # Check reverse
   if apt_pkg.VersionCompare(Split[1],Split[0]) != -1*int(Split[2]):
     print "Comparision failed on line %u. '%s' ? '%s' %i != %i"%(CurLine,
             Split[1],Split[0],apt_pkg.VersionCompare(Split[1],Split[0]),
             -1*int(Split[2]));
