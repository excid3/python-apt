#!/usr/bin/env python2.4
#
# Test for the pkgCache code
# 

import apt_pkg
import sys

def main():
	apt_pkg.init()
	cache = apt_pkg.GetCache()
	depcache = apt_pkg.GetDepCache(cache)
	depcache.Init()
	i=0
	all=cache.PackageCount
	print "Running Cache test on all packages:"
	# first, get all pkgs
	for pkg in cache.Packages:
		i += 1
		x = pkg.Name
		# then get each version
		for ver in pkg.VersionList:
			# get some version information
			a = ver.FileList
			b = ver.VerStr
			c = ver.Arch
			d = ver.DependsListStr
			dl = ver.DependsList
			# get all dependencies (a dict of string->list, 
			# e.g. "depends:" -> [ver1,ver2,..]
			for dep in dl.keys():
				# get the list of each dependency object
				for depVerList in dl[dep]:
					for z in depVerList:
						# get all TargetVersions of
						# the dependency object
						for j in z.AllTargets():
							f = j.FileList
							g = ver.VerStr
							h = ver.Arch
							k = ver.DependsListStr
							j = ver.DependsList
							pass
				
		print "\r%i/%i=%.3f%%    " % (i,all,(float(i)/float(all)*100)),

if __name__ == "__main__":
	main()
	sys.exit(0)
