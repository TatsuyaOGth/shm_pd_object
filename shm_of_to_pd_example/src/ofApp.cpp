#include "ofApp.h"


void ofApp::setup(){
    
    //----------
    // setup send data
    //----------
    // memory allocate
    mSendData = new float[NUM_DATA];
    
    // setup ofxSharedMemory
    mShmDataSender.setup("of_to_pd", sizeof(float) * NUM_DATA, false);
    mfSenderConnection = mShmDataSender.connect(); // try connection
    
    
    //----------
    // setup receive data
    //----------
    // setup ofxSharedMemory
    mShmDataReceiver.setup("pd_to_of", sizeof(float) * NUM_DATA, true);
    mfReceiverConnection = mShmDataReceiver.connect(); // try connection
}


void ofApp::update(){

    //----------
    // update send data
    //----------
    if (mfSenderConnection)
    {
        // update send data
        for (int i = 0; i < NUM_DATA; ++i)
        {
            mSendData[i] = ofNoise((i * 0.04 + ofGetElapsedTimef()) * 2 - 1);
        }
        
        // set send data
        mShmDataSender.setData(mSendData);
        
    }
    else {
        mfSenderConnection = mShmDataSender.connect();
    }
    
    
    //----------
    // update receiver data
    //----------
    if (mfReceiverConnection)
    {
        // get shared data
        mReceiveData = mShmDataReceiver.getData();
    }
    else {
        mfReceiverConnection = mShmDataReceiver.connect();
    }

    
}


void ofApp::draw(){
    ofBackground(30);
    
    
    //----------
    // draw send data (yellow ball)
    //----------
    ofSetColor(ofColor::yellow);
    
    for (int i = 0; i < NUM_DATA; ++i)
    {
        ofCircle(ofMap(i, 0, NUM_DATA, 0, ofGetWidth()), mSendData[i] * ofGetHeight(), 5);
    }
    
    
    //----------
    // draw receive data (pink ball)
    //----------
    ofSetColor(ofColor::pink);
    
    if (mReceiveData != NULL) //<--- null check
    {
        for (int i = 0; i < NUM_DATA; ++i)
        {
            ofCircle(ofMap(i, 0, NUM_DATA, 0, ofGetWidth()), mReceiveData[i] * ofGetHeight(), 5);
        }
    }

    
    
    // show shared memory size
    ofSetColor(255);
    ofDrawBitmapString("of_to_pd shared memory size: " + ofToString(sizeof(float) * NUM_DATA), 20, 20);
    ofDrawBitmapString("pd_to_of shared memory size: " + ofToString(sizeof(float) * NUM_DATA), 20, 36);
}


void ofApp::keyPressed(int key){

}


void ofApp::keyReleased(int key){

}


void ofApp::mouseMoved(int x, int y ){

}


void ofApp::mouseDragged(int x, int y, int button){

}


void ofApp::mousePressed(int x, int y, int button){

}


void ofApp::mouseReleased(int x, int y, int button){

}


void ofApp::windowResized(int w, int h){

}


void ofApp::gotMessage(ofMessage msg){

}


void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
