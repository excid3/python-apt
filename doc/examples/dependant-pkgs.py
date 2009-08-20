#!/usr/bin/env python

import apt
import sys

pkgs = set()
cache = apt.Cache()
for pkg in cache:
	candver = cache._depcache.GetCandidateVer(pkg._pkg)
	if candver == None:
		continue
	dependslist = candver.DependsList
	for dep in dependslist.keys():
		# get the list of each dependency object
		for depVerList in dependslist[dep]:
			for z in depVerList:
				# get all TargetVersions of
				# the dependency object
				for tpkg in z.AllTargets():
					if sys.argv[1] == tpkg.ParentPkg.Name:
						pkgs.add(pkg.name)

main = set()
universe = set()
for pkg in pkgs:
	if "universe" in cache[pkg].section:
		universe.add(cache[pkg].sourcePackageName)
	else:
		main.add(cache[pkg].sourcePackageName)

print "main:"		
print "\n".join(main)
print

print "universe:"		
print "\n".join(universe)
