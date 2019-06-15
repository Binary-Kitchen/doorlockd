"""
Doorlockd -- Binary Kitchen's smart door opener

Copyright (c) Binary Kitchen e.V., 2018-2019

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

import functools
from configparser import ConfigParser
from os.path import join


SYSCONFDIR = './etc'
PREFIX = '.'

root_prefix = join(PREFIX, 'share', 'doorlockd')
sounds_prefix = join(root_prefix, 'sounds')


def check_exists(func):
    @functools.wraps(func)
    def decorator(*args, **kwargs):
        config = args[0]
        if not config.config.has_option(config.config_topic, args[1]):
            return None
        return func(*args, **kwargs)
    return decorator


class Config:
    def __init__(self, config_topic):
        self.config_topic = config_topic
        self.config = ConfigParser()
        self.config.read(join(SYSCONFDIR, 'doorlockd.cfg'))

    @check_exists
    def boolean(self, key):
        return self.config.getboolean(self.config_topic, key)

    @check_exists
    def str(self, key):
        return self.config.get(self.config_topic, key)

    @check_exists
    def int(self,key):
        return self.config.getint(self.config_topic, key)
