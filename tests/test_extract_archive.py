#!/usr/bin/python

import apt
import apt_inst
import os
import sys

print os.getcwd()
apt_inst.debExtractArchive(open(sys.argv[1]), "/tmp/")
print os.getcwd()
