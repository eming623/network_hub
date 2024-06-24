/*
 * cspt_kernel.c
 *
 *  Created on: Sep 15, 2014
 *      Author: Yiming Xu
 */
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include "cspt_kernel.h"
#include "cspt_config.h"
#include "cspt_adapter.h"

node_table_s node_table;
variable_table_s var_table;
message_table_s message_table;
command_table_s cmd_table;

int32 g_epoll_fd;

typedef struct CMD_MAP_
{
    uint32 ulNum;
    cmd_func cmdFunc;
} command_mapping_info;

command_mapping_info g_command_mapping_table_arr[] =
{
{ 0, kernel_send },
{ 1, kernel_wait },
{ 2, kernel_goto },
{ 3, kernel_sleep },
{ 4, kernel_evaluate },
{ 5, kernel_operation } //include + - * / % > <
};

void* kernel_entry(void *arg)
{
    printf("%s-%u:Enter kernel_entry\r\n", __FILE__, __LINE__);
    int32 message_id;
    void *message_ptr;
    tcp_client_message_header_s *message_header_ptr;
    while (1)
    {
        srv_msg_rcv(TID_KERNEL, &message_id, &message_ptr);
        if (TID_TCPSERVER != srv_msg_getsrctid(message_ptr))
        {
            srv_msg_free(message_ptr);
            continue;
        }
        message_header_ptr = (tcp_client_message_header_s*) message_ptr;
        printf("--=== Message type = %d ===-- \n",
                message_header_ptr->message_type);
        switch (message_header_ptr->message_type & 0xFF000000)
        {
        case KERNEL_MSG_PRECOMPILE:
            if (SRV_ERR
                    == precompile(message_ptr,
                            message_header_ptr->message_type))
            {
                srv_msg_free(message_ptr);
                return NULL;
            }
            srv_msg_free(message_ptr);
            break;

        case KERNEL_MSG_RUN:
            if (SRV_OK != case_run(((case_run_s*) message_ptr)->command_id))
            {
                printf("%s-%u:Case run failed\r\n", __FILE__, __LINE__);
            }
            break;
        default:
            break;
        }

    }
    return NULL;
}

int32 precompile(void *message_ptr, uint32 message_type)
{
    printf("--=== message_type=%d ===-- \n", message_type);
    switch (message_type)
    {
    case PRECOMPILE_MSG_TOPO:
        if (SRV_ERR == nodetable_create(message_ptr))
        {
            return SRV_ERR;
        }
        break;
    case PRECOMPILE_MSG_VAR:
        if (SRV_ERR == vartable_create(message_ptr))
        {
            return SRV_ERR;
        }
        break;
    case PRECOMPILE_MSG_IMAGE:
        if (SRV_ERR == imagetable_create(message_ptr))
        {
            return SRV_ERR;
        }
        break;
    case PRECOMPILE_MSG_CONTENT:
        if (SRV_ERR == add_msg(message_ptr))
        {
            return SRV_ERR;
        }
        break;
    case PRECOMPILE_MSG_COMMAND:
        if (SRV_ERR == cmdtable_create(message_ptr))
        {
            return SRV_ERR;
        }
        break;
    default:
        break;
    }
    return SRV_OK;
}

int32 nodetable_cleanup()
{
    uint32 index;
    struct epoll_event ev;
    ev.events = EPOLLIN;

    for (index = 0; index < node_table.node_num; index++)
    {
        if ((node_table.node_info_arr[index].node_type == UDP_NODE_TYPE_SEND
                || node_table.node_info_arr[index].node_type
                        == UDP_NODE_TYPE_RECV)
                && node_table.node_info_arr[index].info.udp_node_info.flag
                        == UDP_NODE_FLAG_LOCAL)
        {
            if (node_table.node_info_arr[index].info.udp_node_info.socket > 0)
            {
                if (node_table.node_info_arr[index].node_type
                        == UDP_NODE_TYPE_RECV)
                {
                    ev.data.fd =
                            node_table.node_info_arr[index].info.udp_node_info.socket;
                    if (-1
                            == epoll_ctl(g_epoll_fd, EPOLL_CTL_DEL, ev.data.fd,
                                    &ev))
                    {
                        printf("%s-%u:epoll delete failed.\r\n", __FILE__,
                                __LINE__);
                        return SRV_ERR;
                    }
                }
                if (SRV_ERR
                        == srv_skt_close(
                                (service_handle) node_table.node_info_arr[index].info.udp_node_info.socket))
                {
                    printf("%s-%u:socket close failed.\r\n", __FILE__,
                            __LINE__);
                    return SRV_ERR;
                }
            }
        }
    }

    bzero(node_table.node_info_arr, sizeof(node_info) * MAX_NODE_NUM);
    node_table.node_num = 0;

    return SRV_OK;
}

int32 create_bind_socket(uint32 ip_addr, uint16 port)
{
    int32 socket_handle;
    struct sockaddr_in addr;

    socket_handle = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == socket_handle)
    {
        return SRV_INVALID;
    }

    addr.sin_family = AF_INET;
    if (0 == ip_addr)
    {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        addr.sin_addr.s_addr = htonl(ip_addr);
    }
    addr.sin_port = htons(port);
    if (-1 == bind(socket_handle, (struct sockaddr*) &addr, sizeof(addr)))
    {
        close(socket_handle);
        return SRV_INVALID;
    }

    return socket_handle;
}

uint32 find_node_by_ip_port(uint32 start, uint32 end, uint32 ip_addr,
        uint16 port)
{
    uint32 index;
    for (index = start; index < end; index++)
    {
        if (node_table.node_info_arr[index].info.udp_node_info.flag
                == UDP_NODE_FLAG_LOCAL
                && node_table.node_info_arr[index].info.udp_node_info.ip_addr
                        == ip_addr
                && node_table.node_info_arr[index].info.udp_node_info.port
                        == port)
        {
            return index;
        }
    }
    return SRV_INVALID;
}

