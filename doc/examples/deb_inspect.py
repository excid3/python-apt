#!/usr/bin/env python
# some example for apt_inst

import apt_pkg
import apt_inst
import sys
import os.path

def Callback(What,Name,Link,Mode,UID,GID,Size,MTime,Major,Minor):
    """ callback for debExtract """
    
    print "%s '%s','%s',%u,%u,%u,%u,%u,%u,%u"\
          % (What,Name,Link,Mode,UID,GID,Size, MTime, Major, Minor);


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "need filename argumnet"
        sys.exit(1)
    file = sys.argv[1]

    print "Working on: %s" % file
    print "Displaying data.tar.gz:"
    apt_inst.debExtract(open(file), Callback, "data.tar.gz")

    print "Now extracting the control file:"
    control = apt_inst.debExtractControl(open(file))
    sections = apt_pkg.ParseSection(control)

    print "Maintainer is: "
    print sections["Maintainer"]

    print
    print "DependsOn: "
    depends = sections["Depends"]
    print apt_pkg.ParseDepends(depends)


    print "extracting archive"
    dir = "/tmp/deb"
    os.mkdir(dir)
    apt_inst.debExtractArchive(open(file),dir)
    def visit(arg, dirname, names):
        print "%s/" % dirname
        for file in names:
            print "\t%s" % file
    os.path.walk(dir, visit, None)
