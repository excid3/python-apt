#!/usr/bin/env python2.4
#
# Test for the PkgSrcRecords code
# it segfaults for python-apt < 0.5.37
# 

import apt_pkg
import sys

def main():
	apt_pkg.init()
	cache = apt_pkg.GetCache()
	i=0
	print "Running PkgSrcRecords test on all packages:"
	for x in cache.Packages:
		i += 1
		src = apt_pkg.GetPkgSrcRecords()
		if src.Lookup(x.Name):
			#print src.Package
			pass
		print "\r%i/%i=%.3f%%    " % (i,cache.PackageCount, (float(i)/float(cache.PackageCount)*100)),

if __name__ == "__main__":
	main()
	sys.exit(0)
