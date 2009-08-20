#!/usr/bin/env python2.4
#
# Test for the pkgCache code
# 

import apt_pkg
import sys, os


if __name__ == "__main__":
    lock = "/tmp/test.lck"
    
    apt_pkg.init()

    # system-lock
    apt_pkg.PkgSystemLock()

    pid = os.fork()
    if pid == 0:
        try:
            apt_pkg.PkgSystemLock()
        except SystemError, s:
            print "Can't get lock: (error text:\n%s)" % s
	sys.exit(0)

    apt_pkg.PkgSystemUnLock()

    # low-level lock
    fd = apt_pkg.GetLock(lock,True)
    print "Lockfile fd: %s" % fd

    # try to get lock without error flag
    pid = os.fork()
    if pid == 0:
        # child
        fd = apt_pkg.GetLock(lock,False)
        print "Lockfile fd (child): %s" % fd
	sys.exit(0)

    # try to get lock with error flag
    pid = os.fork()
    if pid == 0:
        # child
        fd = apt_pkg.GetLock(lock,True)
        print "Lockfile fd (child): %s" % fd
	sys.exit(0)
    
