"""
Doorlockd -- Binary Kitchen's smart door opener

Copyright (c) Binary Kitchen e.V., 2018

Author:
  Ralf Ramsauer <ralf@binary-kitchen.de>
  Thomas Schmid <tom@binary-kitchen.de>

This work is licensed under the terms of the GNU GPL, version 2.  See
the LICENSE file in the top-level directory.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.
"""

from configparser import ConfigParser
from os.path import join

class Config:
    config_topic = 'doorlock'

    def __init__(self, sysconfdir):
        self.config = ConfigParser()
        self.config.read([join(sysconfdir, 'doorlockd.default.cfg'),
                          join(sysconfdir, 'doorlockd.cfg')])

    def boolean(self, key):
        return self.config.getboolean(self.config_topic, key)

    def str(self, key):
        return self.config.get(self.config_topic, key)

    def int(self,key):
        return self.config.getint(self.config_topic, key)