int32 nodetable_create(void *message_ptr)
{
    config_node_info *config_node_info_ptr;
    uint32 udp_send_node_num;
    uint32 udp_receive_node_num;
    uint32 index;
    int32 socket;
    uint32 node_index;
    struct epoll_event ev;

    ev.events = EPOLLIN;

    if (NULL == message_ptr)
    {
        printf("%s-%u:null error\r\n", __FILE__, __LINE__);
        return SRV_ERR;
    }

    //clean up
    if (SRV_OK != nodetable_cleanup())
    {
        printf("%s-%u:node table clean up failed\r\n", __FILE__, __LINE__);
        return SRV_ERR;
    }

    if (0 == g_epoll_fd || -1 == g_epoll_fd)
    {
        g_epoll_fd = epoll_create(MAX_UDP_RECV_NUM);
        if (-1 == g_epoll_fd)
        {
            printf("%s-%u:epoll create failed.\r\n", __FILE__, __LINE__);
            return SRV_ERR;
        }
    }

    config_node_info_ptr = (config_node_info*) message_ptr;

    udp_send_node_num = config_node_info_ptr->udp_send_node_num;
    udp_receive_node_num = config_node_info_ptr->udp_receive_node_num;

    if (udp_send_node_num + udp_receive_node_num > MAX_NODE_NUM)
    {
        printf("%s-%u:Node number exceed\r\n", __FILE__, __LINE__);
        return SRV_ERR;
    }

    node_table.node_num = udp_send_node_num + udp_receive_node_num;

    for (index = 0; index < udp_send_node_num; index++)
    {
        node_table.node_info_arr[index].node_id = index;
        node_table.node_info_arr[index].node_type = UDP_NODE_TYPE_SEND;
        node_table.node_info_arr[index].info.udp_node_info.ip_addr =
                config_node_info_ptr->config_udp_send_info_arr[index].ip_addr;
        node_table.node_info_arr[index].info.udp_node_info.port =
                config_node_info_ptr->config_udp_send_info_arr[index].port;
        node_table.node_info_arr[index].info.udp_node_info.flag =
                config_node_info_ptr->config_udp_send_info_arr[index].flag;
        switch (config_node_info_ptr->config_udp_send_info_arr[index].flag)
        {
        case UDP_NODE_FLAG_LOCAL:
            socket = create_bind_socket(
                    node_table.node_info_arr[index].info.udp_node_info.ip_addr,
                    node_table.node_info_arr[index].info.udp_node_info.port);
            if (SRV_INVALID == socket)
            {
                printf("%s-%u:create socket failed\r\n", __FILE__, __LINE__);
                return SRV_ERR;
            }
            node_table.node_info_arr[index].info.udp_node_info.socket = socket;
            break;
        case UDP_NODE_FLAG_PEER:
            //do nothing
            break;
        default:
            printf("%s-%u:udp node flag wrong\r\n", __FILE__, __LINE__);
            return SRV_ERR;
        }
    }

    for (index = 0; index < udp_receive_node_num; index++)
    {
        node_table.node_info_arr[index + udp_send_node_num].node_id = index
                + udp_send_node_num;
        node_table.node_info_arr[index + udp_send_node_num].node_type =
                UDP_NODE_TYPE_RECV;
        node_table.node_info_arr[index + udp_send_node_num].info.udp_node_info.ip_addr =
                config_node_info_ptr->config_udp_recv_info_arr[index].ip_addr;
        node_table.node_info_arr[index + udp_send_node_num].info.udp_node_info.port =
                config_node_info_ptr->config_udp_recv_info_arr[index].port;
        node_table.node_info_arr[index + udp_send_node_num].info.udp_node_info.flag =
                config_node_info_ptr->config_udp_recv_info_arr[index].flag;
        switch (config_node_info_ptr->config_udp_recv_info_arr[index].flag)
        {
        case UDP_NODE_FLAG_LOCAL:
            //See if the ip&port is in udp send node
            node_index =
                    find_node_by_ip_port(0, udp_send_node_num,
                            node_table.node_info_arr[index + udp_send_node_num].info.udp_node_info.ip_addr,
                            node_table.node_info_arr[index + udp_send_node_num].info.udp_node_info.port);
            if (SRV_INVALID != node_index)
            {
                socket =
                        node_table.node_info_arr[node_index].info.udp_node_info.socket;
            }
            else
            {
                socket =
                        create_bind_socket(
                                node_table.node_info_arr[index
                                        + udp_send_node_num].info.udp_node_info.ip_addr,
                                node_table.node_info_arr[index
                                        + udp_send_node_num].info.udp_node_info.port);
                if (SRV_INVALID == socket)
                {
                    printf("%s-%u:create socket failed\r\n", __FILE__,
                            __LINE__);
                    return SRV_ERR;
                }
            }
            node_table.node_info_arr[index + udp_send_node_num].info.udp_node_info.socket =
                    socket;

            //Add socket to epoll
            if (-1
                    == fcntl(socket, F_SETFL,
                            fcntl(socket, F_GETFL, 0) | O_NONBLOCK))
            {
                printf("%s-%u:fcntl failed\r\n", __FILE__, __LINE__);
                return SRV_ERR;
            }
            ev.data.fd = socket;
            if (epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, socket, &ev) == -1)
            {
                printf("%s-%u:epoll add failed\r\n", __FILE__, __LINE__);
                return SRV_ERR;
            }

            break;
        case UDP_NODE_FLAG_PEER:
            //do nothing
            break;
        default:
            printf("%s-%u:udp node flag wrong\r\n", __FILE__, __LINE__);
            return SRV_ERR;
        }
    }

    return SRV_OK;
}

