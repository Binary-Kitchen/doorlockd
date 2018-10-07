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
from random import sample

log = logging.getLogger()

# copied from sudo
eperm_insults = {
        'Wrong!  You cheating scum!',
        'And you call yourself a Rocket Scientist!',
        'No soap, honkie-lips.',
        'Where did you learn to type?',
        'Are you on drugs?',
        'My pet ferret can type better than you!',
        'You type like i drive.',
        'Do you think like you type?',
        'Your mind just hasn\'t been the same since the electro-shock, has it?',
        'Maybe if you used more than just two fingers...',
        'BOB says:  You seem to have forgotten your passwd, enter another!',
        'stty: unknown mode: doofus',
        'I can\'t hear you -- I\'m using the scrambler.',
        'The more you drive -- the dumber you get.',
        'Listen, broccoli brains, I don\'t have time to listen to this trash.',
        'I\'ve seen penguins that can type better than that.',
        'Have you considered trying to match wits with a rutabaga?',
        'You speak an infinite deal of nothing',
}

def choose_insult():
    return(sample(eperm_insults, 1)[0])


class AuthMethod(Enum):
    LDAP_USER_PW = 1
    LOCAL_USER_DB = 2

    def __str__(self):
        if self == AuthMethod.LDAP_USER_PW:
            return 'LDAP'
        elif self == AuthMethod.LOCAL_USER_DB:
            return 'Local'
        return 'Error'


class AuthenticationResult(Enum):
    Success = 0
    Perm = 1
    InternalError = 2

    def __str__(self):
        if self == AuthenticationResult.Success:
            return 'Yo, passt!'
        elif self == AuthenticationResult.Perm:
            return choose_insult()
        else:
            return 'Internal authentication error'

class Authenticator:
    def __init__(self, simulate=False):
        self._simulate = simulate
        self._backends = set()

    @property
    def backends(self):
        return self._backends

    def enable_ldap_backend(self, uri, binddn):
        self._ldap_uri = uri
        self._ldap_binddn = binddn
        self._backends.add(AuthMethod.LDAP_USER_PW)

        ldap.set_option(ldap.OPT_X_TLS_REQUIRE_CERT, ldap.OPT_X_TLS_DEMAND)
        ldap.set_option(ldap.OPT_REFERRALS, 0)


    def enable_local_backend(self, filename):
        self._local_db = dict()

        with open(filename, 'r') as f:
            for line in f:
                line = line.split()
                user = line[0]
                pwd = line[1].split(':')
                self._local_db[user] = pwd

        self._backends.add(AuthMethod.LOCAL_USER_DB)

    def _try_auth_local(self, user, password):
        if user not in self._local_db:
            return AuthenticationResult.Perm

        stored_pw = self._local_db[user][0]
        stored_salt = self._local_db[user][1]
        if stored_pw == hashlib.sha256(stored_salt.encode() + password.encode()).hexdigest():
            return AuthenticationResult.Success

        return AuthenticationResult.Perm

    def _try_auth_ldap(self, user, password):
        log.info('  Trying to LDAP auth (user, password) as user %s', user)
        ldap_username = self._ldap_binddn % user
        try:
            l = ldap.initialize(self._ldap_uri)
            l.simple_bind_s(ldap_username, password)
            l.unbind_s()
        except ldap.INVALID_CREDENTIALS:
            log.info('  Invalid credentials')
            return AuthenticationResult.Perm
        except ldap.LDAPError as e:
            log.info('  LDAP Error: %s' % e)
            return AuthenticationResult.InternalError
        return AuthenticationResult.Success

    def try_auth(self, credentials):
        if self._simulate:
            log.info('SIMULATION MODE! ACCEPTING ANYTHING!')
            return AuthenticationResult.Success

        method = credentials[0]
        if method not in self._backends:
            return AuthenticationResult.InternalError

        if method == AuthMethod.LDAP_USER_PW:
            return self._try_auth_ldap(credentials[1], credentials[2])
        elif method == AuthMethod.LOCAL_USER_DB:
            return self._try_auth_local(credentials[1], credentials[2])

        return AuthenticationResult.InternalError
