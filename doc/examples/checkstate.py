#!/usr/bin/python
#
#
# this example is not usefull to find out about updated, upgradable packages
# use the depcache.py example for it (because a pkgPolicy is not used here)
#

import apt_pkg
apt_pkg.init()

cache = apt_pkg.GetCache()
packages = cache.Packages

uninstalled, updated, upgradable = {}, {}, {}

for package in packages:
	versions = package.VersionList
	if not versions:
		continue
	version = versions[0]
	for other_version in versions:
		if apt_pkg.VersionCompare(version.VerStr, other_version.VerStr)<0:
			version = other_version
	if package.CurrentVer:
		current = package.CurrentVer
		if apt_pkg.VersionCompare(current.VerStr, version.VerStr)<0:
			upgradable[package.Name] = version
			break
		else:
			updated[package.Name] = current
	else:
		uninstalled[package.Name] = version


for l in (uninstalled, updated, upgradable):
	print l.items()[0]
