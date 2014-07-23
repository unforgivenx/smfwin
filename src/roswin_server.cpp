#include "ros/ros.h"
#include "smfwin/roswin_server.h"
#include "std_msgs/String.h"
#include "std_msgs/Float32.h"
#include "std_msgs/UInt32.h"
#include "Node.h"
#include "Type.h"
#include <topic_tools/shape_shifter.h>
#include <sensor_msgs/LaserScan.h>
#include <smfwin/SMF.h>
#include <smfwin/GetSMFData.h>
#include <rosbag/message_instance.h>
#include <ros/serialization.h>
#include <sstream>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <cstring>
#include <sys/utsname.h>
#include <time.h>

#define MAXEVENTS 64

using namespace std;
using namespace ros;
using namespace ros::serialization;
using namespace topic_tools;
using std::memcpy;

std::vector<Node> clients;
std::vector<Type> types;


float Float(char unsigned *const p)
{
	float val;

	memcpy(&val,p,sizeof val);

	return val;
}

const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);

    return buf;
}

static int make_socket_non_blocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      //perror ("fcntl");
      ROS_ERROR("fcntl");
      return -1;
    }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
    {
      //perror ("fcntl");
      ROS_ERROR("fcntl");
      return -1;
    }

  return 0;
}

static int create_and_bind (char *port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int s, sfd;

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
  hints.ai_flags = AI_PASSIVE;     /* All interfaces */

  s = getaddrinfo (NULL, port, &hints, &result);
  if (s != 0)
    {
      //fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
      ROS_ERROR("getaddrinfo: %s", gai_strerror (s));
      return -1;
    }

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sfd == -1)
        continue;

      s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
      if (s == 0)
        {
          /* We managed to bind successfully! */
          break;
        }

      close (sfd);
    }

  if (rp == NULL)
    {
      //fprintf (stderr, "Could not bind\n");
      ROS_ERROR("Could not bind");
      return -1;
    }

  freeaddrinfo (result);

  return sfd;
}


std_msgs::String msg;
bool success;

void clientsInfo() {

	std::vector<std::string>::iterator it;
	for(std::vector<Node>::size_type i = 0; i != clients.size(); i++) {
		//ROS_INFO("Client \n");
		cout << "Client " << clients[i].getSocketID() << endl;
		clients[i].listListens();
	}
}

void distributeMessage(std::string msgid, std::string msgtype, std::string msgdata, std::string msgts, int fd) {

	std::vector<std::string>::iterator it;
	//cout << msgtype << endl;
	for(std::vector<Node>::size_type i = 0; i != clients.size(); i++) {
		if (clients[i].getListens().size()>0)
		{
			it = std::find(clients[i].getListens().begin(), clients[i].getListens().end(), msgtype );
			if (it != clients[i].getListens().end() && fd != clients[i].getSocketID())
			{
				//ROS_INFO("a");
				std::string smfmsg;
				smfmsg = "<SMFPacket>\n";
				smfmsg += "<Type>" + msgtype + "</Type>\n";
				smfmsg += "<Data>" + msgdata + "</Data>\n";
				smfmsg += "<ID>" + msgid + "</ID>\n";
				smfmsg += "<TimeStamp>" + msgts + "</TimeStamp>\n";
				smfmsg += "</SMFPacket>\n";
				write (clients[i].getSocketID(), smfmsg.c_str(), smfmsg.length());
			}
			else
			{
				//ROS_INFO("b");
			}
		}
	}
}

string get_xml_element (string xmltext, string tag){

	size_t pos1, pos2;
	string ex_tag = "<" + tag + ">";
	string ex_tag_e = "</" + tag + ">";
	pos1 = xmltext.find(ex_tag)+ex_tag.size();
	pos2 = xmltext.find(ex_tag_e);
	if (pos1 < 0)
		return "-1";
	else
		return xmltext.substr(pos1, pos2-pos1);

}

