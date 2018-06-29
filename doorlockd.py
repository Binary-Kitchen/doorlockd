#!/usr/bin/env python3

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

import ldap
import logging
import sys

from enum import Enum
from random import sample
from serial import Serial
from subprocess import Popen
from threading import Thread
from time import sleep

from flask import abort, Flask, jsonify, render_template, request
from flask_bootstrap import Bootstrap
from flask_socketio import SocketIO
from flask_wtf import FlaskForm
from wtforms import PasswordField, StringField, SubmitField
from wtforms.validators import DataRequired, Length

__author__ = 'Ralf Ramsauer'
__copyright = 'Copyright (c) Ralf Ramsauer, 2018'
__license__ = 'GPLv2'
__email__ = 'ralf@binary-kitchen.de'
__status__ = 'Development'
__maintainer__ = 'Ralf Ramsauer'
__version__ = '0.01a'

log_level = logging.DEBUG
date_fmt = '%Y-%m-%d %H:%M:%S'
log_fmt = '%(asctime)-15s %(levelname)-8s %(message)s'
log = logging.getLogger()

default_ldap_uri = 'ldaps://ldap1.binary.kitchen/ ' \
                   'ldaps://ldap2.binary.kitchen/ ' \
                   'ldaps://ldapm.binary.kitchen/'
default_binddn = 'cn=%s,ou=people,dc=binary-kitchen,dc=de'

html_title = 'Binary Kitchen Doorlock (%s - v%s)' % (__status__, __version__)

webapp = Flask(__name__)
webapp.config.from_pyfile('config.cfg')
socketio = SocketIO(webapp, async_mode='threading')
Bootstrap(webapp)
serial_port = webapp.config.get('SERIAL_PORT')
simulate = webapp.config.get('SIMULATE')
run_hooks = webapp.config.get('RUN_HOOKS')

ldap.set_option(ldap.OPT_X_TLS_REQUIRE_CERT, ldap.OPT_X_TLS_DEMAND)
ldap.set_option(ldap.OPT_REFERRALS, 0)
if 'LDAP_CA' in webapp.config.keys():
    ldap.set_option(ldap.OPT_X_TLS_CACERTFILE, webapp.config.get('LDAP_CA'))

ldap_uri = webapp.config.get('LDAP_URI')
ldap_binddn = webapp.config.get('LDAP_BINDDN')

wave_emergency = webapp.config.get('WAVE_EMERGENCY')
wave_lock = webapp.config.get('WAVE_LOCK')
wave_lock_button = webapp.config.get('WAVE_LOCK_BUTTON')
wave_unlock = webapp.config.get('WAVE_UNLOCK')
wave_unlock_button = webapp.config.get('WAVE_UNLOCK_BUTTON')
wave_zonk = webapp.config.get('WAVE_ZONK')
sounds = webapp.config.get('SOUNDS')

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


def playsound(filename):
    if not sounds:
        return
    Popen(['nohup', 'aplay', filename])


def start_hook(script):
    if not run_hooks:
        log.info('Hooks disabled: not starting %s' % script)
        return
    log.info('Starting hook %s' % script)
    Popen(['nohup', script])


def run_lock():
    start_hook('./scripts/post_lock')


def run_unlock():
    start_hook('./scripts/post_unlock')


class AuthMethod(Enum):
    LDAP_USER_PW = 1


class DoorState(Enum):
    Close = 1
    Open = 2

    def to_img(self):
        led = 'red'
        if self == DoorState.Open:
             led = 'green'
        return 'static/led-%s.png' % led

    def to_html(self):
        if self == DoorState.Open:
            return 'Offen'
        return 'Geschlossen'


class LogicResponse(Enum):
    Success = 0
    Perm = 1
    AlreadyLocked = 2
    AlreadyOpen = 3
    Inval = 4
    LDAP = 5

    EmergencyUnlock = 10,
    ButtonLock = 11,
    ButtonUnlock = 12,

    def to_html(self):
        if self == LogicResponse.Success:
            return 'Yo, passt.'
        elif self == LogicResponse.Perm:
            return choose_insult()
        elif self == LogicResponse.AlreadyLocked:
            return 'Narf. Schon zu.'
        elif self == LogicResponse.AlreadyOpen:
            return 'Schon offen, treten Sie ein!'
        elif self == LogicResponse.Inval:
            return 'Das was du willst geht nicht.'
        elif self == LogicResponse.LDAP:
            return 'Moep! Geh LDAP fixen!'
        elif self == LogicResponse.EmergencyUnlock:
            return '!!! Emergency Unlock !!!'
        elif self == LogicResponse.ButtonLock:
            return 'Closed by lock button'
        elif self == LogicResponse.ButtonUnlock:
            return 'Temporary unlock by unlock button'

        return 'Bitte spezifizieren Sie.'


