#ifndef SSHIFTER_H_
#define SSHIFTER_H_

#include <ros/ros.h>
#include <topic_tools/shape_shifter.h>

extern "C"
{
	ros::NodeHandle* InitializeNode();
	void DisposeNode(ros::NodeHandle* node);
	bool RosOk();
	void RosSpinOnce();
	ros::Publisher* Advertise(ros::NodeHandle* node, const char* topicName, unsigned int queueLength);
	void DisposePublisher(ros::Publisher* publisher);
	ros::Subscriber* Subscribe(ros::NodeHandle* node, const char* topicName, unsigned int queueLength, void (*callback)(topic_tools::ShapeShifter::ConstPtr));
	void DisposeSubscriber(ros::Subscriber* subscriber);
	void RosSpinOnce();
	const char* ShapeShifterGetDataType(topic_tools::ShapeShifter::ConstPtr message);
	const char* ShapeShifterGetDefinition(topic_tools::ShapeShifter::ConstPtr message);
	unsigned char* ShapeShifterGetData(topic_tools::ShapeShifter::ConstPtr message);
	unsigned int ShapeShifterGetDataLength(topic_tools::ShapeShifter::ConstPtr messsage);
}

#endif

