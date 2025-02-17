#include <stdio.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <rmw_microros/rmw_microros.h>

#include "pico/stdlib.h"
#include "pico_uart_transports.h"

const uint LED_PIN = 25;
int period = 10;

rcl_subscription_t subscriber;
rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;
std_msgs__msg__Int32 recv_msg;

void timer_callback(rcl_timer_t *timer, int64_t last_call_time)
{
    rcl_ret_t ret = rcl_publish(&publisher, &msg, NULL);
    msg.data++;
    //period++;
/*    
	static int toggle = 0;
	
	toggle++;
	if(toggle%2){
		gpio_put(LED_PIN, 1);
	}else{
		gpio_put(LED_PIN, 0);
	}    
  */  
}

void timer_blinky_callback(rcl_timer_t *timer, int64_t last_call_time)
{
	static int cnt = 0;
	
	cnt++;
	if(cnt<period/2){
		gpio_put(LED_PIN, 1);
	}else{
		gpio_put(LED_PIN, 0);
	}
	if(cnt >= period){
		cnt = 0;
	}
	
}

void subscription_callback(const void * msgin)
{
/*
	const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;
	printf("Received: %d\n", msg->data);
	*/
	const double * msg = (const double *)msgin;
	period = (int)1/(*msg);
	
	
	/*
	static int cnt = 0;
	
	cnt++;
	if(cnt<period/2){
		gpio_put(LED_PIN, 1);
	}else{
		gpio_put(LED_PIN, 0);
	}
	if(cnt >= period){
		cnt = 0;
	}	
	*/
	
	
}

int main()
{
    rmw_uros_set_custom_transport(
		true,
		NULL,
		pico_serial_transport_open,
		pico_serial_transport_close,
		pico_serial_transport_write,
		pico_serial_transport_read
	);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    rcl_timer_t timer;
    rcl_timer_t timer_blinky;
    
    rcl_node_t node;
    rcl_allocator_t allocator;
    rclc_support_t support;
    rclc_executor_t executor;

    allocator = rcl_get_default_allocator();

    // Wait for agent successful ping for 2 minutes.
    const int timeout_ms = 1000; 
    const uint8_t attempts = 120;

    rcl_ret_t ret = rmw_uros_ping_agent(timeout_ms, attempts);

    if (ret != RCL_RET_OK)
    {
        // Unreachable agent, exiting program.
        return ret;
    }

    rclc_support_init(&support, 0, NULL, &allocator);

    rclc_node_init_default(&node, "tool_node", "", &support);
    rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "tool_publisher");

    rclc_subscription_init_default(
	&subscriber,
	&node,
	ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
	"UR20215300001/speed_scaling");        
        

    rclc_timer_init_default(
        &timer,
        &support,
        RCL_MS_TO_NS(1000),
        timer_callback);

    rclc_timer_init_default(
        &timer_blinky,
        &support,
        RCL_MS_TO_NS(100),
        timer_blinky_callback);        
        

    rclc_executor_init(&executor, &support.context, 3, &allocator);
    rclc_executor_add_timer(&executor, &timer);
    rclc_executor_add_timer(&executor, &timer_blinky);
    rclc_executor_add_subscription(&executor, &subscriber, &recv_msg, &subscription_callback, ON_NEW_DATA);


    gpio_put(LED_PIN, 1);

    msg.data = 0;
    while (true)
    {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
    }
    return 0;
}
