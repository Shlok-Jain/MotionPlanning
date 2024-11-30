#include <iostream>
#include <cmath>
#include <vector>

#include "Coordinate.hpp"
#include "PathPlanner.hpp"
#include "Visualize.hpp"

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "std_msgs/msg/float32_multi_array.hpp"

using namespace std;

double min(double a, double b) { return a < b ? a : b; }
double max(double a, double b) { return a > b ? a : b; }

double set_angle(double theta)
{

    while (theta > M_PI)
    {
        theta -= 2 * M_PI;
    }
    while (theta < -M_PI)
    {
        theta += 2 * M_PI;
    }
    return theta;
}

Visualize *window = new Visualize(1600);
double runTime = 0.01;
double A = 22, B = 14;
optimalPlanner plannerType = PLANNER_RRTSTAR;
planningObjective objectiveType = OBJECTIVE_PATHLENGTH;

long long int iteration_count = 0;
int flag = 0;
double obs_size = 0.5;

std::vector<Point2D> obs(10), targetPos;
Point2D nowPos(0, 0, 0), finalPos(7, 1, 0), ballPos(0, 0, 0);
std::vector<pair<double, double>> points;

int findclosestpoint(std::vector<Point2D> &targetPos, Point2D &nowPos)
{
    double min_dist = 1000000;
    int idx = 0;
    for (int i = 0; i < targetPos.size(); i++) // excluding last point and start
    {
        if (sqrtl((targetPos[i].x - nowPos.x) * (targetPos[i].x - nowPos.x) +
                  (targetPos[i].y - nowPos.y) * (targetPos[i].y - nowPos.y)) <
            min_dist)
        {
            min_dist =
                sqrt((targetPos[i].x - nowPos.x) * (targetPos[i].x - nowPos.x) +
                     (targetPos[i].y - nowPos.y) * (targetPos[i].y - nowPos.y));
            idx = i;
        }
    }
    return idx;
}

// Function to check if a point lies within the extended region of a line
bool check_path(const Point2D &path_point, const Point2D &obs_point)
{
    double dist = sqrt((obs_point.x - path_point.x) * (obs_point.x - path_point.x) + (obs_point.y - path_point.y) * (obs_point.y - path_point.y));
    return (dist > obs_size); // Check if the distance is within the extended region
}

class ListenerNode : public rclcpp::Node
{
public:
    ListenerNode() : Node("listener_node")
    {
        // Create subscribers for two topics
        o1_subscriber = this->create_subscription<std_msgs::msg::Float32MultiArray>("o1_data", 10, std::bind(&ListenerNode::o1_callback, this, std::placeholders::_1));
        o2_subscriber = this->create_subscription<std_msgs::msg::Float32MultiArray>("o2_data", 10, std::bind(&ListenerNode::o2_callback, this, std::placeholders::_1));
        o3_subscriber = this->create_subscription<std_msgs::msg::Float32MultiArray>("o3_data", 10, std::bind(&ListenerNode::o3_callback, this, std::placeholders::_1));
        o4_subscriber = this->create_subscription<std_msgs::msg::Float32MultiArray>("o4_data", 10, std::bind(&ListenerNode::o4_callback, this, std::placeholders::_1));
        o5_subscriber = this->create_subscription<std_msgs::msg::Float32MultiArray>("o5_data", 10, std::bind(&ListenerNode::o5_callback, this, std::placeholders::_1));
        b1_subscriber = this->create_subscription<std_msgs::msg::Float32MultiArray>("b1_data", 10, std::bind(&ListenerNode::b1_callback, this, std::placeholders::_1));
        b2_subscriber = this->create_subscription<std_msgs::msg::Float32MultiArray>("b2_data", 10, std::bind(&ListenerNode::b2_callback, this, std::placeholders::_1));
        b3_subscriber = this->create_subscription<std_msgs::msg::Float32MultiArray>("b3_data", 10, std::bind(&ListenerNode::b3_callback, this, std::placeholders::_1));
        b4_subscriber = this->create_subscription<std_msgs::msg::Float32MultiArray>("b4_data", 10, std::bind(&ListenerNode::b4_callback, this, std::placeholders::_1));
        b5_subscriber = this->create_subscription<std_msgs::msg::Float32MultiArray>("b5_data", 10, std::bind(&ListenerNode::b5_callback, this, std::placeholders::_1));
        ball_subscriber = this->create_subscription<std_msgs::msg::Float32MultiArray>("ball_data", 10, std::bind(&ListenerNode::ball_callback, this, std::placeholders::_1));
        decision_target_subscriber = this->create_subscription<std_msgs::msg::Float32MultiArray>("decision_target_data", 10, std::bind(&ListenerNode::decision_target_callback, this, std::placeholders::_1));
        target_array_publisher = this->create_publisher<std_msgs::msg::Float32MultiArray>("target_pos", 10);
    }

