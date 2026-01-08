import pretty_midi
from midi_utils import get_octave

# units in cm
WHITE_KEY_WIDTH_CM = 2
BLACK_KEY_WIDTH_CM = 1

"""
    Speed calculation is done assuming just one solenoid
    Will not support anything that plays multiple octaves together
"""
def calc_speed(notes):
    curr_note = notes[0]

    speed_plt = []
    time_s = []

    for note in notes:
        prev_note   = curr_note
        curr_note   = note

        dt = curr_note.start - prev_note.end

        # create a array that's pitch names to previous

        notes = range(prev_note.pitch, curr_note.pitch+1)
        note_names = [pretty_midi.note_number_to_name(n) for n in notes]

        # count black and white keys using note names
        black_keys = ["#" in name for name in note_names]
        num_black_keys = len(black_keys)
        num_white_keys = len(notes) - num_black_keys

        distance_cm = num_black_keys * BLACK_KEY_WIDTH_CM + num_white_keys * WHITE_KEY_WIDTH_CM

        speed_cmps = distance_cm/dt

        speed_plt.append(speed_cmps)
        time_s.append(curr_note.start)

    return time_s, speed_plt