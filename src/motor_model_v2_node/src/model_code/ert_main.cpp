#include <chrono>
#include "rclcpp/rclcpp.hpp"
#include "output_interface.h"
#include "generated_model.h"
/* MSG INCLUDE */
#include "data_interfaces/msg/msg_mcu_output.hpp"
#include "data_interfaces/msg/msg_dyno_cmd.hpp"
#include "data_interfaces/msg/msg_motor_output.hpp"
#include "data_interfaces/msg/msg_dyno_sensing.hpp"
using namespace std::chrono_literals;
using std::placeholders::_1;

/* USING */
using stMsgMcuOutput = data_interfaces::msg::MsgMcuOutput;
using stMsgDynoCmd = data_interfaces::msg::MsgDynoCmd;
using stMsgMotorOutput = data_interfaces::msg::MsgMotorOutput;
using stMsgDynoSensing = data_interfaces::msg::MsgDynoSensing;

/* VARIABLE */
stMsgMcuOutput::SharedPtr inputData1;
stMsgDynoCmd::SharedPtr inputData2;

class MotorModelV2Node : public rclcpp::Node
{
public:
  MotorModelV2Node()
  : Node("motor_model_v2_node"), count_(0)
  {
	  /* NODE INIT */
    subscription1 = this->create_subscription<stMsgMcuOutput>("msg_mcu_output", std::bind(&MotorModelV2Node::subscription_callback1, this, _1));
    subscription2 = this->create_subscription<stMsgDynoCmd>("msg_dyno_cmd", std::bind(&MotorModelV2Node::subscription_callback2, this, _1));
    publisher1 = this->create_publisher<stMsgMotorOutput>("msg_motor_output");
    publisher2 = this->create_publisher<stMsgDynoSensing>("msg_dyno_sensing");
        timer_ = this->create_wall_timer(1ms, std::bind(&MotorModelV2Node::timer_callback, this));
  }

private:
	/* TIMER CALLBACK */
  void timer_callback()
  {
    static int d1_max = 0, d1_min = 100000, d1_avg = 0, d1_sum = 0;
    static int d2_max = 0, d2_min = 100000, d2_avg = 0, d2_sum = 0;
    static int d3_max = 0, d3_min = 100000, d3_avg = 0, d3_sum = 0;
    static int i = 0;
    auto t1 = std::chrono::steady_clock::now();
    generated_model_step();
    auto t2 = std::chrono::steady_clock::now();
    publisher1->publish(outputData1);
    publisher2->publish(outputData2);
    auto t3 = std::chrono::steady_clock::now();
    auto d1 = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    auto d2 = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();
    auto d3 = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t1).count();
    if (d1 > d1_max) d1_max = d1;
    if (d1 < d1_min) d1_min = d1;
    if (d2 > d2_max) d2_max = d2;
    if (d2 < d2_min) d2_min = d2;
    if (d3 > d3_max) d3_max = d3;
    if (d3 < d3_min) d3_min = d3;
    if (i >= 999)
    {
      d1_avg = (int)(d1_sum / 1000.0);
      d2_avg = (int)(d2_sum / 1000.0);
      d3_avg = (int)(d3_sum / 1000.0);
      d1_sum = d1_avg;
      d2_sum = d2_avg;
      d3_sum = d3_avg;
      i = 0;
    }
    else
    {
      d1_sum += d1;
      d2_sum += d2;
      d3_sum += d3;
      i++;
    }
    printf("\e[2J\e[H");
    printf("------------------------------------------------------------------------\n");
    printf("Measured duration\tInstant(ns)\tMax(ns)\t\tMin(ns)\tAvg(ns)\n");
    printf("------------------------------------------------------------------------\n");
    printf("Model\t\t\t%d\t\t%d\t\t%d\t%d\n", d1, d1_max, d1_min, d1_avg);
    printf("Publish\t\t\t%d\t\t%d\t\t%d\t%d\n", d2, d2_max, d2_min, d2_avg);
    printf("Total\t\t\t%d\t\t%d\t\t%d\t%d\n", d3, d3_max, d3_min, d3_avg);
  }
  	
    /* SUB CALLBACK */
  void subscription_callback1(const stMsgMcuOutput::SharedPtr msg)
  {
    inputData1 = msg;
  }
  void subscription_callback2(const stMsgDynoCmd::SharedPtr msg)
  {
    inputData2 = msg;
  }
    
    /* NODE DECLARATION */
  rclcpp::Subscription<stMsgMcuOutput>::SharedPtr subscription1;
  rclcpp::Subscription<stMsgDynoCmd>::SharedPtr subscription2;
  rclcpp::Publisher<stMsgMotorOutput>::SharedPtr publisher1;
  rclcpp::Publisher<stMsgDynoSensing>::SharedPtr publisher2;
    rclcpp::TimerBase::SharedPtr timer_;
    size_t count_;
};

std::shared_ptr<MotorModelV2Node> model_node = nullptr;

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  model_node = std::make_shared<MotorModelV2Node>();
	/* INSTANTIATION */
  inputData1 = std::make_shared<stMsgMcuOutput>();
  inputData2 = std::make_shared<stMsgDynoCmd>();
  outputInterfaceInit();
  generated_model_initialize();
  rclcpp::spin(model_node);
  generated_model_terminate();
  rclcpp::shutdown();
  return 0;
}