    // Callback funtions
    void o1_callback(const std_msgs::msg::Float32MultiArray::SharedPtr msg)
    {
        nowPos = Point2D(msg->data[0], msg->data[1], msg->data[2]);
        my_pose_received = true;
        listening();
    }
    void o2_callback(const std_msgs::msg::Float32MultiArray::SharedPtr msg)
    {
        obs[0] = Point2D(msg->data[0], msg->data[1], msg->data[2]);
        bot1_pose_received = true;
    }
    void o3_callback(const std_msgs::msg::Float32MultiArray::SharedPtr msg)
    {
        obs[1] = Point2D(msg->data[0], msg->data[1], msg->data[2]);
        bot2_pose_received = true;
    }
    void o4_callback(const std_msgs::msg::Float32MultiArray::SharedPtr msg)
    {
        obs[2] = Point2D(msg->data[0], msg->data[1], msg->data[2]);
        bot3_pose_received = true;
    }
    void o5_callback(const std_msgs::msg::Float32MultiArray::SharedPtr msg)
    {
        obs[3] = Point2D(msg->data[0], msg->data[1], msg->data[2]);
        bot4_pose_received = true;
    }
    void b1_callback(const std_msgs::msg::Float32MultiArray::SharedPtr msg)
    {
        obs[4] = Point2D(msg->data[0], msg->data[1], msg->data[2]);
        bot5_pose_received = true;
    }
    void b2_callback(const std_msgs::msg::Float32MultiArray::SharedPtr msg)
    {
        obs[5] = Point2D(msg->data[0], msg->data[1], msg->data[2]);
        bot6_pose_received = true;
    }
    void b3_callback(const std_msgs::msg::Float32MultiArray::SharedPtr msg)
    {
        obs[6] = Point2D(msg->data[0], msg->data[1], msg->data[2]);
        bot7_pose_received = true;
    }
    void b4_callback(const std_msgs::msg::Float32MultiArray::SharedPtr msg)
    {
        obs[7] = Point2D(msg->data[0], msg->data[1], msg->data[2]);
        bot8_pose_received = true;
    }
    void b5_callback(const std_msgs::msg::Float32MultiArray::SharedPtr msg)
    {
        obs[8] = Point2D(msg->data[0], msg->data[1], msg->data[2]);
        bot9_pose_received = true;
    }
    void ball_callback(const std_msgs::msg::Float32MultiArray::SharedPtr msg)
    {
        ballPos = Point2D(msg->data[0], msg->data[1], 0);
        obs[9] = Point2D(msg->data[0], msg->data[1], 0);
        ball_pose_received = true;
    }
    void decision_target_callback(const std_msgs::msg::Float32MultiArray::SharedPtr msg)
    {
        cout << "New decision terget data received" << endl;
        finalPos = Point2D(msg->data[0], msg->data[1], msg->data[2]);
        target_pose_received = true;
    }

