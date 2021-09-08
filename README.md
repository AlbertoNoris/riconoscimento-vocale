# About

This repository contains my bachelor degree's thesis with the title: "Riconoscimento vocale per Robot" which in english stands for: "Speech recognition for Robots".

The entire project was designed to be discussed in italian so there may be some cases where google translate might be needed but other than that the code is english based so if you manage to follow the instructions you can test it by speaking english.


# Install
This is designed to run on Linux 18.04 so having this version should be sufficient but not necessary.

These are the steps in order to install it:

1. download these [prebuilt binaries](https://github.com/alphacep/vosk-api/releases/download/v0.3.30/vosk-linux-x86-0.3.30.zip) extract them and put the `libvosk.so` in the /lib folder

open the terminal and type these commands

2. `git clone https://github.com/AlbertoNoris/speech-recognition.git`

3. `cd speech-recognition`

4. `./comprensore`

5. `./microfono.py`

the program should be up and running and you can test it by saying phrases like "ok robot go to the door" or "ok robot turn fifteen degrees and go to the window"

# Change Model
If you follow the install section you should have everything needed in order to run the program but one thing that could be interesting to change is the `Model` which is the Neural Net that does the Speech to Text conversion.
The one provided with the Install section is a nice balance between weight and accuracy but if you want to try other ones go to the [model section of Vosk](https://alphacephei.com/vosk/models), download the one you want to try and replace it with the `Model` folder.

# Structure
The in depth structure of the program is explained inside the thesis but in general microfono.py is responsible for the speech to text and writes what it hears inside the comando document then comprensore.c loads comando and tries to understand if the command is acceptable or not.


