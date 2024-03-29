#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "MQTTClient.h"

#define ADDRESS     "ssl://127.0.0.1:8883"
#define USERNAME    "emqx"
#define PASSWORD    "public"
#define CLIENTID    "c-client"
#define QOS         0
#define TOPIC       "emqx/c-test"
#define TIMEOUT     10000L
#define CLIENTCERT  "/etc/emqx/certs/client-cert.pem"
#define CACERT      "/etc/emqx/certs/cacert.pem"
#define PRIVATEKEY  "/etc/emqx/certs/client-key.pem"

void publish(MQTTClient client, char *topic, char *payload) {
    MQTTClient_message message = MQTTClient_message_initializer;
    message.payload = payload;
    message.payloadlen = strlen(payload);
    message.qos = QOS;
    message.retained = 0;
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(client, topic, &message, &token);
    MQTTClient_waitForCompletion(client, token, TIMEOUT);
    printf("Send `%s` to topic `%s` \n", payload, TOPIC);
}

int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message) { char *payload = message->payload;
    printf("Received `%s` from `%s` topic \n", payload, topicName);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

int main(int argc, char *argv[]) {
    int rc;
    MQTTClient client;

	MQTTClient_createOptions createOpts = MQTTClient_createOptions_initializer;
    MQTTClient_create(&client, ADDRESS, CLIENTID, 0, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.username = USERNAME;
    conn_opts.password = PASSWORD;

	MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
	/*
	 * Set the value of 'verify' to 0, which means
	 * that the domain should not be checked.
	 * Otherwise, if the domain does not match,
	 * an error will occur.
	 */
	ssl_opts.verify = 0;
	ssl_opts.trustStore = CACERT;
	/*
	 * Two-Way SSL Authentiacation needed.
	 * If you want to use One-Way SSL Authentication,
	 * please comment out keyStore and privateKey.
	 */
	ssl_opts.privateKey = PRIVATEKEY;
	ssl_opts.keyStore = CLIENTCERT;
	/*
	 * Notice: In higher versions of openssl
	 * the SSL/TLS version is depended on openssl itself.
	 * (Paho will use TLS_Client_method(), more details please see openssl docs)
	 */
	conn_opts.ssl = &ssl_opts;

    MQTTClient_setCallbacks(client, NULL, NULL, on_message, NULL);
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    } else {
        printf("Connected to MQTT Broker!\n");
    }
    // subscribe topic
    MQTTClient_subscribe(client, TOPIC, QOS);
    char payload[16];
    for (int i = 0; i < 100; i += 1) {
        // publish message to broker
        snprintf(payload, 16, "message-%d", i);
        publish(client, TOPIC, payload);
        sleep(1);
    }
    MQTTClient_disconnect(client, TIMEOUT);
    MQTTClient_destroy(&client);
    return rc;
}
