// Eight step sequencer with:
// Note selection with scalable range
// Ability to mute or skip steps
// Ability to repeat a step x number of times before advancing 
// Change if sequencer resets to y posistion after z number of steps
// Change note legnth
// Set sequencer rules: 
//						1 = Forwards
//						2 = Backwards
//                      3 = Ping Pong
//                      4 = Ping Pong Fixed Length
// 
// Programming quirks:
// Sequencer arrays use 0 to 9 steps insted of 0 to 7 steps 
// Steps 0 and 9 are used as double steps for sequencer mode 4 and enable the sequencer to wrap around correctly in all other modes
//
// Some sliders scale above maximum desired value to allow easier acess to maximum value range when moving sliders quickly
// Varible limiting and conditon checks in code makes sure values are within their proper range

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

	// Program settings
	ofSetVerticalSync(true);
	ofSetLogLevel(OF_LOG_VERBOSE);


	// Initialise values
	stepNumber = 1;
	resetPosition = 1;
	resetLength = 128;
	resetPositionCount = 128;
	resetState = 0;
	sequencerMode = 1;
	sequenceAdvance = 1;
	playNote = 0;
	logicCheck = 0;
	noteLow = 1;
	noteHigh = 127;

	for (int i = 0; i < 10; i++) {
		stepNote[i] = 0.5;
		stepRepeat[i] = 1;

		if ((i == 0) || (i == 9)) {
			stepActive[i] = 0;
			stepMute[i] = 0;
			stepRepeatCount[i] = 1;
		}
		else {
			stepActive[i] = 1;
			stepMute[i] = 1;
			stepRepeatCount[i] = stepRepeat[i];
		}
	}

	// User instruction for Midi setup
	cout << "Please type in the desired numbered Midi input ID followed by numbered Midi output ID" << endl;
	cout << "eg: '37' = Midi Input 3 and Midi Output 7" << endl;
	cout << "Press 'Enter' to comfirm input" << endl;
	cout << endl << "(If the inputted numbered ID is incompatible an error will appear in this console and the program must be restarted)" << endl;

	cout << endl << "Available Midi Inputs:" << endl;
	midiIn.listPorts(); // Print input ports to console

	cout << endl << "Available Midi Outputs:" << endl;
	midiOut.listPorts(); // Print output ports to console
	cout << endl;

	userMidiSelect = ((int)getchar()) - 48;
	cout << endl << "User selected numbered Midi Input ID: " << userMidiSelect << endl;
	midiIn.openPort(userMidiSelect);

	midiIn.ignoreTypes(false, false, false); // Ignore sysex, timing, and active sense messages
	midiIn.addListener(this); // Add ofApp as a listener

	userMidiSelect = ((int)getchar()) - 48;
	cout << endl << "User selected numbered Midi Ouput ID: " << userMidiSelect << endl;
	midiOut.openPort(userMidiSelect);

	// Initialise Midi Output values
	channel = 1;
	currentPgm = 0;
	note = 0;
	velocity = 0;
	pan = 0;
	bend = 0;
	touch = 0;
	polytouch = 0;

	// Setup Timers
	noteLength = 500;
	noteLengthTimer.setup(noteLength, 0);
	blinkTimer.setup(100, 0);
}

//--------------------------------------------------------------
void ofApp::exit() {

	// Clean up Midi upon exit
	midiIn.closePort();
	midiIn.removeListener(this);
	midiOut.closePort();
}

//--------------------------------------------------------------
void ofApp::newMidiMessage(ofxMidiMessage& msg) {

	midiMessage = msg; // Make a copy of the latest Midi Input message

	if (msg.status == MIDI_NOTE_ON) { // Advance sequncer upon recieving Midi Note On message from Midi Input

		clockAdvance();
	}
}

//--------------------------------------------------------------
void ofApp::clockAdvance() {

	// Only Advance if Step Repeat has depleted 
	if (stepRepeatCount[stepNumber] <= 1) {
		stepRepeatCount[stepNumber] = stepRepeat[stepNumber];
		stepNumber += sequenceAdvance;
	}
	else {
		stepRepeatCount[stepNumber] -= 1; // Deplete Repeat Counter
	}

	playNote = 1; // Enable Midi Ouput of current step

	// Check if Reset is enabled and if sequencer needs to be reset
	if (resetState == 1) {
		resetPositionCount -= 1; // Deplete Reset Counter

		if (resetPositionCount <= 0) {
			resetSequence();
		}
	}

	// Start current step blink
	blinkTimer.reset();
	blinkTimer.startTimer();
}