class DoorHandler:
    state = DoorState.Close
    do_schnapper = False

    CMD_UNLOCK = b'u'
    CMD_LOCK = b'l'
    CMD_SCHNAPPER = b's'
    BUTTON_LOCK_PRESS = b'L'
    BUTTON_UNLOCK_PRESS = b'U'
    BUTTON_EMERGENCY_PRESS = b'E'

    def __init__(self, device):
        if simulate:
            return

        self.serial = Serial(device, baudrate=9600, bytesize=8, parity='N',
                             stopbits=1, timeout=0)
        self.thread = Thread(target=self.thread_worker)
        self.thread.start()

    def handle_input(self, recv, expect=None):
        if recv == DoorHandler.BUTTON_LOCK_PRESS:
            playsound(wave_lock_button)
            if self.state == DoorState.Open:
                run_lock()
            self.state = DoorState.Close
            logic.emit_status(LogicResponse.ButtonLock)
        elif recv == DoorHandler.BUTTON_UNLOCK_PRESS:
            playsound(wave_unlock_button)
            logic.emit_status(LogicResponse.ButtonUnlock)
        elif recv == DoorHandler.BUTTON_EMERGENCY_PRESS:
            playsound(wave_emergency)
            logic.emit_status(LogicResponse.EmergencyUnlock)

        if expect is None:
            return True
        return recv == expect

    def send_command(self, cmd):
        self.serial.write(cmd)
        self.serial.flush()
        sleep(0.1)
        char = self.serial.read(1)
        if not self.handle_input(char, cmd):
            log.warning('Sent serial command, got wrong response')

    def thread_worker(self):
        while True:
            sleep(0.4)
            while True:
                char = self.serial.read(1)
                if len(char) == 0:
                    break
                self.handle_input(char)

            if self.state == DoorState.Open:
                self.send_command(DoorHandler.CMD_UNLOCK)
                if self.do_schnapper:
                    self.send_command(DoorHandler.CMD_SCHNAPPER)
                    self.do_schnapper = False
            elif self.state == DoorState.Close:
                self.send_command(DoorHandler.CMD_LOCK)

    def open(self):
        self.do_schnapper = True
        if self.state == DoorState.Open:
            return LogicResponse.AlreadyOpen

        self.state = DoorState.Open
        run_unlock()
        return LogicResponse.Success

    def close(self):
        if self.state == DoorState.Close:
            return LogicResponse.AlreadyLocked

        self.state = DoorState.Close
        run_lock()
        return LogicResponse.Success

    def request(self, state):
        if state == DoorState.Close:
            return self.close()
        elif state == DoorState.Open:
            return self.open()


class Logic:
    def __init__(self):
        self.door_handler = DoorHandler(serial_port)

    def _try_auth_ldap(self, user, password):
        if simulate:
            log.info('SIMULATION MODE! ACCEPTING ANYTHING!')
            return LogicResponse.Success

        log.info('  Trying to LDAP auth (user, password) as user %s', user)
        ldap_username = ldap_binddn % user
        try:
            l = ldap.initialize(ldap_uri)
            l.simple_bind_s(ldap_username, password)
            l.unbind_s()
        except ldap.INVALID_CREDENTIALS:
            log.info('  Invalid credentials')
            return LogicResponse.Perm
        except ldap.LDAPError as e:
            log.info('  LDAP Error: %s' % e)
            return LogicResponse.LDAP
        return LogicResponse.Success

    def try_auth(self, credentials):
        method = credentials[0]
        if method == AuthMethod.LDAP_USER_PW:
            return self._try_auth_ldap(credentials[1], credentials[2])

        return LogicResponse.Inval

    def _request(self, state, credentials):
        err = self.try_auth(credentials)
        if err != LogicResponse.Success:
            return err
        return self.door_handler.request(state)

    def request(self, state, credentials):
        err = self._request(state, credentials)
        if err == LogicResponse.Success:
            if self.door_handler.state == DoorState.Open:
                playsound(wave_unlock)
            if self.door_handler.state == DoorState.Close:
                playsound(wave_lock)
        elif err == LogicResponse.AlreadyLocked or err == LogicResponse.AlreadyOpen:
            playsound(wave_zonk)
        self.emit_status(err)
        return err

    def emit_status(self, message=None):
        led = self.state.to_img()
        if message is None:
            message = self.state.to_html()
        else:
            message = message.to_html()

        socketio.emit('status', {'led': led, 'message': message})

    @property
    def state(self):
        return self.door_handler.state


