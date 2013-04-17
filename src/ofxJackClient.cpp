//
//  ofxJackClient.cpp
//
//  Created by game over on 18/03/13.
//  Copyright (c) 2013 trace media. All rights reserved.
//

#include "ofxJackClient.h"

int _process (jack_nframes_t nframes, void *arg){
    // just call the process function passed as *arg
	return static_cast<ofxJackClient*>(arg)->process(nframes);      
}

//--------------------------------------------------------------
ofxJackClient::ofxJackClient(){
    client = NULL;
}

//--------------------------------------------------------------
ofxJackClient::~ofxJackClient(){
    stop();
}

//--------------------------------------------------------------
bool ofxJackClient::setup(string clientName, bool useProcessAudio){
    if ((client = jack_client_open (clientName.c_str(), JackNullOption, NULL)) == NULL) {
		ofLogError() << "JACK server is not running";
		return false;
	}else{
        if(useProcessAudio){
            jack_set_process_callback(client, _process, this);
        }
        return true;
    }
}

//--------------------------------------------------------------
bool ofxJackClient::start(){
    if(client == NULL){
        ofLogError() << "Create client before starting!";
        return false;
    }
    if(jack_activate(client) != 0) {
		ofLogError() << "Cannot start JACK client: " << getClientName();
	}
    return true;
}

//--------------------------------------------------------------
void ofxJackClient::stop(){
    
    if(client != NULL){
        jack_deactivate(client);
        jack_client_close (client);
        client = NULL;
    }
    inPorts.clear();
    outPorts.clear();
}

//--------------------------------------------------------------
vector<string> ofxJackClient::getAllPorts(){
    return getPortsForClient("");
}

//--------------------------------------------------------------
vector<string> ofxJackClient::getClientPorts(){
    return getPortsForClient(getClientName());
}

//--------------------------------------------------------------
vector<string> ofxJackClient::getPortsForClient(string clientName){
    
    bool noServer = false;
    jack_status_t status;
    const char **ports, **connections;
    vector<string> portNames;
    
    
    if(client == NULL){
        // no need to start the server
        noServer = true;
        client = jack_client_open ("port list", JackNoStartServer, &status);
        if(client == NULL){
            if(status & JackServerFailed){
                ofLogError() << "JACK server is not running";
            }else{
                ofLogError() << "jack_client_open() failed with status: " << status;
            }
        }
    }
    
    ports = jack_get_ports(client, clientName.c_str(), NULL, 0);
    
    for (int portIndex = 0; ports[portIndex]; ++portIndex) {
        
        jack_port_t *port;
        
        portNames.push_back((string)ports[portIndex]);
        
        port = jack_port_by_name (client, ports[portIndex]);
        
        ostringstream os;
        
        os << portIndex << " " << ports[portIndex];
        
        int flags = jack_port_flags(port);
        
        if(flags & JackPortIsInput){
            os << " input,";
        }
        if(flags & JackPortIsOutput){
            os << " output,";
        }
        if (flags & JackPortCanMonitor) {
            os << " can monitor,";
        }
        if (flags & JackPortIsPhysical) {
            os << " physical,";
        }
        if (flags & JackPortIsTerminal) {
            os << " terminal,";
        }
        
        os << " latency: " << jack_port_get_latency(port);
        
        if((connections = jack_port_get_all_connections (client, jack_port_by_name(client, ports[portIndex]))) != 0){
            os << endl;
            for (int j = 0; connections[j]; j++) {
                os << connections[j] << endl;;
            }
            free (connections);
        }
        
		ofLogNotice() << os.str();
        
    }
    
    if(noServer) jack_client_close (client);
    
    return portNames;
    
}

//--------------------------------------------------------------
string ofxJackClient::getClientName(){
    return (string)jack_get_client_name(client);
}

//--------------------------------------------------------------
bool ofxJackClient::createPort(string portName, JackPortFlags portType){
    
    if(client == NULL){
        ofLogError() << "Create client first!";
        return false;
    }
    
    switch (portType) {
        case JackPortIsInput:
        {
            inPorts.push_back(jack_port_register (client, portName.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0));
            break;
        }
        case JackPortIsOutput:
        {
            outPorts.push_back(jack_port_register (client, portName.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0));
            break;
        }
            
    }
}

//--------------------------------------------------------------
bool ofxJackClient::connect(string inPortName, string outPortName){
    if(jack_connect(client, inPortName.c_str(), outPortName.c_str()) != 0){
        ofLogError() << "Cannot connect ports: " << inPortName << " with " << outPortName;
        return false;
    }else{
        return true;
    }
}

//--------------------------------------------------------------
int ofxJackClient::process(jack_nframes_t nframes){
    // overide this in your client
    ofLogWarning() << "You should either not use process audio or override this function in your client!";
    return 0;
}