#!/usr/bin/env python

import apt

cache = apt.Cache()

for pkg in cache:
   if not pkg.candidateRecord:
      continue
   if pkg.candidateRecord.has_key("Task"):
      print "Pkg %s is part of '%s'" % (pkg.name, pkg.candidateRecord["Task"].split())
      #print pkg.candidateRecord
