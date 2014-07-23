/*
 * Node.cpp
 *
 *  Created on: Jul 29, 2012
 *      Author: unforgiven
 */

#include "Node.h"
#include <iostream>
#include <algorithm>

using namespace std;

Node::Node() {
	node_id = "0";

}

Node::Node(int sid) {
	node_id = "0";
	socket_id = sid;
}

Node::~Node() {
	// TODO Auto-generated destructor stub
}

std::vector<std::string>& Node::getListens() {
	return listens;
}

std::string Node::getNodeID() {
	return node_id;
}

std::string Node::setNodeID(std::string id) {
	node_id = id;
	return 0;
}

int Node::getSocketID() {
	return socket_id;
}

int Node::setSocketID(int id) {
	socket_id = id;
	return 0;
}

int Node::addListen(std::string topic_name){
	listens.push_back(topic_name);
	return 0;
}

int Node::removeListen(std::string topic_name){
	std::vector<std::string>::iterator it = std::find(listens.begin(), listens.end(), topic_name );
	listens.erase(it);
	return 0;
}

int Node::clearListens(){
	listens.clear();
	return 0;
}

void Node::listListens(){

	for(std::vector<std::string>::size_type i = 0; i != listens.size(); i++) {
	    cout << listens[i] << endl;
	}

}

bool operator== ( const Node &n1, const Node &n2)
{
		return n1.socket_id == n2.socket_id;
}

