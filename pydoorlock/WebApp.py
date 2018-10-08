import logging

from flask import abort, Flask, jsonify, render_template, request
from flask_socketio import SocketIO
from flask_wtf import FlaskForm
from wtforms import PasswordField, StringField, SubmitField
from wtforms.validators import DataRequired, Length

from .Door import DoorState
from .Authenticator import AuthMethod

log = logging.getLogger()

webapp = Flask(__name__)
socketio = SocketIO(webapp, async_mode='threading')


def emit_doorstate(response=None):
    state = logic.state
    if response:
        message = str(response)
    else:
        message = str(state)
    socketio.emit('status', {'led': state.to_img(), 'message': message})


class AuthenticationForm(FlaskForm):
    username = StringField('Username', [Length(min=3, max=25)])
    password = PasswordField('Password', [DataRequired()])
    method = StringField('Method', [DataRequired()])
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

        if self.method.data == 'Local':
            self.method = AuthMethod.LOCAL_USER_DB
        else: # default: use LDAP
            self.method = AuthMethod.LDAP_USER_PW

        return True


@socketio.on('request_status')
@socketio.on('connect')
def on_connect():
    emit_doorstate()


@webapp.route('/display')
def display():
    return render_template('display.html',
                           room=room,
                           title=title,
                           welcome=welcome)


@webapp.route('/api', methods=['POST'])
def api():
    def json_response(response, msg=None):
        json = dict()
        json['err'] = response.value
        json['msg'] = response.to_html() if msg is None else msg
        if response == LogicResponse.Success or \
           response == LogicResponse.AlreadyActive:
            # TBD: Remove 'open'. No more users. Still used in App Version 2.1.1!
            json['open'] = logic.state.is_open()
            json['status'] = logic.state.value
        return jsonify(json)

    method = request.form.get('method')
    user = request.form.get('user')
    password = request.form.get('pass')
    command = request.form.get('command')

    if method == 'local':
        method = AuthMethod.LOCAL_USER_DB
    else: # 'ldap' or default
        method = AuthMethod.LDAP_USER_PW

    if any(v is None for v in [user, password, command]):
        log.warning('Incomplete API request')
        abort(400)

    if not user or not password:
        log.warning('Invalid username or password format')
        return json_response(LogicResponse.Inval,
                             'Invalid username or password format')

    credentials = method, user, password

    if command == 'status':
        return json_response(logic.try_auth(credentials))

    desired_state = DoorState.from_string(command)
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

    if request.method == 'POST' and authentication_form.validate():
        user = authentication_form.username.data
        password = authentication_form.password.data
        method = authentication_form.method
        credentials = method, user, password

        log.info('Incoming request from %s' % user.encode('utf-8'))
        log.info('  authentication method: %s' % method)
        desired_state = authentication_form.desired_state
        log.info('  desired state: %s' % desired_state)
        log.info('  current state: %s' % logic.state)
        response = logic.request(desired_state, credentials)
        log.info('  response: %s' % response)

    # Don't trust python, zero credentials
    user = password = credentials = None

    return render_template('index.html',
                           authentication_form=authentication_form,
                           auth_backends=logic.auth.backends,
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
    socketio.run(webapp, host=host, port=8080, use_reloader=False, debug=debug)
