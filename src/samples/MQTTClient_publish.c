/*******************************************************************************
 * Copyright (c) 2012, 2020 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *   https://www.eclipse.org/legal/epl-2.0/
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"

//初始参数
#define ADDRESS "tcp://127.0.0.1:1883"
#define CLIENTID "ExampleClientPub"
#define TOPIC "MQTT Examples"
//携带的消息
#define PAYLOAD "Hello World!"
//服务质量
#define QOS 1
#define TIMEOUT 10000L

volatile MQTTClient_deliveryToken deliveredtoken;

//消息发送成功回调
void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

//消息接收回调
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char *)message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

//连接丢失回调
void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char *argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    //初始化发布的消息
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    //创建客户端
    if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID,
                                MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    //设置  连接丢失,消息送达, 发送成功 ,回调
    if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to set callbacks, return code %d\n", rc);
        rc = EXIT_FAILURE;
        // goto destroy_exit;
    }

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    //消息内容
    pubmsg.payload = PAYLOAD;
    //消息长度
    pubmsg.payloadlen = (int)strlen(PAYLOAD);
    //服务质量
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    int ch;
    do
    {
        ch = getchar();

        // 1Kb= 1024 字节
        // 1mb = 1024kb
        //最大理论限制   268435455字节   256MB
        // 1 << 28-1
        //KB = 1<<10
        //MB = 1<<20
        // GB = 1<<30
        // TB = 1<<40
        // PB = 1<<50

        if (ch != '\n')
        {

            char *m = NULL;
            m = (char *)malloc(  (268435455) * sizeof(char));
            for (int i = 0; i < (268435455); i++)
            {
                m[i] = ch;
            }
            // char m[(1 << 20) * 4];
            // for (int i = 0; i < (1 << 20) * 4; i++)
            // {
            //     m[i] = ch;
            // }

            pubmsg.payload = m;
            pubmsg.payloadlen = (int)strlen(m);
            //发布消息
            if ((rc = MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
            {
                printf("Failed to publish message, return code %d\n", rc);
                exit(EXIT_FAILURE);
            }
        }

    } while (ch != 'Q' && ch != 'q');

    printf("Waiting for up to %d seconds for publication of %s\n"
           "on topic %s for client with ClientID: %s\n",
           (int)(TIMEOUT / 1000), PAYLOAD, TOPIC, CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    printf("Message with delivery token %d delivered\n", token);

    //断开与  broker的连接
    if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
        printf("Failed to disconnect, return code %d\n", rc);

    //销毁客户端
    MQTTClient_destroy(&client);
    return rc;
}
