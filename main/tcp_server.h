/**
 * @file tcp_server.h
 * @author Czira Bence (czirabence@gmail.com)
 * 
 * @brief This is a modified version of esp-idf/examples/protocols/sockets/tcp_server. 
 * Choose IPv4, IPv6 or both via setting <b>EXAMPLE_IPV4 and EXAMPLE_IPV6</b>,
 * set server port number with EXAMPLE_PORT, keep-alive idle time, interval time
 * and package resend count with <b>EXAMPLE_KEEPALIVE_IDLE, EXAMPLE_KEEPALIVE_INTERVAL
 * and EXAMPLE_KEEPALIVE_COUNT</b> under <b>TCP Server Configuration</b> submenu in project configuration menu.
 * 
 * @version 0.1
 * @date 2023-06-12
 */
#ifndef TCP_SERVER_H
#define TCP_SERVER_H

enum TCP_server_state{
    Connected,
    Disconnected
};

/**
 * @brief Indicate TCP server-client connection state
 */
extern enum TCP_server_state server_state; 

/**
 * @brief Initialise and run tcp server to send messages to client periodically.
 * 
 * @param pvParameters - pointer to char array, which needs to be transmitted.
 * Content of the array can change between two transmission cycles.
 */
void tcp_server_task(void *pvParameters);

#endif //__TCP_SERVER_H__