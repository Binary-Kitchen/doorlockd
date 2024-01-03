from enum import Enum
from random import sample

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
    return sample(eperm_insults, 1)[0]


class DoorlockResponse(Enum):
    Success = 0
    Perm = 1
    AlreadyActive = 2
    # don't break old apps, value 3 is reserved now
    RESERVED = 3
    Inval = 4
    BackendError = 5

    EmergencyUnlock = 10,
    ButtonLock = 11,
    ButtonUnlock = 12,
    ButtonPresent = 13,

    def __str__(self):
        if self == DoorlockResponse.Success:
            return 'Yo, passt.'
        elif self == DoorlockResponse.Perm:
            return choose_insult()
        elif self == DoorlockResponse.AlreadyActive:
            return 'Zustand bereits aktiv'
        elif self == DoorlockResponse.Inval:
            return 'Das was du willst geht nicht.'
        elif self == DoorlockResponse.EmergencyUnlock:
            return '!!! Emergency Unlock !!!'
        elif self == DoorlockResponse.ButtonLock:
            return 'Closed by button'
        elif self == DoorlockResponse.ButtonUnlock:
            return 'Opened by button'
        elif self == DoorlockResponse.ButtonPresent:
            return 'Present by button'
        elif self == DoorlockResponse.BackendError:
            return "Backend Error"

        return 'Error'

