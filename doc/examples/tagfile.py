#!/usr/bin/env python
import apt_pkg

Parse = apt_pkg.ParseTagFile(open("/var/lib/dpkg/status","r"));

while Parse.Step() == 1:
   print Parse.Section.get("Package");
   print apt_pkg.ParseDepends(Parse.Section.get("Depends",""));