class AuthenticationForm(FlaskForm):
    username = StringField('Username', [Length(min=3, max=25)])
    password = PasswordField('Password', [DataRequired()])
    open = SubmitField('Open')
    close = SubmitField('Close')

    def __init__(self, *args, **kwargs):
        FlaskForm.__init__(self, *args, **kwargs)
        self.desired_state = DoorState.Close

    def validate(self):
        if not FlaskForm.validate(self):
            return False

        if self.open.data:
            self.desired_state = DoorState.Open

        return True


@socketio.on('request_status')
@socketio.on('connect')
def on_connect():
	logic.emit_status()


@webapp.route('/display')
def display():
    if request.remote_addr != '127.0.0.1':
        abort(403)
    return render_template('display.html')


@webapp.route('/api', methods=['POST'])
def api():
    def json_response(response, msg=None):
        json = dict()
        json['err'] = response.value
        json['msg'] = response.to_html() if msg is None else msg
        if response == LogicResponse.Success or \
           response == LogicResponse.AlreadyLocked or \
           response == LogicResponse.AlreadyOpen:
            json['status'] = str(logic.state.to_html())
        return jsonify(json)

    user = request.form.get('user')
    password = request.form.get('pass')
    command = request.form.get('command')

    if any(v is None for v in [user, password, command]):
        log.warning('Incomplete API request')
        abort(400)

    if not user or not password:
        log.warning('Invalid username or password format')
        return json_response(LogicResponse.Inval,
                             'Invalid username or password format')

    credentials = AuthMethod.LDAP_USER_PW, user, password
    desired_state = None

    if command == 'status':
        return json_response(logic.try_auth(credentials))
    elif command == 'lock':
        desired_state = DoorState.Close
    elif command == 'unlock':
        desired_state = DoorState.Open

    if not desired_state:
        return json_response(LogicResponse.Inval, "Invalid command requested")

    log.info('Incoming API request from %s' % user.encode('utf-8'))
    log.info('  desired state: %s' % desired_state)
    log.info('  current state: %s' % logic.state)

    response = logic.request(desired_state, credentials)

    return json_response(response)


@webapp.route('/', methods=['GET', 'POST'])
def home():
    authentication_form = AuthenticationForm()
    response = None

    # detect old API if the 'api' POST variable is set.
    # NOTE: THESE BITS WILL BE REMOVED ONCE EVERYONE UPDATED THEIR APP
    if request.method == 'POST' and request.form.get('api'):
        log.info('Deprecated API usage detected')
        user = request.form.get('user')
        password = request.form.get('pass')
        command = request.form.get('command')

        if any(v is None for v in [user, password, command]):
            log.warning('Incomplete deprecated API request')
            abort(400)

        desired_state = DoorState.Close
        if command == 'unlock':
            desired_state = DoorState.Open
        credentials = AuthMethod.LDAP_USER_PW, user, password

        log.info('Incoming request from %s' % user.encode('utf-8'))
        log.info('  desired state: %s' % desired_state)
        log.info('  current state: %s' % logic.state)
        log.info('  -> Knock knock, %s, please update your app!' % user)

        response = logic.request(desired_state, credentials)
        if response == LogicResponse.Success:
            return '0'
        elif response == LogicResponse.Perm:
            return '7'
        elif response == LogicResponse.AlreadyLocked:
            return '3'
        elif response == LogicResponse.AlreadyOpen:
            return '2'
        elif response == LogicResponse.LDAP:
            return '10'
        else:
            return '1' # Fail-mode

    if request.method == 'POST' and authentication_form.validate():
        user = authentication_form.username.data
        password = authentication_form.password.data
        credentials = AuthMethod.LDAP_USER_PW, user, password

        log.info('Incoming request from %s' % user.encode('utf-8'))
        desired_state = authentication_form.desired_state
        log.info('  desired state: %s' % desired_state)
        log.info('  current state: %s' % logic.state)
        response = logic.request(desired_state, credentials)
        log.info('  response: %s' % response)

    # Don't trust python, zero credentials
    user = password = credentials = None

    return render_template('index.html',
                           authentication_form=authentication_form,
                           response=response,
                           state_text=logic.state.to_html(),
                           led=logic.state.to_img(),
                           title=html_title)


if __name__ == '__main__':
    logging.basicConfig(level=log_level, stream=sys.stdout,
                        format=log_fmt, datefmt=date_fmt)
    log.info('Starting doorlockd')
    log.info('Using serial port: %s' % webapp.config.get('SERIAL_PORT'))

    logic = Logic()

    socketio.run(webapp, host='0.0.0.0', port=8080)

    sys.exit(0)
