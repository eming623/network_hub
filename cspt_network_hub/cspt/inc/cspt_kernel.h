#ifndef CSPT_KERNEL_H_
#define CSPT_KERNEL_H_

#include "srv_common.h"

#define MAX_UDP_SEND_NUM       300
#define MAX_UDP_RECV_NUM       300
#define MAX_NODE_NUM           1000
#define MAX_VAR_NUM            200
#define MAX_IMAGE_NUM          1000
#define MAX_VAR_NUM_PER_MSG    30
#define MAX_KEYIE_NUM          50
#define MAX_ARG_NUM            10   //for command table use
#define MAX_CMD_NUM            1500

typedef enum
{
    KERNEL_MSG_PRECOMPILE = 0x0A << 24, KERNEL_MSG_RUN = 0x0B << 24
} kernel_message_type_e;

typedef enum
{
    PRECOMPILE_MSG_TOPO = KERNEL_MSG_PRECOMPILE,
    PRECOMPILE_MSG_VAR,
    PRECOMPILE_MSG_IMAGE,
    PRECOMPILE_MSG_CONTENT,
    PRECOMPILE_MSG_COMMAND
} precompile_message_type_e;

typedef enum
{
    VAR_TYPE_1_BYTE = 1, VAR_TYPE_2_BYTE = 2, VAR_TYPE_4_BYTE = 4
} var_type_e;

typedef enum
{
    UDP_NODE_FLAG_LOCAL = 1, UDP_NODE_FLAG_PEER
} udp_node_flag_e;

typedef struct
{
    uint32 ip_addr;
    uint16 port;
    uint16 reserved;
    udp_node_flag_e flag;
} config_udp_info_s;

typedef struct
{
    uint32 udp_send_node_num;
    config_udp_info_s config_udp_send_info_arr[MAX_UDP_SEND_NUM];
    uint32 udp_receive_node_num;
    config_udp_info_s config_udp_recv_info_arr[MAX_UDP_RECV_NUM];
} config_node_info;

typedef struct
{
    uint32 ip_addr;
    uint16 port;
    uint16 reserved;
    udp_node_flag_e flag;
    int32 socket;
} udp_node_info_s;

typedef enum
{
    UDP_NODE_TYPE_SEND = 1, UDP_NODE_TYPE_RECV,
} node_type_e;

typedef struct
{
    uint32 node_id;
    node_type_e node_type; //1 means udp send,2 means udp receive
    union
    {
        udp_node_info_s udp_node_info;
    } info;
} node_info;

typedef struct
{
    uint32 node_num;
    node_info node_info_arr[MAX_NODE_NUM];
} node_table_s;

typedef struct
{
    uint32 var_id;
    uint32 length;
    void *value_ptr;
} var_info_s;

typedef struct
{
    uint32 var_num;
    var_info_s var_info_arr[MAX_VAR_NUM];
} variable_table_s;

typedef enum
{
    KEY_TYPE_VALUE, KEY_TYPE_VAR
} key_type_e;

typedef struct
{
    uint32 offset;
    uint32 length;
    uint32 key_type;
    uint32 var_id; //if key Type is a variable,the var_id is needed
} key_info_s;

typedef struct
{
    uint32 offset;
    uint32 var_id;
} var_offset_s;

typedef struct
{
    uint32 message_id; //equal to array subscript
    void *message_ptr;
    uint32 message_len;
    uint32 var_num;
    var_offset_s var_offset_arr[MAX_VAR_NUM_PER_MSG];
    uint32 key_num;
    key_info_s key_info_arr[MAX_KEYIE_NUM];
} message_info_s;

typedef struct
{
    uint32 message_num;
    message_info_s message_info_arr[MAX_IMAGE_NUM];
} message_table_s;

typedef enum
{
    ARG_TYPE_VALUE, ARG_TYPE_VARID
} command_arg_type_e;

typedef struct
{
    command_arg_type_e arg_type;
    uint32 arg_value;
} command_arg_info_s;

typedef uint32 (*cmd_func)(void *);

typedef struct
{
    uint32 command_id;
    cmd_func command_func;
    uint32 arg_num;
    command_arg_info_s arg_info_arr[MAX_ARG_NUM];
    uint32 true_next_command_id;
    uint32 false_next_command_id;
} command_info_s;

typedef struct
{
    uint32 command_num;
    command_info_s command_info_arr[MAX_CMD_NUM];
} command_table_s;

typedef struct
{
    uint32 command_id;
} case_run_s;

typedef enum
{
    GOTO_FLAG_IF,
    GOTO_FLAG_FOR
} goto_flag_e;

typedef enum
{
    OPER_ADDITION,
    OPER_SUBTRACT,
    OPER_MULTIPLY,
    OPER_DIVIDE,
    OPER_MOD,
    OPER_MORETHAN,
    OPER_LESSTHAN
} operation_e;
//*******************************************************************//

void* kernel_entry(void *arg);

int32 precompile(void *pMsg, uint32 ulMsgType);

int32 nodetable_cleanup();

int32 create_bind_socket(uint32 ulIp, uint16 usPort);

uint32 find_node_by_ip_port(uint32 ulStart, uint32 ulEnd, uint32 ulIP,
        uint16 usPort);

int32 nodetable_create(void *pMsg);

int32 vartable_create(void *pMsg);

int32 imagetable_create(void *pMsg);

int32 add_msg(void *pMsg);

int32 cmdtable_create(void *pMsg);

int32 case_run(uint32 ulCmdID);

uint32 kernel_send(void *pArg);

uint32 kernel_wait(void *pArg);

int32 keyie_cmp(void *pMsg, uint32 ulKeyIELen, uint32 ulVarID);

uint32 kernel_goto(void *pArg);

uint32 kernel_sleep(void *pArg);

uint32 kernel_evaluate(void *pArg);

uint32 kernel_operation(void *pArg);

#endif /* CSPT_KERNEL_H_ */
