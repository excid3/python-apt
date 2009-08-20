#!/usr/bin/python

import unittest
import apt_pkg

class testHashes(unittest.TestCase):
    " test the hashsum functions against strings and files "

    def testMD5(self):
        # simple
        s = "foo"
        s_md5 = "acbd18db4cc2f85cedef654fccc4a4d8"
        res = apt_pkg.md5sum(s)
        self.assert_(res == s_md5)
        # file
        res = apt_pkg.md5sum(open("hashsum_test.data"))
        self.assert_(res == s_md5)
        # with zero (\0) in the string
        s = "foo\0bar"
        s_md5 = "f6f5f8cd0cb63668898ba29025ae824e"
        res = apt_pkg.md5sum(s)
        self.assert_(res == s_md5)
        # file
        res = apt_pkg.md5sum(open("hashsum_test_with_zero.data"))
        self.assert_(res == s_md5)

    def testSHA1(self):
        # simple
        s = "foo"
        s_hash = "0beec7b5ea3f0fdbc95d0dd47f3c5bc275da8a33"
        res = apt_pkg.sha1sum(s)
        self.assert_(res == s_hash)
        # file
        res = apt_pkg.sha1sum(open("hashsum_test.data"))
        self.assert_(res == s_hash)
        # with zero (\0) in the string
        s = "foo\0bar"
        s_hash = "e2c300a39311a2dfcaff799528415cb74c19317f"
        res = apt_pkg.sha1sum(s)
        self.assert_(res == s_hash)
        # file
        res = apt_pkg.sha1sum(open("hashsum_test_with_zero.data"))
        self.assert_(res == s_hash)

    def testSHA256(self):
        # simple
        s = "foo"
        s_hash = "2c26b46b68ffc68ff99b453c1d30413413422d706483bfa0f98a5e886266e7ae"
        res = apt_pkg.sha256sum(s)
        self.assert_(res == s_hash)
        # file
        res = apt_pkg.sha256sum(open("hashsum_test.data"))
        self.assert_(res == s_hash)
        # with zero (\0) in the string
        s = "foo\0bar"
        s_hash = "d6b681bfce7155d44721afb79c296ef4f0fa80a9dd6b43c5cf74dd0f64c85512"
        res = apt_pkg.sha256sum(s)
        self.assert_(res == s_hash)
        # file
        res = apt_pkg.sha256sum(open("hashsum_test_with_zero.data"))
        self.assert_(res == s_hash)

if __name__ == "__main__":
    unittest.main()
