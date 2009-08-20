#!/usr/bin/python

import apt_pkg
apt_pkg.init()

cache = apt_pkg.GetCache()

class Wanted:

	def __init__(self, name):
		self.name = name
		self.recommended = []
		self.suggested = []

wanted = {}

for package in cache.Packages:
	current = package.CurrentVer
	if not current:
		continue
	depends = current.DependsList
	for (key, attr) in (('Suggests', 'suggested'), 
	                    ('Recommends', 'recommended')):
		list = depends.get(key, [])
		for dependency in list:
			name = dependency[0].TargetPkg.Name
			dep = cache[name]
			if dep.CurrentVer:
				continue
			getattr(wanted.setdefault(name, Wanted(name)),
			        attr).append(package.Name)

ks = wanted.keys()
ks.sort()

for want in ks:
	print want, wanted[want].recommended, wanted[want].suggested




