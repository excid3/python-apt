#!/usr/bin/env python

import sys
import apt


def dependencies(cache, pkg, deps, key="Depends"):
    #print "pkg: %s (%s)" % (pkg.name, deps)
    candver = cache._depcache.GetCandidateVer(pkg._pkg)
    if candver == None:
        return deps
    dependslist = candver.DependsList
    if dependslist.has_key(key):
        for depVerList in dependslist[key]:
            for dep in depVerList:
                if cache.has_key(dep.TargetPkg.Name):
                    if pkg.name != dep.TargetPkg.Name and not dep.TargetPkg.Name in deps:
                        deps.add(dep.TargetPkg.Name)
                        dependencies(cache, cache[dep.TargetPkg.Name], deps, key)
    return deps


pkgname = sys.argv[1]
c = apt.Cache()
pkg = c[pkgname]

deps = set()

deps = dependencies(c,pkg, deps, "Depends")
print " ".join(deps)

preDeps = set()
preDeps = dependencies(c,pkg, preDeps, "PreDepends")
print " ".join(preDeps)
