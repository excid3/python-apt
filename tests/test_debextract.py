#!/usr/bin/python

import apt_inst
import sys

def Callback(What,Name,Link,Mode,UID,GID,Size,MTime,Major,Minor):
    print "%s '%s','%s',%u,%u,%u,%u,%u,%u,%u" % (
        What,Name,Link,Mode,UID,GID,Size, MTime, Major, Minor)

member = "data.tar.lzma" 
if len(sys.argv) > 2:
    member = sys.argv[2]
apt_inst.debExtract(open(sys.argv[1]), Callback, member)
