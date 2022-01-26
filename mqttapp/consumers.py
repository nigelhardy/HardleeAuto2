import datetime
from asgiref.sync import async_to_sync
from channels.consumer import SyncConsumer
from channels.layers import get_channel_layer
import paho.mqtt.client as paho


class MqttConsumer(SyncConsumer):

    def mqtt_sub(self, event):
        topic = event['text']['topic']
        payload = event['text']['payload']
        # do something with topic and payload
        channel_layer = get_channel_layer()
        async_to_sync(channel_layer.group_send)(
            'testmqtt',
            {
                'type': 'handle_mqtt',
                'message': {"rf_outlet_toggle": 1}
            }
        )
        # Temp, just to prove you can send to MQTT as well!
        async_to_sync(channel_layer.send)('mqtt.pub', {  # also needs to be mqtt.pub
            'type': 'mqtt.pub',  # necessary to be mqtt.pub
            'text': {
                'topic': 'testmq',
                'payload': "bigdawg"
            }
        })
        print("sub topic: {0}, payload: {1}".format(topic, payload))

    def mqtt_pub(self, event):
        topic = event['text']['topic']
        payload = event['text']['payload']
        # do something with topic and payload

        print("pub topic: {0}, payload: {1}".format(topic, payload))