int32 vartable_create(void *message_ptr)
{
    uint32 index;
    variable_table_s *var_table_ptr;

    if (NULL == message_ptr)
    {
        printf("%s-%u:null error\r\n", __FILE__, __LINE__);
        return SRV_ERR;
    }

    //Clean up
    for (index = 0; index < var_table.var_num; index++)
    {
        if (var_table.var_info_arr[index].value_ptr != NULL)
        {
            srv_buf_free(var_table.var_info_arr[index].value_ptr);
            var_table.var_info_arr[index].value_ptr = NULL;
        }
    }
    bzero(var_table.var_info_arr, sizeof(var_info_s) * MAX_VAR_NUM);
    var_table.var_num = 0;

    //Create table
    var_table_ptr = (variable_table_s*) message_ptr;
    if (var_table_ptr->var_num > MAX_VAR_NUM)
    {
        printf("%s-%u:Variable number exceed\r\n", __FILE__, __LINE__);
        return SRV_ERR;
    }
    var_table.var_num = var_table_ptr->var_num;
    for (index = 0; index < var_table.var_num; index++)
    {
        var_table.var_info_arr[index].var_id = index;
        var_table.var_info_arr[index].length =
                var_table_ptr->var_info_arr[index].length;
        switch (var_table.var_info_arr[index].length)
        {
        case VAR_TYPE_1_BYTE:
            var_table.var_info_arr[index].value_ptr = srv_buf_alloc(
                    VAR_TYPE_1_BYTE);
            if (NULL == var_table.var_info_arr[index].value_ptr)
            {
                printf("%s-%u:Buffer allocate failed\r\n", __FILE__, __LINE__);
                return SRV_ERR;
            }
            memset(var_table.var_info_arr[index].value_ptr, 0, VAR_TYPE_1_BYTE);
            break;
        case VAR_TYPE_2_BYTE:
            var_table.var_info_arr[index].value_ptr = srv_buf_alloc(
                    VAR_TYPE_2_BYTE);
            if (NULL == var_table.var_info_arr[index].value_ptr)
            {

                printf("%s-%u:Buffer allocate failed\r\n", __FILE__, __LINE__);
                return SRV_ERR;
            }
            memset(var_table.var_info_arr[index].value_ptr, 0, VAR_TYPE_2_BYTE);
            break;
        case VAR_TYPE_4_BYTE:
            var_table.var_info_arr[index].value_ptr = srv_buf_alloc(
                    VAR_TYPE_4_BYTE);
            if (NULL == var_table.var_info_arr[index].value_ptr)
            {

                printf("%s-%u:Buffer allocate failed\r\n", __FILE__, __LINE__);
                return SRV_ERR;
            }
            memset(var_table.var_info_arr[index].value_ptr, 0, VAR_TYPE_4_BYTE);
            break;
        default:

            printf("%s-%u:Variable length incorrect\r\n", __FILE__, __LINE__);
            return SRV_ERR;
        }
    }

    printf("%s-%u:Variable table create succeed\r\n", __FILE__, __LINE__);
    return SRV_OK;
}

int32 imagetable_create(void *message_ptr)
{
    uint32 index1;
    uint32 index2;
    message_table_s *message_table_ptr;

    if (NULL == message_ptr)
    {

        printf("%s-%u:null error\r\n", __FILE__, __LINE__);
        return SRV_ERR;
    }

    //Clean up
    for (index1 = 0; index1 < message_table.message_num; index1++)
    {
        if (message_table.message_info_arr[index1].message_ptr != NULL)
        {
            srv_buf_free(message_table.message_info_arr[index1].message_ptr);
            message_table.message_info_arr[index1].message_ptr = NULL;
        }
    }
    bzero(message_table.message_info_arr,
            sizeof(message_info_s) * MAX_IMAGE_NUM);
    message_table.message_num = 0;

    //Create table
    message_table_ptr = (message_table_s*) message_ptr;
    if (message_table_ptr->message_num > MAX_IMAGE_NUM)
    {
        printf("%s-%u:Image number exceed\r\n", __FILE__, __LINE__);
        return SRV_ERR;
    }
    message_table.message_num = message_table_ptr->message_num;
    for (index1 = 0; index1 < message_table.message_num; index1++)
    {
        message_table.message_info_arr[index1].message_id =
                message_table_ptr->message_info_arr[index1].message_id;
        message_table.message_info_arr[index1].message_ptr = NULL;
        message_table.message_info_arr[index1].message_len =
                message_table_ptr->message_info_arr[index1].message_len;
        message_table.message_info_arr[index1].var_num =
                message_table_ptr->message_info_arr[index1].var_num;
        message_table.message_info_arr[index1].key_num =
                message_table_ptr->message_info_arr[index1].key_num;
        for (index2 = 0;
                index2 < message_table.message_info_arr[index1].var_num;
                index2++)
        {
            message_table.message_info_arr[index1].var_offset_arr[index2].offset =
                    message_table_ptr->message_info_arr[index1].var_offset_arr[index2].offset;
            message_table.message_info_arr[index1].var_offset_arr[index2].var_id =
                    message_table_ptr->message_info_arr[index1].var_offset_arr[index2].var_id;
        }
        for (index2 = 0;
                index2 < message_table.message_info_arr[index1].key_num;
                index2++)
        {
            message_table.message_info_arr[index1].key_info_arr[index2].offset =
                    message_table_ptr->message_info_arr[index1].key_info_arr[index2].offset;
            message_table.message_info_arr[index1].key_info_arr[index2].length =
                    message_table_ptr->message_info_arr[index1].key_info_arr[index2].length;
            message_table.message_info_arr[index1].key_info_arr[index2].key_type =
                    message_table_ptr->message_info_arr[index1].key_info_arr[index2].key_type;
            message_table.message_info_arr[index1].key_info_arr[index2].var_id =
                    message_table_ptr->message_info_arr[index1].key_info_arr[index2].var_id;
        }
    }

    printf("%s-%u:Image table create succeed\r\n", __FILE__, __LINE__);
    return SRV_OK;
}

int32 add_msg(void *message_ptr)
{
    static uint32 message_count = 0;
    if (NULL == message_ptr)
    {
        message_count = 0;

        printf("%s-%u:null error\r\n", __FILE__, __LINE__);
        return SRV_ERR;
    }

    printf("%s-%u:Add message count:%d\r\n", __FILE__, __LINE__, message_count);
    if (++message_count == message_table.message_num)
    {
        message_count = 0;

        printf("%s-%u:Add all messages succeed\r\n", __FILE__, __LINE__);
    }
    return SRV_OK;
}

