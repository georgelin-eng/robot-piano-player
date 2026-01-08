import pretty_midi
import os
import yaml

STRIKE_TIME   = 50e-3 # time it takes for solenoid plunger to go out and back in, ms
MAX_HOLD_TIME = 2     # solenoid should not be held for more than 2s
C2_PITCH = 36 
C5_PITCH = 72
PITCHES_IN_OCTAVE = 12

def get_octave(pitch):
    return int(pitch / PITCHES_IN_OCTAVE) - 1

def midi_to_notes(song_name):
    # Make sure that MIDI file has just a single track or otherwise do some preprocessing
    pm = pretty_midi.PrettyMIDI(f"songs/{song_name}.mid")

    notes = []
    for instrument in pm.instruments:
        notes.extend(instrument.notes)  # add all notes from this instrument

    config_path = f"config/{song_name}.yaml"
    if os.path.exists(config_path):
        with open(config_path, 'r') as f:
            config = yaml.safe_load(f)
            start_time = config.get('start_time', 0)  # default 0 if not set
            end_time = config.get('end_time', float('inf'))  # default no end limit

        # Filter notes
        filtered_notes = [note for note in notes if start_time <= note.start <= end_time]
        return filtered_notes

    notes.sort(key=lambda n: n.start)

    return notes

def make_song_staccato(notes, note_length=0.5):
    for note in notes:
        dt = note.end - note.start

        if (dt > STRIKE_TIME):
            note.end = note.end - dt * note_length

        if (dt > MAX_HOLD_TIME):
            note.end = note.start + MAX_HOLD_TIME

    return notes

# TODO: Ask Rohan about how to do this so that the musicality is preserved
def fit_song_into_keyboard(notes):
    for note in notes:
        note.pitch -= 12

    for note in notes:
        if (note.pitch < C2_PITCH):
            diff = 2 - get_octave(note.pitch)         
            note.pitch += 12 * diff

        if (note.pitch >= C5_PITCH):
            diff = get_octave(note.pitch) - 5
            note.pitch -= 12 * (diff+1)

    return notes

def seperate_left_right(notes, PITCH_SEPARATION):
    left_hand_notes  = [note for note in notes if note.pitch < PITCH_SEPARATION]
    right_hand_notes = [note for note in notes if note.pitch >= PITCH_SEPARATION]

    return left_hand_notes, right_hand_notes

def notes_to_midi(notes, filename, instrument_name='Electric Grand Piano'):
    midi_out = pretty_midi.PrettyMIDI()
    instr_program = pretty_midi.instrument_name_to_program(instrument_name)
    instr = pretty_midi.Instrument(program=instr_program)
    
    for note in notes:
        midi_note = pretty_midi.Note (
            velocity=note.velocity,
            pitch=note.pitch,
            start=note.start,
            end=note.end
        )

        instr.notes.append(midi_note)
    
    midi_out.instruments.append(instr)
    midi_out.write(filename)
