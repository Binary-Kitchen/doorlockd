"""
Doorlockd -- Binary Kitchen's smart door opener

Copyright (c) Binary Kitchen e.V., 2018

Author:
  Ralf Ramsauer <ralf@binary-kitchen.de>

This work is licensed under the terms of the GNU GPL, version 2.  See
the LICENSE file in the top-level directory.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.
"""

import hashlib
import ldap
import logging

from enum import Enum

from .Doorlock import DoorlockResponse

log = logging.getLogger()


class AuthMethod(Enum):
    LDAP_USER_PW = 1
    LOCAL_USER_DB = 2

    def __str__(self):
        if self == AuthMethod.LDAP_USER_PW:
            return 'LDAP'
        elif self == AuthMethod.LOCAL_USER_DB:
            return 'Local'
        return 'Error'


class Authenticator:
    def __init__(self, cfg):
        self._simulate = cfg.boolean('SIMULATE_AUTH')
        self._backends = set()

        f_blacklist = cfg.str('USER_BLACKLIST')
        self._user_blacklist = set()
        if f_blacklist:
            with open(f_blacklist, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line.startswith('#'):
                        continue
                    if line:
                        self._user_blacklist.add(line)

        if self._simulate:
            return

        self._ldap_uri = cfg.str('LDAP_URI')
        self._ldap_binddn = cfg.str('LDAP_BINDDN')
        if self._ldap_uri and self._ldap_binddn:
            log.info('Initialising LDAP auth backend')
            self._backends.add(AuthMethod.LDAP_USER_PW)
            ldap.set_option(ldap.OPT_X_TLS_REQUIRE_CERT, ldap.OPT_X_TLS_DEMAND)
            ldap.set_option(ldap.OPT_REFERRALS, 0)

        file_local_db = cfg.str('LOCAL_USER_DB')
        if file_local_db:
            log.info('Initialising local auth backend')
            self._local_db = dict()

            with open(file_local_db, 'r') as f:
                for line in f:
                    line = line.split()
                    user = line[0]
                    pwd = line[1].split(':')
                    self._local_db[user] = pwd

            self._backends.add(AuthMethod.LOCAL_USER_DB)

    @property
    def backends(self):
        return self._backends

    def _try_auth_local(self, user, password):
        log.info('  Trying to local auth (user, password) as user %s', user)
        if user not in self._local_db:
            log.info('  No user %s in local database', user)
            return DoorlockResponse.Perm

        stored_pw = self._local_db[user][0]
        stored_salt = self._local_db[user][1]
        if stored_pw == hashlib.sha256(stored_salt.encode() + password.encode()).hexdigest():
            log.info('  Authenticated as user %s', user)
            return DoorlockResponse.Success

        log.info('  Invalid credentials')
        return DoorlockResponse.Perm

    def _try_auth_ldap(self, user, password):
        log.info('  Trying to LDAP auth (user, password) as user %s', user)
        ldap_username = self._ldap_binddn % user
        try:
            l = ldap.initialize(self._ldap_uri)
            l.simple_bind_s(ldap_username, password)
            l.unbind_s()
        except ldap.INVALID_CREDENTIALS:
            log.info('  Invalid credentials')
            return DoorlockResponse.Perm
        except ldap.LDAPError as e:
            log.info('  LDAP Error: %s' % e)
            return DoorlockResponse.InternalError
        log.info('  Authenticated as user %s', user)
        return DoorlockResponse.Success

    def try_auth(self, credentials):
        user, password = credentials

        if user in self._user_blacklist:
            return DoorlockResponse.Perm

        if self._simulate:
            log.info('SIMULATION MODE! ACCEPTING ANYTHING!')
            return DoorlockResponse.Success
        if AuthMethod.LDAP_USER_PW in self._backends:
            retval = self._try_auth_ldap(user, password)
            if retval == DoorlockResponse.Success:
                return retval
        if AuthMethod.LOCAL_USER_DB in self._backends:
            return self._try_auth_local(user, password)
        return DoorlockResponse.Perm
