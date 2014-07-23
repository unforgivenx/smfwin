/*
 * Node.h
 *
 *  Created on: Jul 29, 2012
 *      Author: unforgiven
 */

#ifndef NODE_H_
#define NODE_H_

#include <vector>
#include <string>



class Node {
public:
	Node();
	Node(int);
	virtual ~Node();
	std::vector<std::string>& getListens();
	std::string getNodeID();
	std::string setNodeID(std::string);
	int getSocketID();
	int setSocketID(int);
	int addListen(std::string);
	int removeListen(std::string);
	int clearListens();
	void listListens();
	friend bool operator== ( const Node &n1, const Node &n2);

protected:
	std::string node_id;
	int socket_id;
	std::vector<std::string> listens;

};

#endif /* NODE_H_ */
