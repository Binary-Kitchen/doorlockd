from flask_wtf import FlaskForm
from wtforms import PasswordField, StringField, SubmitField
from wtforms.validators import DataRequired, Length

from .Door import DoorState
from .Authenticator import AuthMethod

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