void split(const string& str, const string& delim, vector<string>& parts) {
  size_t start, end = 0;
  while (end < str.size()) {
    start = end;
    while (start < str.size() && (delim.find(str[start]) != string::npos)) {
      start++;  // skip initial whitespace
    }
    end = start;
    while (end < str.size() && (delim.find(str[end]) == string::npos)) {
      end++; // skip to end of word
    }
    if (end-start != 0) {  // just ignore zero-length strings.
      parts.push_back(string(str, start, end-start));
    }
  }
}

void smfCallback(const ros::MessageEvent<smfwin::SMF const>& event)
{
	const std::string& publisher_name = event.getPublisherName();
	ros::M_string header = event.getConnectionHeader();
	ros::Time receipt_time = event.getReceiptTime();
	const smfwin::SMF::ConstPtr& msg = event.getMessage();
	//ROS_INFO("callback");

	ros::M_string::iterator it = header.find("type");
	std::string mtype = it->second;

	it = header.find("message_definition");
	std::string mdef = it->second;

	std::stringstream ss;
	ros::WallTime t= ros::WallTime::now(); 
	std::stringstream s;  // Allocates memory on stack
    s << t.sec << "." << t.nsec;
	std::string tstamp = s.str();

	distributeMessage(publisher_name, msg->type.c_str(), msg->data.c_str(), tstamp, 0);
	//ROS_INFO("Message sent from ROS to %s type", msg->type.c_str());
}

bool get_smf_data(smfwin::GetSMFData::Request  &req,
         smfwin::GetSMFData::Response &res)
{
	std::vector<Type>::iterator type_it;
	type_it = std::find(types.begin(), types.end(), Type(req.type) );
	if (type_it == types.end())
	res.data = "null";
	else
	res.data = type_it->getData();
	return true;
}