//--------------------------------------------------------------
void ofApp::resetSequence() {

	stepNumber = resetPosition; // Set current step to Reset Posistion

	resetPositionCount = resetLength; // Reset the Reset Counter

	// Reset Repeat Counter
	for (int i = 1; i < 9; i++) {
		stepRepeatCount[i] = stepRepeat[i];
	}

	// Reset sequencer direction
	if ((sequencerMode == 3) || (sequencerMode == 4)) {
		sequenceAdvance = 1;
	}

}

//--------------------------------------------------------------
void ofApp::limit() {

	// Saftey checks values

	if (resetLength > 128) {
		resetLength = 128;
	}
	else if (resetLength < 1) {
		resetLength = 1;
	}

	if (noteHigh > 127) {
		noteHigh = 127;
	}
	else if (noteHigh < 1) {
		noteHigh = 1;
	}

	if (noteLow > 126) {
		noteLow = 126;
	}
	else if (noteLow < 0) {
		noteLow = 0;
	}

	for (int i = 1; i < 9; i++) {
		if (stepRepeat[i] > 8) {
			stepRepeat[i] = 1;
		}
		else if (stepRepeat[i] < 1) {
			stepRepeat[i] = 8;
		}
	}

	if (resetPosition > 8) {
		resetPosition = 1;
	}
	else if (resetPosition < 1) {
		resetPosition = 8;
	}

	if (sequencerMode > 4) {
		sequencerMode = 1;
	}
	else if (sequencerMode < 1) {
		sequencerMode = 4;
	}

	if (stepNumber > 9) {
		stepNumber = 1;
	}
	else if (stepNumber < 0) {
		stepNumber = 8;
	}
}

//--------------------------------------------------------------
void ofApp::update() {

	logicCheck = 0; // Condition check at least once

	limit(); // Check value ranges

	while ((stepActive[stepNumber] == 0) || (logicCheck == 0)) { // Conidition check at least once and until a note is playable
		// Set sequencer settings based on sequencer mode
		if (sequencerMode == 1) {
			sequenceAdvance = 1;
		}
		else if (sequencerMode == 2) {
			sequenceAdvance = -1;
		}
		else if (sequencerMode == 3) {
			if (stepNumber <= 1) {
				sequenceAdvance = 1;
			}
			else if (stepNumber >= 8) {
				sequenceAdvance = -1;
			}
		}
		else if (sequencerMode == 4) {
			if (stepNumber <= 0) {
				sequenceAdvance = 1;
			}
			else if (stepNumber >= 9) {
				sequenceAdvance = -1;
			}
		}

		// Sequencer wrap around
		if (sequencerMode != 4) {
			if (stepNumber == 9) {
				stepNumber = 1;
			}
			else if (stepNumber == 0) {
				stepNumber = 8;
			}
		}

		// Skip non active steps
		if (stepActive[stepNumber] == 0) {
			stepNumber += sequenceAdvance;
		}

		logicCheck = 1; // Enable condition check exit
	}

	// Turn note off when timer finishes
	if (noteLengthTimer.isTimerFinished() == 1) {
		midiOut.sendNoteOff(channel, note, velocity);
	}

	// Enable Midi Output for current step if it is not muted and conditions to play note is enabled
	if ((stepMute[stepNumber] == 1) && (playNote == 1)) {

		playNote = 0;

		midiOut.sendNoteOff(channel, note, velocity); // Turn currently playing notes off

		note = ((1 - stepNote[stepNumber]) * 127); // Scale note values
		note = ofMap(note, 0, 127, noteLow, noteHigh, 1); // Re-scale values based on user range settings
		velocity = 127;

		midiOut.sendNoteOn(channel, note, velocity); // Send out Midi note

		// Reset note timer
		noteLengthTimer.reset();
		noteLengthTimer.startTimer();
	}
}