int32 cmdtable_create(void *message_ptr)
{
    uint32 index;
    uint32 arg_index;
    command_table_s *command_table_ptr = NULL;

    if (NULL == message_ptr)
    {

        printf("%s-%u:null error\r\n", __FILE__, __LINE__);
        return SRV_ERR;
    }

    //Clean up
    bzero(&cmd_table, sizeof(cmd_table));

    //Create table
    if (command_table_ptr->command_num > MAX_CMD_NUM)
    {

        printf("%s-%u:Command number exceed\r\n", __FILE__, __LINE__);
        return SRV_ERR;
    }
    command_table_ptr = (command_table_s*) message_ptr;
    cmd_table.command_num = command_table_ptr->command_num;
    for (index = 0; index < cmd_table.command_num; index++)
    {
        cmd_table.command_info_arr[index].command_id =
                command_table_ptr->command_info_arr[index].command_id;
        cmd_table.command_info_arr[index].command_func =
                g_command_mapping_table_arr[(uint32) command_table_ptr->command_info_arr[index].command_func].cmdFunc;
        cmd_table.command_info_arr[index].arg_num =
                command_table_ptr->command_info_arr[index].arg_num;
        for (arg_index = 0;
                arg_index < cmd_table.command_info_arr[index].arg_num;
                arg_index++)
        {
            cmd_table.command_info_arr[index].arg_info_arr[arg_index].arg_type =
                    command_table_ptr->command_info_arr[index].arg_info_arr[arg_index].arg_type;
            cmd_table.command_info_arr[index].arg_info_arr[arg_index].arg_value =
                    command_table_ptr->command_info_arr[index].arg_info_arr[arg_index].arg_value;
        }
        cmd_table.command_info_arr[index].true_next_command_id =
                command_table_ptr->command_info_arr[index].true_next_command_id;
        cmd_table.command_info_arr[index].false_next_command_id =
                command_table_ptr->command_info_arr[index].false_next_command_id;
    }

    printf("%s-%u:Command table create succeed\r\n", __FILE__, __LINE__);
    return SRV_OK;
}

int32 case_run(uint32 command_id)
{
    command_info_s *command_info_ptr;
    uint32 next_command_id;

    if (command_id >= cmd_table.command_num)
    {

        printf("%s-%u:Wrong command\r\n", __FILE__, __LINE__);
        return SRV_ERR;
    }

    next_command_id = command_id;

    while (1)
    {
        command_info_ptr = &(cmd_table.command_info_arr[next_command_id]);
        next_command_id = command_info_ptr->command_func(
                (void*) command_info_ptr);

        if (SRV_INVALID == next_command_id)
        {

            printf("%s-%u:Command execute failed\r\n", __FILE__, __LINE__);
            return SRV_ERR;
        }

        if (NULL == cmd_table.command_info_arr[next_command_id].command_func)
        {
            break;
        }
    }

    printf("%s-%u:Case run succeed\r\n", __FILE__, __LINE__);
    return SRV_OK;
}

