import datetime
import django
import os
from asgiref.sync import async_to_sync
from channels.consumer import SyncConsumer
from channels.layers import get_channel_layer
import paho.mqtt.client as paho
os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'hardleeauto.settings')

django.setup()
from devices.models import RFOutlet, RGBLight


class MqttConsumer(SyncConsumer):

    def mqtt_sub(self, event):
        topic = event['text']['topic']
        payload = event['text']['payload']
        # do something with topic and payload
        try:
            topic_split = topic.split("/")
            if len(topic_split) != 3:
                print("Invalid topic! " + topic)
                return
            module_type = topic_split[0]
            dev_id = int(topic_split[1])
            info_type = topic_split[2]
            print("module type=" + module_type)
            if module_type == 'rgb-light' and info_type == "status":
                light = RGBLight.objects.get(unique_id=dev_id)
                if "red" in payload:
                    light.red = int(payload["red"])
                if "green" in payload:
                    light.green = int(payload["green"])
                if "blue" in payload:
                    light.blue = int(payload["blue"])
                if "is_on" in payload:
                    light.is_on = int(payload["is_on"])
                light.save()
                channel_layer = get_channel_layer()
                async_to_sync(channel_layer.group_send)(
                    'device_updates',
                    {
                        'type': 'mqtt_rgb_light_update',
                        'message': light.get_json_state()
                    }
                )
            elif module_type == 'rf-outlet':
                pass
        except:
            pass

        # channel_layer = get_channel_layer()
        # async_to_sync(channel_layer.group_send)(
        #     'testmqtt',
        #     {
        #         'type': 'handle_mqtt',
        #         'message': {"rf_outlet_toggle": 1}
        #     }
        # )
        print("sub topic: {0}, payload: {1}".format(topic, payload))

    def mqtt_pub(self, event):
        topic = event['text']['topic']
        payload = event['text']['payload']
        # do something with topic and payload

        print("pub topic: {0}, payload: {1}".format(topic, payload))
