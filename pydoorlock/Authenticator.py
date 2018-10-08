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
        if user not in self._local_db:
            return DoorlockResponse.Perm

        stored_pw = self._local_db[user][0]
        stored_salt = self._local_db[user][1]
        if stored_pw == hashlib.sha256(stored_salt.encode() + password.encode()).hexdigest():
            return DoorlockResponse.Success

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
        return DoorlockResponse.Success

    def try_auth(self, credentials):
        if self._simulate:
            log.info('SIMULATION MODE! ACCEPTING ANYTHING!')
            return DoorlockResponse.Success

        method = credentials[0]
        if method not in self._backends:
            return DoorlockResponse.InternalError

        if method == AuthMethod.LDAP_USER_PW:
            return self._try_auth_ldap(credentials[1], credentials[2])
        elif method == AuthMethod.LOCAL_USER_DB:
            return self._try_auth_local(credentials[1], credentials[2])

        return DoorlockResponse.InternalError
