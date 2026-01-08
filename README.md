# robot-piano-player

## Software Scripts

To run, create a virtuation environment with venv and cd into ```software/python```

To install the required libraries run the following:

```pip install -r requirements.txt```

### Software Folders

- ```matlab``` simulink and MATLAB simulation files
- ```src``` C source files
- ```python/config``` contains YAML files for different songs noting start and end time if deciding to play just a certain section
- ```python/songs``` MIDI files both pre and post processing

### Python Utilities

- ```midi_utils.py``` functions for song processing to make them easier to play/compatible with the keyboard
- ```speed_utils.py``` functions to calculate plot hand speed across the course of the song