//--------------------------------------------------------------
void ofApp::draw() {

	ofBackground(125); // Clear screen

	// Text labels
	ofSetColor(255);
	ofDrawBitmapString("Pitch", 680, 125);
	ofDrawBitmapString("Step Active", 680, 305);
	ofDrawBitmapString("Step Mute", 680, 362);
	ofDrawBitmapString("Step Mute", 680, 362);
	ofDrawBitmapString("Step Repeat", 680, 452);
	ofDrawBitmapString("Step Number " + ofToString(stepNumber), 10, 80);

	// Grey out Reset text if disabled
	if (resetState == 0) {
		ofSetColor(200);
	}

	ofDrawBitmapString("Reset Count " + ofToString(resetPositionCount), 10, 100);

	// Reset Length
	ofDrawBitmapString("Reset Length " + ofToString(resetLength), 10, 120);
	ofSetColor(0);
	ofDrawRectangle(10, 125, 150, 20);
	if (resetState == 1) {
		ofSetColor(255);
	}
	ofDrawRectangle(170, 125, 20, 20);
	ofSetColor(255);
	ofDrawRectangle(((float(resetLength) / 128) * 150) + 10, 125, 5, 20);

	// Reset Posistion
	ofDrawBitmapString("Reset Posistion " + ofToString(resetPosition), 10, 160);
	ofSetColor(0);
	ofDrawRectangle(10, 165, 150, 20);
	ofSetColor(255);
	ofDrawRectangle((((float(resetPosition) - 1) / 7) * 150) + 10, 165, 5, 20);

	// Sequencer Mode
	ofDrawBitmapString("Sequencer Mode " + ofToString(sequencerMode), 10, 200);
	ofSetColor(0);
	ofDrawRectangle(10, 205, 150, 20);
	ofSetColor(255);
	ofDrawRectangle((((float(sequencerMode) - 1) / 3) * 150) + 10, 205, 5, 20);

	// Note Length
	ofDrawBitmapString("Note Length " + ofToString(noteLength) + "ms", 10, 240);
	ofSetColor(0);
	ofDrawRectangle(10, 245, 150, 20);
	ofSetColor(255);
	ofDrawRectangle(((float(noteLength) / 1000) * 150) + 10, 245, 5, 20);

	// Note High
	ofDrawBitmapString("Note High " + ofToString(noteHigh), 10, 280);
	ofSetColor(0);
	ofDrawRectangle(10, 285, 150, 20);
	ofSetColor(255);
	ofDrawRectangle((((float(noteHigh) - 1) / 126) * 150) + 10, 285, 5, 20);

	// Note Low
	ofDrawBitmapString("Note Low " + ofToString(noteLow), 10, 320);
	ofSetColor(0);
	ofDrawRectangle(10, 325, 150, 20);
	ofSetColor(255);
	ofDrawRectangle(((float(noteLow) / 126) * 150) + 10, 325, 5, 20);

	// Step Note
	for (int i = 1; i < 9; i++) {
		ofSetColor(0);
		ofDrawRectangle(230 + (i * 50), 20, 40, 200);
		if (stepMute[i] == 1) {
			ofSetColor(255);
		}
		else {
			ofSetColor(200);
		}
		ofDrawRectangle(230 + (i * 50), (20 + (float(stepNote[i]) * 200)), 40, 5);
	}

	// Current Step
	for (int i = 1; i < 9; i++) {
		ofSetColor(0);
		if ((stepNumber == i) && (stepActive[i] == 1)) { // Step Blinks upon midi input
			if (blinkTimer.isTimerFinished() == 0) {
				ofSetColor(255);
			}
			else {
				ofSetColor(200);
			}
		}
		ofDrawCircle(250 + (i * 50), 250, 20);
	}

	// Step Active
	for (int i = 1; i < 9; i++) {
		if (stepActive[i] == 1) {
			ofSetColor(255);
		}
		else {
			ofSetColor(0);
		}
		ofDrawRectangle(230 + (i * 50), 280, 40, 40);
	}

	// Step Mute
	for (int i = 1; i < 9; i++) {
		if (stepMute[i] == 1) {
			ofSetColor(255);
		}
		else {
			ofSetColor(0);
		}
		ofDrawRectangle(230 + (i * 50), 340, 40, 40);
	}

	// Step Repeat
	for (int i = 1; i < 9; i++) {
		// Step Repeat slider
		ofSetColor(0);
		ofDrawRectangle(230 + (i * 50), 400, 40, 100);
		ofSetColor(255);
		ofDrawRectangle(230 + (i * 50), (400 + ((1 - (float(stepRepeat[i]) - 1) / 7) * 100)), 40, 5);

		// Step Repeat counter
		ofSetColor(0);
		ofDrawRectangle(270 + (i * 50), 400, 5, 100);
		ofSetColor(255);
		ofDrawRectangle(270 + (i * 50), 400 + ((float(stepRepeatCount[i]) / float(stepRepeat[i])) * 100), 5, 100 - ((float(stepRepeatCount[i]) / float(stepRepeat[i])) * 100));
	}

	// Step Active
	for (int i = 1; i < 9; i++) {
		if (stepActive[i] == 0) {
			ofSetColor(125, 125, 125, 125);
			ofDrawRectangle(230 + (i * 50), 20, 45, 485);
		}
	}

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

	// Manual value adjustment via keyboard for testing / dev purposes

	if (key == ' ') {
		clockAdvance();
	}
	else if (key == 'r') {
		resetSequence();
	}
	else if (key == '-') {
		resetPosition -= 1;
	}
	else if (key == '_') {
		resetPosition -= 1;
	}
	else if (key == '+') {
		resetPosition += 1;
	}
	else if (key == '=') {
		resetPosition += 1;
	}
	else if (key == '[') {
		sequencerMode -= 1;
	}
	else if (key == '{') {
		sequencerMode -= 1;
	}
	else if (key == ']') {
		sequencerMode += 1;
	}
	else if (key == '}') {
		sequencerMode += 1;
	}
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
	// Scale mouse posistion from sliders to relevent values
	// Where relevent apply condition statments and limit values when mouse conditions met

	for (int i = 1; i < 9; i++) {
		if ((x >= (230 + (i * 50))) && (x <= ((230 + 40) + (i * 50)))) {
			if ((y >= 20) && (y <= (20 + 200))) {
				stepNote[i] = ((float(y) - 20) / 200);
			}
			else if ((y >= 400) && (y <= (100 + 400))) {
				stepRepeat[i] = ((7 - ((float(y) - 400) / 100) * 7) + 1);
				stepRepeatCount[i] = stepRepeat[i];
			}
		}
	}

	if ((x >= 10) && (x <= 10 + 150)) {
		if ((y >= 125) && (y <= 125 + 20)) {
			resetLength = ((((float(x) - 10) / 150) * 128) + 1);
			if (resetLength > 128) {
				resetLength = 128;
			}
			else if (resetLength < 1) {
				resetLength = 1;
			}
			resetPositionCount = resetLength;
		}
		else if ((y >= 165) && (y <= 165 + 20)) {
			resetPosition = ((((float(x) - 10) / 150) * 8) + 1);
		}
		else if ((y >= 205) && (y <= 205 + 20)) {
			sequencerMode = ((((float(x) - 10) / 150) * 4) + 1);
			sequenceAdvance = 1;
		}
		else if ((y >= 245) && (y <= 245 + 20)) {
			noteLength = (((float(x) - 10) / 150) * 1001);
			if (noteLength > 1000) {
				noteLength = 1000;
			}
			else if (noteLength < 0) {
				noteLength = 0;
			}
			noteLengthTimer.setTimer(noteLength);
		}
		else if ((y >= 285) && (y <= 285 + 20)) {
			noteHigh = ((((float(x) - 10) / 150) * 127) + 1);

			if (noteLow >= noteHigh) {
				noteLow = (noteHigh - 1);
			}
		}
		else if ((y >= 325) && (y <= 325 + 20)) {
			noteLow = (((float(x) - 10) / 150) * 127);
			if (noteLow >= noteHigh) {
				noteHigh = (noteLow + 1);
			}
		}
	}

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
	// Same as mouse dragged but with the addition of buttons

	for (int i = 1; i < 9; i++) {
		if ((x >= (230 + (i * 50))) && (x <= ((230 + 40) + (i * 50)))) {
			if ((y >= 20) && (y <= (20 + 200))) {
				stepNote[i] = ((float(y) - 20) / 200);
			}
			else if ((y >= 280) && (y <= (280 + 40))) {
				stepActive[i] = !stepActive[i];
			}
			else if ((y >= 340) && (y <= (340 + 40))) {
				stepMute[i] = !stepMute[i];
			}
			else if ((y >= 400) && (y <= (100 + 400))) {
				stepRepeat[i] = ((7 - ((float(y) - 400) / 100) * 7) + 1);
				stepRepeatCount[i] = stepRepeat[i];
			}
		}
	}

	if ((x >= 10) && (x <= 10 + 150)) {
		if ((y >= 125) && (y <= 125 + 20)) {
			resetLength = ((((float(x) - 10) / 150) * 128) + 1);
			if (resetLength > 128) {
				resetLength = 128;
			}
			else if (resetLength < 1) {
				resetLength = 1;
			}
			resetPositionCount = resetLength;
		}
		else if ((y >= 165) && (y <= 165 + 20)) {
			resetPosition = ((((float(x) - 10) / 150) * 8) + 1);
		}
		else if ((y >= 205) && (y <= 205 + 20)) {
			sequencerMode = ((((float(x) - 10) / 150) * 4) + 1);
			sequenceAdvance = 1;
		}
		else if ((y >= 245) && (y <= 245 + 20)) {
			noteLength = (((float(x) - 10) / 150) * 1001);

			if (noteLength > 1000) {
				noteLength = 1000;
			}
			else if (noteLength < 0) {
				noteLength = 0;
			}
			noteLengthTimer.setTimer(noteLength);
		}
		else if ((y >= 285) && (y <= 285 + 20)) {
			noteHigh = ((((float(x) - 10) / 150) * 127) + 1);
		}
		else if ((y >= 325) && (y <= 325 + 20)) {
			noteLow = (((float(x) - 10) / 150) * 127);
		}
	}

	if ((x >= 170) && (x <= 170 + 20)) {
		if ((y >= 125) && (y <= 125 + 20)) {
			resetState = !resetState;
			if (resetState == 1) {
				resetPositionCount = resetLength;
			}
		}
	}

}