int main(int argc, char **argv)
{


  Node client1;
  struct utsname sysinf;
  std::vector<Node>::iterator it;
  std::vector<Type>::iterator type_it;
  uname(&sysinf);
  std::string wlcm = "Welcome to ROSWIN Server v1.0.0 by METU Inmate Lab\n";
  wlcm += "Running on ";
  wlcm += sysinf.sysname;
  wlcm += " ";
  wlcm += sysinf.release;
  wlcm += " Since ";
  wlcm += currentDateTime();
  wlcm += '\n';
  
  //cout << sysinf.sysname << " " << sysinf.release << endl;
  //std::vector<std::string> glistens = client1.getListens();

  ros::init(argc, argv, "smf");
  ros::NodeHandle n;
  ros::NodeHandle nodeHandler;

  ros::Publisher smf_type_pub = n.advertise<std_msgs::String>("smf_win_type_rx", 1000);
  ros::Publisher smf_pub = n.advertise<smfwin::SMF>("smf_win_rx", 1000);
  ros::Subscriber smf_sub = n.subscribe<smfwin::SMF>("smf_win_tx", 1000, &smfCallback);
  ros::ServiceServer service = n.advertiseService("get_smf_data", get_smf_data);

  ros::Rate loop_rate(100);
  //ROS_INFO("1\n");

  std::string msgback;
  int pi = 0;
  int sfd, s;
  int efd;
  struct epoll_event event;
  struct epoll_event *events;

  if (argc != 2)
    {
      //fprintf (stderr, "Usage: %s [port]\n", argv[0]);
      ROS_ERROR("Usage: %s [port]", argv[0]);
      exit (EXIT_FAILURE);
    }

  sfd = create_and_bind (argv[1]);
  if (sfd == -1)
    abort ();

  s = make_socket_non_blocking (sfd);
  if (s == -1)
    abort ();

  s = listen (sfd, SOMAXCONN);
  if (s == -1)
    {
      ROS_ERROR("listen");
      //perror ("listen");
      abort ();
    }

  efd = epoll_create1 (0);
  if (efd == -1)
    {
      ROS_ERROR("epoll_create");
      //perror ("epoll_create");
      abort ();
    }

  event.data.fd = sfd;
  event.events = EPOLLIN | EPOLLET;
  s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
  if (s == -1)
    {
      ROS_ERROR("epoll_ctl");
      //perror ("epoll_ctl");
      abort ();
    }

  /* Buffer where events are returned */
  events = (epoll_event*)calloc (MAXEVENTS, sizeof event);
  int counter=0;
  ROS_INFO("Server is ready to accept connections");
  /* The event loop */
  while (ros::ok())
  {
	  int n, i;

	        n = epoll_wait (efd, events, MAXEVENTS, 0);
	        for (i = 0; i < n; i++)
	  	{
	  	  if ((events[i].events & EPOLLERR) ||
	                (events[i].events & EPOLLHUP) ||
	                (!(events[i].events & EPOLLIN)))
	  	    {
	                /* An error has occured on this fd, or the socket is not
	                   ready for reading (why were we notified then?) */
	  	      //fprintf (stderr, "epoll error\n");
	  	      ROS_INFO("Closed connection on descriptor %d", events[i].data.fd);
	  	      //printf ("Closed connection on descriptor %d\n", events[i].data.fd);

	            /* Closing the descriptor will make epoll remove it
	               from the set of descriptors which are monitored. */
	            close (events[i].data.fd);
	            it = std::find(clients.begin(), clients.end(), Node(events[i].data.fd) );
	            clients.erase(it);
	  	      //close (events[i].data.fd);
	  	      continue;
	  	    }

	  	  else if (sfd == events[i].data.fd)
	  	    {
	                /* We have a notification on the listening socket, which
	                   means one or more incoming connections. */
	                while (1)
	                  {
	                    struct sockaddr in_addr;
	                    socklen_t in_len;
	                    int infd;
	                    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

	                    in_len = sizeof in_addr;
	                    infd = accept (sfd, &in_addr, &in_len);
	                    if (infd == -1)
	                      {
	                        if ((errno == EAGAIN) ||
	                            (errno == EWOULDBLOCK))
	                          {
	                            /* We have processed all incoming
	                               connections. */
	                            break;
	                          }
	                        else
	                          {
	                          	ROS_ERROR("accept");
	                            //perror ("accept");
	                            break;
	                          }
	                      }

	                    s = getnameinfo (&in_addr, in_len,
	                                     hbuf, sizeof hbuf,
	                                     sbuf, sizeof sbuf,
	                                     NI_NUMERICHOST | NI_NUMERICSERV);
	                    if (s == 0)
	                      {
	                      	ROS_INFO("Accepted connection on descriptor %d (host=%s, port=%s)", infd, hbuf, sbuf);
	                        //printf("Accepted connection on descriptor %d (host=%s, port=%s)\n", infd, hbuf, sbuf);

	                        //std::string wlcm = "Welcome to ROSWIN Server\n";
	                        write (infd, wlcm.c_str(), wlcm.length());
	                        Node client1 (infd);
	                        clients.push_back(client1);
	                      }

	                    /* Make the incoming socket non-blocking and add it to the
	                       list of fds to monitor. */
	                    s = make_socket_non_blocking (infd);
	                    if (s == -1)
	                      abort ();

	                    event.data.fd = infd;
	                    event.events = EPOLLIN | EPOLLET;
	                    s = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &event);
	                    if (s == -1)
	                      {
	                      	ROS_ERROR("epoll_ctl");
	                        //perror ("epoll_ctl");
	                        abort ();
	                      }
	                  }
	                continue;
	              }
	            else
	              {
	                /* We have data on the fd waiting to be read. Read and
	                   display it. We must read whatever data is available
	                   completely, as we are running in edge-triggered mode
	                   and won't get a notification again for the same
	                   data. */
	                int done = 0;

	                while (1)
	                  {
	                    ssize_t count;
	                    char buf[512];

	                    count = read (events[i].data.fd, buf, sizeof buf);
	                    if (count == -1)
	                      {
	                        /* If errno == EAGAIN, that means we have read all
	                           data. So go back to the main loop. */
	                        if (errno != EAGAIN)
	                          {
	                            //perror ("read");
	                            ROS_ERROR("read");
	                            done = 1;
	                          }
	                        break;
	                      }
	                    else if (count == 0)
	                      {
	                        /* End of file. The remote has closed the
	                           connection. */
	                        done = 1;
	                        break;
	                      }

	                    it = std::find(clients.begin(), clients.end(), Node(events[i].data.fd) );
	                    std::string client_msg( reinterpret_cast<char const*>(buf), count ) ;
	                    client_msg.erase( std::remove(client_msg.begin(), client_msg.end(), '\r'), client_msg.end() );
	                    client_msg.erase( std::remove(client_msg.begin(), client_msg.end(), '\n'), client_msg.end() );

	                    if (client_msg.find("<SMFPacket>") < 0 || client_msg.find("</SMFPacket>") < 0)
	                    {
	                    	msgback = "Not a valid SMF packet!\n";
							write (events[i].data.fd, msgback.c_str(), msgback.length());
	                    }
	                    else
	                    {
	                    	//cout << "msg " << client_msg.c_str() << endl;
							string msg_id = get_xml_element(client_msg, "ID");
							//cout << "msgid " << msg_id << endl;

							string msgtype = get_xml_element(client_msg, "Type");
							//cout << "msgtype " << msgtype << endl;

							string msgdata = get_xml_element(client_msg, "Data");
							//cout << "msgdata " << msgdata << endl;

							string msglistens = get_xml_element(client_msg, "Listens");
							//cout << "msglistens " << msglistens << endl;

							vector<string> topics;
							split(msglistens, ",", topics);
							ROS_DEBUG("Message from client ID: %s", msg_id.c_str());
							//cout << "Message from client ID: " << msg_id << endl;
							if (topics.size()>0)
							{
								it->clearListens();
								ROS_DEBUG("Client listens topics: ");
								//cout << "Client listens topics: " << endl;
								for(std::vector<std::string>::size_type i = 0; i != topics.size(); i++) {
									it->addListen(topics[i]);
									ROS_DEBUG(" - %s", topics[i].c_str());
									//cout << " - " << topics[i].c_str() << endl;
								}
							}
							else if (msglistens.size()<1)
							{
								it->clearListens();
								ROS_DEBUG("Client cleared listened topics ");
								//cout << "Client cleared listened topics " << endl;
							}

							if (msg_id == "-1")
							{
								it->setNodeID(msg_id);
							}

							string msgtimestamp = get_xml_element(client_msg, "TimeStamp");
							type_it = std::find(types.begin(), types.end(), Type(msgtype) );
							if (type_it == types.end())
							{
								Type type1 (msgtype);
								type1.setData(msgdata);
		                        types.push_back(type1);
		                        ROS_DEBUG("New type added: %s with data: %s", msgtype.c_str(), msgdata.c_str());
		                        //cout << "New type added: " << msgtype << " with data: " << msgdata << endl;
							}
							else
							{
								type_it->setData(msgdata);
								ROS_DEBUG("%s Type data updated: %s", msgtype.c_str(), msgdata.c_str());
								//cout << msgtype << " Type data updated: " << msgdata << endl;
							}
							

							distributeMessage(msg_id,msgtype,msgdata,msgtimestamp,events[i].data.fd);
							//clientsInfo();
							smfwin::SMF smf_msg;
							std_msgs::String smf_type_msg;
							smf_type_msg.data = msgtype;
							smf_msg.type = msgtype;
							smf_msg.data = msgdata;
							smf_pub.publish(smf_msg);
							smf_type_pub.publish(smf_type_msg);
	                    }

	                    if (s == -1)
	                      {
	                        //perror ("write");
	                        ROS_ERROR("write");
	                        abort ();
	                      }
	                  }

	                if (done)
	                  {
	                  	ROS_INFO("Closed connection on descriptor %d", events[i].data.fd);
	                    //printf ("Closed connection on descriptor %d\n", events[i].data.fd);

	                    /* Closing the descriptor will make epoll remove it
	                       from the set of descriptors which are monitored. */
	                    close (events[i].data.fd);
	                    it = std::find(clients.begin(), clients.end(), Node(events[i].data.fd) );
	                    clients.erase(it);
	                  }
	              }
	          }

	  	counter++;
    ros::spinOnce();
    loop_rate.sleep();
  }

  free (events);

  close (sfd);

  return EXIT_SUCCESS;
}