    void listening()
    {
        if (!my_pose_received || !bot1_pose_received || !bot2_pose_received || !bot2_pose_received || !bot3_pose_received || !bot4_pose_received || !bot5_pose_received || !bot6_pose_received || !bot7_pose_received || !bot8_pose_received || !bot9_pose_received || !ball_pose_received || !target_pose_received)
            return;

        // check if isok is true for all points in planned path
        for (int i = 0; i < targetPos.size() && flag == 0; i++)
        {
            for (auto curr_obs : obs)
            {
                if (!check_path(targetPos[i], curr_obs))
                {
                    flag = 1;
                    break;
                }
            }
        }

        if (flag || iteration_count == 0 || (targetPos.size() > 0 && (finalPos.x != targetPos.back().x || finalPos.y != targetPos.back().y)))
        {
            try
            {
                targetPos = plan(runTime, A, B, obs, plannerType, objectiveType, nowPos, finalPos);
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
                std::cout << "Could not find path" << std::endl;
                return;
            }

            flag = 0;
        }

        iteration_count++;

        int idx = findclosestpoint(targetPos, nowPos);

        window->visualizeGame(targetPos, nowPos, idx, nowPos.theta, obs, ballPos);

        publish_next_point(targetPos, nowPos);
    }

    // Variables to store received messages
    bool my_pose_received{false}, bot1_pose_received{false}, bot2_pose_received{false}, bot3_pose_received{false}, bot4_pose_received{false}, bot5_pose_received{false}, bot6_pose_received{false}, bot7_pose_received{false}, bot8_pose_received{false}, bot9_pose_received{false}, ball_pose_received{false}, target_pose_received{true};

    // Subscribers
    rclcpp::Subscription<std_msgs::msg::Float32MultiArray>::SharedPtr o1_subscriber, o2_subscriber, o3_subscriber, o4_subscriber, o5_subscriber, b1_subscriber, b2_subscriber, b3_subscriber, b4_subscriber, b5_subscriber, ball_subscriber, decision_target_subscriber;

    // Publisher
    rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr target_array_publisher;

    void publish_next_point(std::vector<Point2D> &path, Point2D &nowPos)
    {
        if (!path.empty())
        {
            int idx = findclosestpoint(path, nowPos);

            auto next_point = path[min(idx + 1, path.size() - 1)];
            auto next_to_next_point = path[min(idx + 2, path.size() - 1)];

            double x1 = next_point.x;
            double y1 = next_point.y;

            double x2 = next_to_next_point.x;
            double y2 = next_to_next_point.y;

            double angle = atan2((y2 - y1), (x2 - x1));

            if (idx + 2 == path.size() - 1 && idx + 1 != path.size() - 1)
            {
                double angle = atan2((y1 - path[idx].y), (x1 - path[idx].x));
            }
            if (idx + 2 == path.size() - 1 && idx + 1 == path.size() - 1)
            {
                double angle = next_point.theta;
            }
            double dist = sqrt((y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1));
            double c = 1;
            double lambda = 0.5;
            double speed = c * (dist - lambda);

            cout << "distance between next points: " << dist << endl;

            if (idx + 1 == path.size() - 1)
            {
                speed = 0;
            }

            cout << "Next Point: " << x1 << " " << y1 << " " << angle << endl;

            double vx = speed * cos(angle);
            double vy = speed * sin(angle);

            double bot_global_angle = set_angle(nowPos.theta);
            double ball_wrto_bot = set_angle(atan2((ballPos.y - nowPos.y), (ballPos.x - nowPos.x)));
            double rel_angle = set_angle(ball_wrto_bot - bot_global_angle);

            rel_angle = min(rel_angle, 0.3);
            rel_angle = max(rel_angle, -0.3);

            double theta = set_angle(rel_angle + bot_global_angle);
            double omega = 0;

            std_msgs::msg::Float32MultiArray message;
            message.data = {x1, y1, theta, vx, vy, omega};
            target_array_publisher->publish(message);
        }
    }
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ListenerNode>());
    rclcpp::shutdown();
    return 0;
}
