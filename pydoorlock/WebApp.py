"""
Doorlockd -- Binary Kitchen's smart door opener

Copyright (c) Binary Kitchen e.V., 2020

Author:
  Ralf Ramsauer <ralf@binary-kitchen.de>

This work is licensed under the terms of the GNU GPL, version 2.  See
the LICENSE file in the top-level directory.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.
"""

import logging
import json
import threading

from flask import abort, Flask, jsonify, render_template, request, Response
from flask_wtf import FlaskForm
from wtforms import PasswordField, StringField, SubmitField
from wtforms.validators import DataRequired, Length

from .Door import DoorState
from .Doorlock import DoorlockResponse

log = logging.getLogger()
webapp = Flask(__name__)
evt = threading.Event()
json_push_state = ""


def emit_doorstate(response=None):
    global json_push_state

    if response:
        message = str(response)
    else:
        message = str(logic.state)

    json_push_state = json.dumps({
        'message': message,
        'img': logic.state.to_img()})

    # Notify push clients
    evt.set()
    evt.clear()


def push_state():
    def event_str():
        return "data: {}\n\n".format(json_push_state)
    
    try:
        yield event_str()
        while True:
            evt.wait()
            yield event_str()
    except GeneratorExit:
        return


class AuthenticationForm(FlaskForm):
    username = StringField('Username', [Length(min=3, max=25)])
    password = PasswordField('Password', [DataRequired()])
    open = SubmitField('Open')
    present = SubmitField('Present')
    close = SubmitField('Close')

    def __init__(self, *args, **kwargs):
        FlaskForm.__init__(self, *args, **kwargs)
        self.desired_state = DoorState.Closed

    def validate(self):
        if not FlaskForm.validate(self):
            return False

        if self.open.data:
            self.desired_state = DoorState.Open
        elif self.present.data:
            self.desired_state = DoorState.Present

        return True


@webapp.route('/display')
def display():
    return render_template('display.html',
                           room=room,
                           title=title,
                           welcome=welcome)


@webapp.route('/push')
def push():
    emit_doorstate()
    resp = Response(push_state(), mimetype="text/event-stream")
    resp.headers['X-Accel-Buffering'] = 'no'
    resp.headers['Cache-Control'] = 'no-cache'
    return resp


@webapp.route('/api', methods=['POST'])
def api():
    def json_response(response, msg=None):
        json_dict = dict()
        json_dict['err'] = response.value
        json_dict['msg'] = str(response) if msg is None else msg
        if response == DoorlockResponse.Success or \
           response == DoorlockResponse.AlreadyActive:
            # TBD: Remove 'open'. No more users. Still used in App Version 2.1.1!
            json_dict['open'] = logic.state.is_open()
            json_dict['status'] = logic.state.value
        return jsonify(json_dict)

    user = request.form.get('user')
    password = request.form.get('pass')
    command = request.form.get('command')

    if any(v is None for v in [user, password, command]):
        log.warning('Incomplete API request')
        abort(400)

    if not user or not password:
        log.warning('Invalid username or password format')
        return json_response(DoorlockResponse.Inval,
                             'Invalid username or password format')

    credentials = user, password

    if command == 'status':
        return json_response(logic.auth.try_auth(credentials))

    desired_state = DoorState.from_string(command)
    if not desired_state:
        return json_response(DoorlockResponse.Inval, "Invalid command requested")

    log.info('Incoming API request from %s' % user.encode('utf-8'))
    log.info('  desired state: %s' % desired_state)
    log.info('  current state: %s' % logic.state)

    response = logic.request(desired_state, credentials)

    return json_response(response)


@webapp.route('/', methods=['GET', 'POST'])
def home():
    authentication_form = AuthenticationForm()
    response = None

    if request.method == 'POST' and authentication_form.validate():
        user = authentication_form.username.data
        password = authentication_form.password.data
        credentials = user, password

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
                           state_text=str(logic.state),
                           led=logic.state.to_img(),
                           banner='%s - %s' % (title, room))


def webapp_run(cfg, my_logic, status, version, template_folder, static_folder):
    global logic
    logic = my_logic
    
    debug = cfg.boolean('DEBUG')

    host = 'localhost'
    if debug:
        host = '0.0.0.0'

    global room
    room = cfg.str('ROOM')

    global title
    title = cfg.str('TITLE')

    global welcome
    welcome = cfg.str('WELCOME')

    global html_title
    html_title = '%s (%s - v%s)' % (title, status, version)

    webapp.config['SECRET_KEY'] = cfg.str('SECRET_KEY')
    webapp.template_folder = template_folder
    webapp.static_folder = static_folder
    webapp.debug = debug
    webapp.run(host = host, port = 8080, use_reloader=False)
