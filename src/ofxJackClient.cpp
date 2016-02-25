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
bool ofxJackClient::setup(string _clientName, bool useProcessAudio){
    if ((client = jack_client_open (_clientName.c_str(), JackNullOption, NULL)) == NULL) {
		ofLogError() << "JACK server is not running";
		return false;
	}else{
        if(useProcessAudio){
            jack_set_process_callback(client, _process, this);
        }
        clientName = _clientName;
        ofLogNotice() << "Jack client setup: " << clientName;
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
	}else{
        ofLogNotice() << "Jack client started: " << getClientName();
    }
    return true;
}

//--------------------------------------------------------------
void ofxJackClient::stop(){
    
    if(client != NULL){
        
        ofLogNotice() << "Atempting to stop Jack client: " << getClientName();
        
        const char **ports;
        
        ports = jack_get_ports(client, clientName.c_str(), NULL, 0);
        
        for (int portIndex = 0; ports[portIndex]; ++portIndex) {
            
            jack_port_t *port;
            
            port = jack_port_by_name (client, ports[portIndex]);
            
            ofLogNotice() << "Disconnecting port: " << ports[portIndex] << endl;
            
            jack_port_disconnect(client, port);
            
            jack_port_unregister(client, port);
            
        }
        
        jack_deactivate(client);
        jack_client_close(client);
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
    
    jack_port_t * port = NULL;
    
    switch (portType) {
        case JackPortIsInput:
        {
            port = jack_port_register (client, portName.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
            if(port == NULL){
                ofLogError() << "Can't create in port: " << portName;
            }else{
                ofLogVerbose() << "Created in port: " << portName;
                inPorts.push_back(port);
            }
            break;
        }
        case JackPortIsOutput:
        {
            port = jack_port_register (client, portName.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
            if(port == NULL){
                ofLogError() << "Can't create out port: " << portName;
            }else{
                ofLogVerbose() << "Created out port: " << portName;
                outPorts.push_back(port);
            }
            break;
        }
            
    }
    
    return (port != NULL);
    
}

//--------------------------------------------------------------
bool ofxJackClient::connect(string inPortName, string outPortName){
    if(jack_connect(client, inPortName.c_str(), outPortName.c_str()) != 0){
        ofLogError() << "Cannot connect port: " << inPortName << " with " << outPortName;
        return false;
    }else{
        ofLogVerbose() << "Connected port: " << inPortName << " with " << outPortName;
        return true;
    }
}

//--------------------------------------------------------------
int ofxJackClient::process(jack_nframes_t nframes){
    // overide this in your client
    ofLogWarning() << "You should either not use process audio or override this function in your client!";
    return 0;
}

//--------------------------------------------------------------
string ofxJackClient::getApplicationName(){
    if(applicationName == ""){
        getApplicationPath();
    }
    return applicationName;
}

//--------------------------------------------------------------
string ofxJackClient::getApplicationPath(){
    // from http://stackoverflow.com/questions/799679/programatically-retrieving-the-absolute-path-of-an-os-x-command-line-app/1024933#1024933
    if(applicationPath == ""){
        int ret;
        pid_t pid; 
        char pathbuf[1024];
        pid = getpid();
        ret = proc_pidpath (pid, pathbuf, sizeof(pathbuf));
        if(ret <= 0){
            ofLogError() << "PID " << pid << " proc_pidpath(): " << strerror(errno);
        }else{
            applicationPath = string(pathbuf);
            vector<string> pathParts = ofSplitString(applicationPath, "/");
            applicationName = pathParts[pathParts.size() - 1];
            //ofLogVerbose() << "proc " << pid << " path: " << pathbuf;
        }
    }
    
    return applicationPath;
}