uint32 kernel_send(void *arg_ptr)
{
    command_info_s *command_info_ptr;
    uint32 index;
    uint32 message_id;
    void *message_ptr;
    uint32 message_length;
    uint32 local_node_id;
    uint32 peer_node_id;

    uint32 var_num;
    uint32 offset;
    uint32 var_id;
    uint32 var_length;

    if (NULL == arg_ptr)
    {
        printf("%s-%u:null error\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }

    command_info_ptr = (command_info_s*) arg_ptr;

    message_id = command_info_ptr->arg_info_arr[0].arg_value;
    local_node_id = command_info_ptr->arg_info_arr[1].arg_value;
    peer_node_id = command_info_ptr->arg_info_arr[2].arg_value;

    if (node_table.node_info_arr[local_node_id].node_type != UDP_NODE_TYPE_SEND
            || node_table.node_info_arr[peer_node_id].node_type
                    != UDP_NODE_TYPE_SEND)
    {
        printf("%s-%u:It's not a udp send node\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }

    message_ptr = message_table.message_info_arr[message_id].message_ptr;
    message_length = message_table.message_info_arr[message_id].message_len;
    var_num = message_table.message_info_arr[message_id].var_num;

    for (index = 0; index < var_num; index++)
    {
        offset =
                message_table.message_info_arr[message_id].var_offset_arr[index].offset;
        var_id =
                message_table.message_info_arr[message_id].var_offset_arr[index].var_id;
        var_length = var_table.var_info_arr[var_id].length;
        switch (var_length)
        {
        case VAR_TYPE_1_BYTE:
            memcpy((int8*) message_ptr + offset,
                    var_table.var_info_arr[var_id].value_ptr, var_length);
            break;
        case VAR_TYPE_2_BYTE:
            memcpy((int8*) message_ptr + offset,
                    var_table.var_info_arr[var_id].value_ptr, var_length);
            //This is need to consider message is net order or host order
            *((uint16*) ((int8*) message_ptr + offset)) = htons(
                    *((uint16*) ((int8*) message_ptr + offset)));
            break;
        case VAR_TYPE_4_BYTE:
            memcpy((int8*) message_ptr + offset,
                    var_table.var_info_arr[var_id].value_ptr, var_length);
            *((uint32*) ((int8*) message_ptr + offset)) = htonl(
                    *((uint32*) ((int8*) message_ptr + offset)));
            break;
        default:

            printf("%s-%u:var length wrong\r\n", __FILE__, __LINE__);
            break;
        }
    }

    //Later should change
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(
            node_table.node_info_arr[peer_node_id].info.udp_node_info.ip_addr);
    sin.sin_port = htons(
            node_table.node_info_arr[peer_node_id].info.udp_node_info.port);
    if (message_length
            != sendto(
                    node_table.node_info_arr[local_node_id].info.udp_node_info.socket,
                    message_ptr, message_length, 0, (struct sockaddr*) &sin,
                    sizeof(sin)))
    {
        printf("%s-%u:send udp failed.\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }

    return command_info_ptr->true_next_command_id;
}

uint32 kernel_wait(void *arg_ptr)
{
    command_info_s *command_info_ptr;
    int32 ret_value;
    uint32 index;
    void *recv_message_ptr;
    struct timeval expected_time_val;
    struct timeval actual_time_val;
    struct timespec time_span;

    uint32 message_id;
    int8 *message_ptr;
    uint32 message_length;
    uint32 local_node_id;
    uint32 peer_node_id;

    uint32 var_num;
    uint32 offset;
    uint32 var_id;
    uint32 var_length;

    uint32 key_num;
    uint32 key_offset;
    uint32 key_size;
    uint32 key_type;
    uint32 var_id_of_key;

    uint32 compare_failed_num = 0;

    uint32 time_count; //ms unit

    if (NULL == arg_ptr)
    {
        printf("%s-%u:null error\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }

    command_info_ptr = (command_info_s*) arg_ptr;

    message_id = command_info_ptr->arg_info_arr[0].arg_value;
    time_count = command_info_ptr->arg_info_arr[1].arg_value;
    local_node_id = command_info_ptr->arg_info_arr[2].arg_value;
    peer_node_id = command_info_ptr->arg_info_arr[3].arg_value;

    if (!(node_table.node_info_arr[local_node_id].node_type
            == UDP_NODE_TYPE_RECV
            && node_table.node_info_arr[peer_node_id].node_type
                    == UDP_NODE_TYPE_RECV))
    {
        printf("%s-%u:Node type wrong\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }

    message_ptr = message_table.message_info_arr[message_id].message_ptr;
    message_length = message_table.message_info_arr[message_id].message_len;

    time_span.tv_sec = 0;
    time_span.tv_nsec = 1000000; //1ms
    gettimeofday(&expected_time_val, NULL);
    expected_time_val.tv_sec += time_count / 1000;
    expected_time_val.tv_usec += (time_count % 1000) * 1000;
    if (expected_time_val.tv_usec >= 1000000)
    {
        (expected_time_val.tv_sec)++;
        expected_time_val.tv_usec -= 1000000;
    }

    while (1)
    {
        gettimeofday(&actual_time_val, NULL);
        if (actual_time_val.tv_sec > expected_time_val.tv_sec
                || (actual_time_val.tv_sec == expected_time_val.tv_sec
                        && actual_time_val.tv_usec > expected_time_val.tv_usec))
        {

            printf("%s-%u:Time out\r\n", __FILE__, __LINE__);
            return SRV_INVALID;
        }

        if (srv_get_msg_num(TID_KERNEL) <= 0)
        {
            nanosleep(&time_span, NULL);
            continue;
        }
        if (SRV_OK != cspt_rcv(TID_KERNEL, &recv_message_ptr))
        {
            nanosleep(&time_span, NULL);
            continue;
        }

        //Compare message length
        if (message_length != ((cspt_header_s*) recv_message_ptr)->length)
        {
            printf("%s-%u:expect length %d but received length %d\r\n",
                    __FILE__, __LINE__, message_length,
                    ((cspt_header_s*) recv_message_ptr)->length);
            cspt_free(recv_message_ptr);
            continue;
        }

        //Compare node
        if (UDP_NODE_TYPE_RECV
                == node_table.node_info_arr[local_node_id].node_type)
        {
            if (((cspt_header_s*) recv_message_ptr)->header.udp_header.ip_addr
                    != node_table.node_info_arr[peer_node_id].info.udp_node_info.ip_addr
                    || ((cspt_header_s*) recv_message_ptr)->header.udp_header.port
                            != node_table.node_info_arr[peer_node_id].info.udp_node_info.port)
            {
                printf("%s-%u:peer address wrong\r\n", __FILE__, __LINE__);
                cspt_free(recv_message_ptr);
                continue;
            }

            if (((cspt_header_s*) recv_message_ptr)->header.udp_header.socket
                    != node_table.node_info_arr[local_node_id].info.udp_node_info.socket)
            {
                printf("%s-%u:local address wrong\r\n", __FILE__, __LINE__);
                cspt_free(recv_message_ptr);
                continue;
            }
        }

        //Compare key IE
        key_num = message_table.message_info_arr[message_id].key_num;
        for (index = 0; index < key_num; index++)
        {
            key_offset =
                    message_table.message_info_arr[message_id].key_info_arr[index].offset;
            key_size =
                    message_table.message_info_arr[message_id].key_info_arr[index].length;
            key_type =
                    message_table.message_info_arr[message_id].key_info_arr[index].key_type;
            if (KEY_TYPE_VALUE == key_type)
            {
                if (0
                        == memcmp(message_ptr + key_offset,
                                ((cspt_header_s*) recv_message_ptr)->message_ptr
                                        + key_offset, key_size))
                {
                    ret_value = SRV_OK;
                }
                else
                {
                    ret_value = SRV_ERR;
                }
            }
            else if (KEY_TYPE_VAR == key_type)
            {
                var_id_of_key =
                        message_table.message_info_arr[message_id].key_info_arr[index].var_id;
                ret_value = keyie_cmp(message_ptr + key_offset, key_size,
                        var_id_of_key);
            }
            else
            {

                printf("%s-%u:Key IE type wrong\r\n", __FILE__, __LINE__);
                return SRV_INVALID;
            }

            if (SRV_OK != ret_value)
            {
                compare_failed_num++;

                printf(
                        "%s-%u:IE which offset is %d length is %d compare failed\r\n",
                        __FILE__, __LINE__, key_offset, key_size);
            }
        }

        if (0 == compare_failed_num)
        {

            printf("%s-%u:All key IE compared succeed\r\n", __FILE__, __LINE__);
        }
        else
        {
            cspt_free(recv_message_ptr);
            return SRV_INVALID;
        }

        //Var evaluate
        var_num = message_table.message_info_arr[message_id].var_num;
        for (index = 0; index < var_num; index++)
        {
            offset =
                    message_table.message_info_arr[message_id].var_offset_arr[index].offset;
            var_id =
                    message_table.message_info_arr[message_id].var_offset_arr[index].var_id;
            var_length = var_table.var_info_arr[var_id].length;

            memcpy(var_table.var_info_arr[var_id].value_ptr,
                    ((cspt_header_s*) recv_message_ptr)->message_ptr + offset,
                    var_length);

            switch (var_length)
            {
            case VAR_TYPE_1_BYTE:
                break;
            case VAR_TYPE_2_BYTE:
                *((uint16*) (var_table.var_info_arr[var_id].value_ptr)) = ntohs(
                        *((uint16*) (var_table.var_info_arr[var_id].value_ptr)));
                break;
            case VAR_TYPE_4_BYTE:
                *((uint32*) (var_table.var_info_arr[var_id].value_ptr)) = ntohl(
                        *((uint32*) (var_table.var_info_arr[var_id].value_ptr)));
                break;
            default:
                break;
            }
        }

        memcpy(message_ptr, ((cspt_header_s*) recv_message_ptr)->message_ptr,
                message_length);
        cspt_free(recv_message_ptr);

        return command_info_ptr->true_next_command_id;
    }
}

int32 keyie_cmp(void *message_ptr, uint32 key_size, uint32 var_id)
{

    if (key_size != 1 && key_size != 2 && key_size != 4)
    {

        printf("%s-%u:Key IE length wrong\r\n", __FILE__, __LINE__);
        return SRV_ERR;
    }

    if (var_table.var_info_arr[var_id].length != 1
            && var_table.var_info_arr[var_id].length != 2
            && var_table.var_info_arr[var_id].length != 4)
    {

        printf("%s-%u:Var length wrong\r\n", __FILE__, __LINE__);
        return SRV_ERR;
    }

    if (key_size == 1 && var_table.var_info_arr[var_id].length == 1)
    {
        if (*((uint8*) message_ptr)
                == (*((uint8*) (var_table.var_info_arr[var_id].value_ptr))))
        {
            return SRV_OK;
        }
        else
        {
            return SRV_ERR;
        }
    }
    else if (key_size == 1 && var_table.var_info_arr[var_id].length == 2)
    {
        if (*((uint8*) message_ptr)
                == (*((uint16*) (var_table.var_info_arr[var_id].value_ptr))))
        {
            return SRV_OK;
        }
        else
        {
            return SRV_ERR;
        }
    }
    else if (key_size == 1 && var_table.var_info_arr[var_id].length == 4)
    {
        if (*((uint8*) message_ptr)
                == (*((uint32*) (var_table.var_info_arr[var_id].value_ptr))))
        {
            return SRV_OK;
        }
        else
        {
            return SRV_ERR;
        }
    }

    else if (key_size == 2 && var_table.var_info_arr[var_id].length == 1)
    {
        if (ntohs(*((uint16*) message_ptr))
                == (*((uint8*) (var_table.var_info_arr[var_id].value_ptr))))
        {
            return SRV_OK;
        }
        else
        {
            return SRV_ERR;
        }
    }
    else if (key_size == 2 && var_table.var_info_arr[var_id].length == 2)
    {
        if (ntohs(*((uint16*) message_ptr))
                == (*((uint16*) (var_table.var_info_arr[var_id].value_ptr))))
        {
            return SRV_OK;
        }
        else
        {
            return SRV_ERR;
        }
    }
    else if (key_size == 2 && var_table.var_info_arr[var_id].length == 4)
    {
        if (ntohs(*((uint16*) message_ptr))
                == (*((uint32*) (var_table.var_info_arr[var_id].value_ptr))))
        {
            return SRV_OK;
        }
        else
        {
            return SRV_ERR;
        }
    }
    else if (key_size == 4 && var_table.var_info_arr[var_id].length == 1)
    {
        if (ntohl(*((uint32*) message_ptr))
                == (*((uint8*) (var_table.var_info_arr[var_id].value_ptr))))
        {
            return SRV_OK;
        }
        else
        {
            return SRV_ERR;
        }
    }
    else if (key_size == 4 && var_table.var_info_arr[var_id].length == 2)
    {
        if (ntohl(*((uint32*) message_ptr))
                == (*((uint16*) (var_table.var_info_arr[var_id].value_ptr))))
        {
            return SRV_OK;
        }
        else
        {
            return SRV_ERR;
        }
    }
    else if (key_size == 4 && var_table.var_info_arr[var_id].length == 4)
    {
        if (ntohl(*((uint32*) message_ptr))
                == (*((uint32*) (var_table.var_info_arr[var_id].value_ptr))))
        {
            return SRV_OK;
        }
        else
        {
            return SRV_ERR;
        }
    }

    return SRV_ERR;
}

uint32 kernel_goto(void *arg_ptr)
{
    command_info_s *command_info_ptr;
    uint32 arg_value_1;
    uint32 arg_value_2;
    uint32 flag;
    uint32 for_loop_count;
    if (NULL == arg_ptr)
    {
        printf("%s-%u:null error", __FILE__, __LINE__);
        return SRV_INVALID;
    }

    command_info_ptr = (command_info_s*) arg_ptr;
    arg_value_1 = command_info_ptr->arg_info_arr[0].arg_value;
    arg_value_2 = command_info_ptr->arg_info_arr[1].arg_value;
    flag = command_info_ptr->arg_info_arr[2].arg_value;

    switch (flag)
    {
    case GOTO_FLAG_IF:
        if (command_info_ptr->arg_info_arr[0].arg_type == ARG_TYPE_VALUE
                && command_info_ptr->arg_info_arr[1].arg_type == ARG_TYPE_VALUE)
        {
            if (arg_value_1 == arg_value_2)
            {
                return command_info_ptr->true_next_command_id;
            }
            else
            {
                return command_info_ptr->false_next_command_id;
            }
        }
        else if (command_info_ptr->arg_info_arr[0].arg_type == ARG_TYPE_VARID
                && command_info_ptr->arg_info_arr[1].arg_type == ARG_TYPE_VALUE)
        {
            if (arg_value_1 >= var_table.var_num)
            {
                printf("%s-%u:Var ID wrong\r\n", __FILE__, __LINE__);
                return SRV_INVALID;
            }
            switch (var_table.var_info_arr[arg_value_1].length)
            {
            case VAR_TYPE_1_BYTE:
                if (*((uint8*) var_table.var_info_arr[arg_value_1].value_ptr)
                        == arg_value_2)
                {
                    return command_info_ptr->true_next_command_id;
                }
                else
                {
                    return command_info_ptr->false_next_command_id;
                }
            case VAR_TYPE_2_BYTE:
                if (*((uint16*) var_table.var_info_arr[arg_value_1].value_ptr)
                        == arg_value_2)
                {
                    return command_info_ptr->true_next_command_id;
                }
                else
                {
                    return command_info_ptr->false_next_command_id;
                }
            case VAR_TYPE_4_BYTE:
                if (*((uint32*) var_table.var_info_arr[arg_value_1].value_ptr)
                        == arg_value_2)
                {
                    return command_info_ptr->true_next_command_id;
                }
                else
                {
                    return command_info_ptr->false_next_command_id;
                }
            default:
                return SRV_INVALID;
            }
        }
        else if (command_info_ptr->arg_info_arr[0].arg_type == ARG_TYPE_VARID
                && command_info_ptr->arg_info_arr[1].arg_type == ARG_TYPE_VARID)
        {
            if (arg_value_1 >= var_table.var_num
                    || arg_value_2 >= var_table.var_num)
            {
                printf("%s-%u:Var ID wrong\r\n", __FILE__, __LINE__);
                return SRV_INVALID;
            }

            if (var_table.var_info_arr[arg_value_1].length
                    != var_table.var_info_arr[arg_value_2].length)
            {
                return command_info_ptr->false_next_command_id;
            }
            else
            {
                if (0
                        == memcmp(var_table.var_info_arr[arg_value_1].value_ptr,
                                var_table.var_info_arr[arg_value_2].value_ptr,
                                var_table.var_info_arr[arg_value_1].length))
                {
                    return command_info_ptr->true_next_command_id;
                }
                else
                {
                    return command_info_ptr->false_next_command_id;
                }
            }
        }
        else
        {
            return SRV_INVALID;
        }
    case GOTO_FLAG_FOR:
        for_loop_count = arg_value_1;
        for_loop_count--; //Java side's action similar to do{}while()
        if (for_loop_count > 0)
        {
            command_info_ptr->arg_info_arr[0].arg_value -= 1;
            return command_info_ptr->false_next_command_id;
        }
        else if (for_loop_count == 0)
        {
            command_info_ptr->arg_info_arr[0].arg_value =
                    command_info_ptr->arg_info_arr[1].arg_value;
            return command_info_ptr->true_next_command_id;
        }
        else
        {
            return SRV_INVALID;
        }
    default:
        return SRV_INVALID;
    }

    return SRV_INVALID;
}

uint32 kernel_sleep(void *arg_ptr)
{
    uint32 time_ms; //millisecond
    struct timespec timespac;
    struct timespec remain_timespec;
    command_info_s *command_info_ptr;

    if (NULL == arg_ptr)
    {
        printf("%s-%u:null error\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }

    command_info_ptr = (command_info_s*) arg_ptr;

    time_ms = command_info_ptr->arg_info_arr[0].arg_value;

    timespac.tv_sec = time_ms / 1000;
    timespac.tv_nsec = (time_ms % 1000) * 1000000;

    while (nanosleep(&timespac, &remain_timespec) == -1)
    {
        if (errno != EINTR)
        {
            break;
        }
        timespac = remain_timespec;
    }

    return command_info_ptr->true_next_command_id;
}

uint32 kernel_evaluate(void *arg_ptr)
{
    command_info_s *command_info_ptr;
    uint32 arg_value_1;
    uint32 arg_value_2;

    if (NULL == arg_ptr)
    {
        printf("%s-%u:null error\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }

    command_info_ptr = (command_info_s*) arg_ptr;

    arg_value_1 = command_info_ptr->arg_info_arr[0].arg_value;
    arg_value_2 = command_info_ptr->arg_info_arr[1].arg_value;

    if (command_info_ptr->arg_info_arr[0].arg_type == ARG_TYPE_VARID
            && command_info_ptr->arg_info_arr[1].arg_type == ARG_TYPE_VALUE)
    {
        if (arg_value_1 >= var_table.var_num)
        {
            printf("%s-%u:Var ID wrong\r\n", __FILE__, __LINE__);
            return SRV_INVALID;
        }

        switch (var_table.var_info_arr[arg_value_1].length)
        {
        case VAR_TYPE_1_BYTE:
            *((uint8*) (var_table.var_info_arr[arg_value_1].value_ptr)) =
                    arg_value_2;
            break;
        case VAR_TYPE_2_BYTE:
            *((uint16*) (var_table.var_info_arr[arg_value_1].value_ptr)) =
                    arg_value_2;
            break;
        case VAR_TYPE_4_BYTE:
            *((uint32*) (var_table.var_info_arr[arg_value_1].value_ptr)) =
                    arg_value_2;
            break;
        default:
            printf("%s-%u:Var length wrong\r\n", __FILE__, __LINE__);
            return SRV_INVALID;
        }

        return command_info_ptr->true_next_command_id;
    }
    else if (command_info_ptr->arg_info_arr[0].arg_type == ARG_TYPE_VARID
            && command_info_ptr->arg_info_arr[1].arg_type == ARG_TYPE_VARID)
    {
        if (arg_value_1 >= var_table.var_num
                || arg_value_2 >= var_table.var_num)
        {
            printf("%s-%u:Var ID wrong\r\n", __FILE__, __LINE__);
            return SRV_INVALID;
        }

        //May lose precision
        switch (var_table.var_info_arr[arg_value_1].length)
        {
        case VAR_TYPE_1_BYTE:
            switch (var_table.var_info_arr[arg_value_2].length)
            {
            case VAR_TYPE_1_BYTE:
                *((uint8*) (var_table.var_info_arr[arg_value_1].value_ptr)) =
                        *((uint8*) (var_table.var_info_arr[arg_value_2].value_ptr));
                break;
            case VAR_TYPE_2_BYTE:
                *((uint8*) (var_table.var_info_arr[arg_value_1].value_ptr)) =
                        *((uint16*) (var_table.var_info_arr[arg_value_2].value_ptr));
                break;
            case VAR_TYPE_4_BYTE:
                *((uint8*) (var_table.var_info_arr[arg_value_1].value_ptr)) =
                        *((uint32*) (var_table.var_info_arr[arg_value_2].value_ptr));
                break;
            default:
                printf("%s-%u:Var length wrong\r\n", __FILE__, __LINE__);
                return SRV_INVALID;
            }
            break;
        case VAR_TYPE_2_BYTE:
            switch (var_table.var_info_arr[arg_value_2].length)
            {
            case VAR_TYPE_1_BYTE:
                *((uint16*) (var_table.var_info_arr[arg_value_1].value_ptr)) =
                        *((uint8*) (var_table.var_info_arr[arg_value_2].value_ptr));
                break;
            case VAR_TYPE_2_BYTE:
                *((uint16*) (var_table.var_info_arr[arg_value_1].value_ptr)) =
                        *((uint16*) (var_table.var_info_arr[arg_value_2].value_ptr));
                break;
            case VAR_TYPE_4_BYTE:
                *((uint16*) (var_table.var_info_arr[arg_value_1].value_ptr)) =
                        *((uint32*) (var_table.var_info_arr[arg_value_2].value_ptr));
                break;
            default:
                printf("%s-%u:Var length wrong\r\n", __FILE__, __LINE__);
                return SRV_INVALID;
            }
            break;
        case VAR_TYPE_4_BYTE:
            switch (var_table.var_info_arr[arg_value_2].length)
            {
            case VAR_TYPE_1_BYTE:
                *((uint32*) (var_table.var_info_arr[arg_value_1].value_ptr)) =
                        *((uint8*) (var_table.var_info_arr[arg_value_2].value_ptr));
                break;
            case VAR_TYPE_2_BYTE:
                *((uint32*) (var_table.var_info_arr[arg_value_1].value_ptr)) =
                        *((uint16*) (var_table.var_info_arr[arg_value_2].value_ptr));
                break;
            case VAR_TYPE_4_BYTE:
                *((uint32*) (var_table.var_info_arr[arg_value_1].value_ptr)) =
                        *((uint32*) (var_table.var_info_arr[arg_value_2].value_ptr));
                break;
            default:
                printf("%s-%u:Var length wrong\r\n", __FILE__, __LINE__);
                return SRV_INVALID;
            }
            break;
        default:
            printf("%s-%u:Var length wrong\r\n", __FILE__, __LINE__);
            return SRV_INVALID;
        }

        return command_info_ptr->true_next_command_id;
    }
    else
    {
        printf("%s-%u:No such kind evaluate\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }
}

uint32 kernel_operation(void *arg_ptr)
{
    command_info_s *command_info_ptr;
    uint32 value1;
    uint32 value2;
    uint32 operation;
    uint32 result_var_id;
    uint32 result_value;

    if (NULL == arg_ptr)
    {
        printf("%s-%u:null error\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }
    command_info_ptr = (command_info_s*) arg_ptr;
    if (ARG_TYPE_VALUE == command_info_ptr->arg_info_arr[0].arg_type)
    {
        value1 = command_info_ptr->arg_info_arr[0].arg_value;
    }
    else if (ARG_TYPE_VARID == command_info_ptr->arg_info_arr[0].arg_type)
    {
        if (command_info_ptr->arg_info_arr[0].arg_value >= var_table.var_num)
        {
            printf("%s-%u:Var ID wrong\r\n", __FILE__, __LINE__);
            return SRV_INVALID;
        }
        switch (var_table.var_info_arr[command_info_ptr->arg_info_arr[0].arg_value].length)
        {
        case VAR_TYPE_1_BYTE:
            value1 =
                    *((uint8*) (var_table.var_info_arr[command_info_ptr->arg_info_arr[0].arg_value].value_ptr));
            break;
        case VAR_TYPE_2_BYTE:
            value1 =
                    *((uint16*) (var_table.var_info_arr[command_info_ptr->arg_info_arr[0].arg_value].value_ptr));
            break;
        case VAR_TYPE_4_BYTE:
            value1 =
                    *((uint32*) (var_table.var_info_arr[command_info_ptr->arg_info_arr[0].arg_value].value_ptr));
            break;
        default:
            printf("%s-%u:Var length wrong\r\n", __FILE__, __LINE__);
            return SRV_INVALID;
        }
    }
    else
    {
        printf("%s-%u:Arg type wrong\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }

    if (ARG_TYPE_VALUE == command_info_ptr->arg_info_arr[1].arg_type)
    {
        value2 = command_info_ptr->arg_info_arr[1].arg_value;
    }
    else if (ARG_TYPE_VARID == command_info_ptr->arg_info_arr[1].arg_type)
    {
        if (command_info_ptr->arg_info_arr[1].arg_value >= var_table.var_num)
        {
            printf("%s-%u:Var ID wrong\r\n", __FILE__, __LINE__);
            return SRV_INVALID;
        }
        switch (var_table.var_info_arr[command_info_ptr->arg_info_arr[1].arg_value].length)
        {
        case VAR_TYPE_1_BYTE:
            value2 =
                    *((uint8*) (var_table.var_info_arr[command_info_ptr->arg_info_arr[1].arg_value].value_ptr));
            break;
        case VAR_TYPE_2_BYTE:
            value2 =
                    *((uint16*) (var_table.var_info_arr[command_info_ptr->arg_info_arr[1].arg_value].value_ptr));
            break;
        case VAR_TYPE_4_BYTE:
            value2 =
                    *((uint32*) (var_table.var_info_arr[command_info_ptr->arg_info_arr[1].arg_value].value_ptr));
            break;
        default:
            printf("%s-%u:Var length wrong\r\n", __FILE__, __LINE__);
            return SRV_INVALID;
        }
    }
    else
    {
        printf("%s-%u:Arg type wrong\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }

    result_var_id = command_info_ptr->arg_info_arr[2].arg_value;
    if (result_var_id >= var_table.var_num)
    {
        printf("%s-%u:Var ID wrong\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }

    operation = command_info_ptr->arg_info_arr[3].arg_value;
    switch (operation)
    {
    case OPER_ADDITION:
        result_value = value1 + value2;
        break;
    case OPER_SUBTRACT:
        result_value = value1 - value2;
        break;
    case OPER_MULTIPLY:
        result_value = value1 * value2;
        break;
    case OPER_DIVIDE:
        if (0 == value2)
        {
            printf("%s-%u:Divisor should not be zero\r\n", __FILE__, __LINE__);
            return SRV_INVALID;
        }
        result_value = value1 / value2;
        break;
    case OPER_MOD:
        if (0 == value2)
        {
            printf("%s-%u:Divisor should not be zero\r\n", __FILE__, __LINE__);
            return SRV_INVALID;
        }
        result_value = value1 % value2;
        break;
    case OPER_MORETHAN:
        result_value = (value1 > value2) ? 1 : 0;
        break;
    case OPER_LESSTHAN:
        result_value = (value1 < value2) ? 1 : 0;
        break;
    default:
        printf("%s-%u:Operation type wrong\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }

    switch (var_table.var_info_arr[result_var_id].length)
    {
    case VAR_TYPE_1_BYTE:
        *((uint8*) (var_table.var_info_arr[result_var_id].value_ptr)) =
                result_value;
        break;
    case VAR_TYPE_2_BYTE:
        *((uint16*) (var_table.var_info_arr[result_var_id].value_ptr)) =
                result_value;
        break;
    case VAR_TYPE_4_BYTE:
        *((uint32*) (var_table.var_info_arr[result_var_id].value_ptr)) =
                result_value;
        break;
    default:
        printf("%s-%u:Var length wrong\r\n", __FILE__, __LINE__);
        return SRV_INVALID;
    }

    return command_info_ptr->true_next_command_id;
}
