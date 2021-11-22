#
# pydoorlock, the python library for doorlock
#
# Author:
#  Ralf Ramsauer <ralf@binary-kitchen.de>
#
# Copyright (c) Binary Kitchen, 2018-2021
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.
#

from setuptools import setup, find_packages

with open("VERSION") as version_file:
    version = version_file.read().lstrip("v").rstrip()

setup(name="pydoorlock", version=version,
      description="A Python interface for doorlock",
      license="GPLv2", url="https://github.com/Binary-Kitchen/doorlockd/",
      author_email="ralf@binary-kitchen.de",
      packages=find_packages())
