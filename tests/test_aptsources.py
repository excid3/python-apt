#!/usr/bin/env python

import unittest
import apt_pkg
import os
import copy

import sys
sys.path.insert(0, "../")
import aptsources
import aptsources.sourceslist
import aptsources.distro

class TestAptSources(unittest.TestCase):
    def __init__(self, methodName):
        unittest.TestCase.__init__(self, methodName)
        apt_pkg.init()
        apt_pkg.Config.Set("Dir::Etc", os.getcwd())
        apt_pkg.Config.Set("Dir::Etc::sourceparts","/xxx")

    def testIsMirror(self):
        self.assertTrue(aptsources.sourceslist.is_mirror("http://archive.ubuntu.com",
                                                         "http://de.archive.ubuntu.com"))
        self.assertFalse(aptsources.sourceslist.is_mirror("http://archive.ubuntu.com",
                                                          "http://ftp.debian.org"))

    def testSourcesListReading(self):
        apt_pkg.Config.Set("Dir::Etc::sourcelist","data/sources.list")
        sources = aptsources.sourceslist.SourcesList()
        self.assertEqual(len(sources.list), 6)
        # test load
        sources.list = []
        sources.load("data/sources.list")
        self.assertEqual(len(sources.list), 6)

    def testSourcesListAdding(self):
        apt_pkg.Config.Set("Dir::Etc::sourcelist","data/sources.list")
        sources = aptsources.sourceslist.SourcesList()
        # test to add something that is already there (main)
        before = copy.deepcopy(sources)
        sources.add("deb","http://de.archive.ubuntu.com/ubuntu/",
                    "edgy",
                    ["main"])
        self.assertTrue(sources.list == before.list)
        # test to add something that is already there (restricted)
        before = copy.deepcopy(sources)
        sources.add("deb","http://de.archive.ubuntu.com/ubuntu/",
                    "edgy",
                    ["restricted"])
        self.assertTrue(sources.list == before.list)
        # test to add something new: multiverse
        sources.add("deb","http://de.archive.ubuntu.com/ubuntu/",
                    "edgy",
                    ["multiverse"])
        found = False
        for entry in sources:
            if (entry.type == "deb" and
                entry.uri == "http://de.archive.ubuntu.com/ubuntu/" and
                entry.dist == "edgy" and
                "multiverse" in entry.comps):
                found = True
        self.assertTrue(found)
        # test to add something new: multiverse *and* 
        # something that is already there
        before = copy.deepcopy(sources)
        sources.add("deb","http://de.archive.ubuntu.com/ubuntu/",
                    "edgy",
                    ["universe", "something"])
        found_universe = 0
        found_something = 0
        for entry in sources:
            if (entry.type == "deb" and
                entry.uri == "http://de.archive.ubuntu.com/ubuntu/" and
                entry.dist == "edgy"):
                for c in entry.comps:
                    if c == "universe":
                        found_universe += 1
                    if c == "something":
                        found_something += 1
        #print "\n".join([s.str() for s in sources])
        self.assertEqual(found_something, 1)
        self.assertEqual(found_universe, 1)

    def testMatcher(self):
        apt_pkg.Config.Set("Dir::Etc::sourcelist","data/sources.list.testDistribution")
        sources = aptsources.sourceslist.SourcesList()
        distro = aptsources.distro.get_distro()
        distro.get_sources(sources)
        # test if all suits of the current distro were detected correctly
        dist_templates = set()
        for s in sources:
            if not s.template:
                self.fail("source entry '%s' has no matcher" % s)

    def testDistribution(self):
        apt_pkg.Config.Set("Dir::Etc::sourcelist","data/sources.list.testDistribution")
        sources = aptsources.sourceslist.SourcesList()
        distro = aptsources.distro.get_distro()
        distro.get_sources(sources)
        # test if all suits of the current distro were detected correctly
        dist_templates = set()
        for s in sources:
            if s.template:
                dist_templates.add(s.template.name)
        #print dist_templates
        for d in ["hardy","hardy-security","hardy-updates","intrepid","hardy-backports"]:
            self.assertTrue(d in dist_templates)
        # test enable 
        comp = "restricted"
        distro.enable_component(comp)
        found = {}
        for entry in sources:
            if (entry.type == "deb" and
                entry.uri == "http://de.archive.ubuntu.com/ubuntu/" and
                "edgy" in entry.dist):
                for c in entry.comps:
                    if c == comp:
                        if not found.has_key(entry.dist):
                            found[entry.dist] = 0
                        found[entry.dist] += 1
        #print "".join([s.str() for s in sources])
        for key in found:
            self.assertEqual(found[key], 1)

        # add a not-already available component
        comp = "multiverse"
        distro.enable_component(comp)
        found = {}
        for entry in sources:
            if (entry.type == "deb" and
                entry.template and
                entry.template.name == "edgy"):
                for c in entry.comps:
                    if c == comp:
                        if not found.has_key(entry.dist):
                            found[entry.dist] = 0
                        found[entry.dist] += 1
        #print "".join([s.str() for s in sources])
        for key in found:
            self.assertEqual(found[key], 1)

if __name__ == "__main__":
    unittest.main()
