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
	depcache = apt_pkg.GetDepCache(cache)
	depcache.Init()
	i=0
	print "Running PkgRecords test on all packages:"
	for pkg in cache.Packages:
		i += 1
		records = apt_pkg.GetPkgRecords(cache)
		if len(pkg.VersionList) == 0:
			#print "no available version, cruft"
			continue
		version = depcache.GetCandidateVer(pkg)
		if not version:
			continue
		file, index = version.FileList.pop(0)
		if records.Lookup((file,index)):
			#print records.FileName
			x = records.FileName
			y = records.LongDesc
			pass
		print "\r%i/%i=%.3f%%    " % (i,cache.PackageCount, (float(i)/float(cache.PackageCount)*100)),

if __name__ == "__main__":
	main()
	sys.exit(0)
