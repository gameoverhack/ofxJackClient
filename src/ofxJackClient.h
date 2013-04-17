//
//  ofxJackClient.h
//
//  Created by game over on 18/03/13.
//  Copyright (c) 2013 trace media. All rights reserved.
//

#ifndef _H_OFXJACKCLIENT
#define _H_OFXJACKCLIENT

#include "ofMain.h"
#include <jack/jack.h>

class ofxJackClient {
    
public:
	
    ofxJackClient();
    ~ofxJackClient();
    
    bool setup(string clientName, bool useProcessAudio = true);

    bool start();
    void stop();
    
    vector<string> getAllPorts();
    vector<string> getClientPorts();
    vector<string> getPortsForClient(string clientName);
    
    string getClientName();
    
    bool createPort(string portName, JackPortFlags portType);
    
    bool connect(string inPortName, string outPortName);
    
    virtual int process(jack_nframes_t nframes);
    
protected:
	
    jack_client_t *client;
    vector<jack_port_t*> inPorts;
    vector<jack_port_t*> outPorts;
    
private:
	
};

#endif
