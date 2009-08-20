#!/usr/bin/env python
#
# a example that prints the URIs of all upgradable packages
#

import apt
import apt_pkg


cache = apt.Cache()
upgradable = filter(lambda p: p.isUpgradable, cache)


for pkg in upgradable:
	pkg._lookupRecord(True)
	path = apt_pkg.ParseSection(pkg._records.Record)["Filename"]
	cand = pkg._depcache.GetCandidateVer(pkg._pkg)
	for (packagefile,i) in cand.FileList:
		indexfile = cache._list.FindIndex(packagefile)
		if indexfile:
			uri = indexfile.ArchiveURI(path)
			print uri
