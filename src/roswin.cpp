#include "ros/ros.h"
#include "std_msgs/String.h"

#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <signal.h>

void dostuff(int); /* function prototype */
void error(const char *msg)
{
    perror(msg);
    exit(1);
}
std_msgs::String msg;
int main(int argc, char **argv)
{

  ros::init(argc, argv, "talker");
  ros::NodeHandle n;
  ros::Publisher chatter_pub = n.advertise<std_msgs::String>("chatter", 1000);
  ros::Rate loop_rate(1);
  ROS_INFO("1\n");
  int sockfd, newsockfd, portno, pid;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  // SOCKET PART
  if (argc < 2) {
      fprintf(stderr,"ERROR, no port provided\n");
      exit(1);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  int opts;
  opts = fcntl(sockfd,F_GETFL);
  opts = (opts | O_NONBLOCK);
  fcntl(sockfd, F_SETFL, opts);  // set to non-blocking
  signal(SIGCHLD,SIG_IGN);
  if (sockfd < 0)
     error("ERROR opening socket");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
           sizeof(serv_addr)) < 0)
           error("ERROR on binding");
  listen(sockfd,5);
  clilen = sizeof(cli_addr);
  // END SOCKET PART
  ROS_INFO("listenin\n");
  while (ros::ok())
  {
	  //ROS_INFO("1st while\n");
	  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) continue; 
		if (fork() == 0) {
			dostuff(newsockfd);
    chatter_pub.publish(msg);
close(newsockfd); 
			exit(0);  // child terminates here
		}
		close(newsockfd); 
exit(0);

    ros::spinOnce();
    loop_rate.sleep();
  }

  close(sockfd);
  return 0;
}

void dostuff (int sock)
{
   int n;
   char buffer[256];

   bzero(buffer,256);
   n = read(sock,buffer,255);
   if (n < 0) error("ERROR reading from socket");
   printf("Here is the message: %s\n",buffer);
   msg.data= "..";
   n = write(sock,"I got your message",18);
   if (n < 0) error("ERROR writing to socket");
}
