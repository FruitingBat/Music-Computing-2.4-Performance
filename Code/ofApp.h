#pragma once

#include "ofMain.h"
#include "ofxMidi.h"
#include "ofxTimer.h"
#include <string>
#include <iostream>

using namespace std;

class ofApp : public ofBaseApp, public ofxMidiListener {

public:
	// oF functions
	void setup();
	void update();
	void draw();
	void exit();
	void keyPressed(int key);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);

	// My functions
	void resetSequence();
	void clockAdvance();
	void limit();

	// Sequncer Variables
	bool stepActive[9],
		stepMute[9],
		playNote,
		resetState,
		logicCheck;

	float stepNote[9];

	int userMidiSelect,
		noteLow,
		noteHigh,
		resetLength,
		noteLength,
		stepNumber,
		resetPosition,
		sequenceAdvance,
		sequencerMode,
		stepRepeat[9],
		stepRepeatCount[9],
		resetPositionCount;

	//Midi In
	void newMidiMessage(ofxMidiMessage& eventArgs);

	stringstream text;
	ofxMidiIn midiIn;
	ofxMidiMessage midiMessage;

	//Midi Out
	ofxMidiOut midiOut;

	int channel;

	unsigned int currentPgm;

	int note,
		velocity,
		pan,
		bend,
		touch,
		polytouch;

	//Timer
	ofxTimer  noteLengthTimer,
		blinkTimer;
};