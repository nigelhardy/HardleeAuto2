# devices/consumers.py
import json

from asgiref.sync import async_to_sync
from channels.consumer import SyncConsumer
from channels.generic.websocket import WebsocketConsumer
from django.core.exceptions import ObjectDoesNotExist
from devices.models import RF433Outlet, RGBLight
from channels.layers import get_channel_layer


class DevicesConsumer(WebsocketConsumer):
    def connect(self):
        username = self.scope["user"]
        if username.is_authenticated:
            async_to_sync(self.channel_layer.group_add)(
                "device_updates",
                self.channel_name
            )
            self.accept()

    def disconnect(self, close_code):
        async_to_sync(self.channel_layer.group_discard)(
            "device_updates",
            self.channel_name
        )
        pass

    def receive(self, text_data, **kwargs):
        username = self.scope["user"]
        if username.is_authenticated:
            print(text_data)
            try:
                text_data_json = json.loads(text_data)
                if 'rf_outlet_toggle' in text_data_json:
                    message = text_data_json['rf_outlet_toggle']
                    outlet = RF433Outlet.objects.get(id=int(message))
                    outlet.toggle()
                    self.send(text_data=outlet.get_json_state())
                elif 'rgb_light_toggle' in text_data_json:
                    message = text_data_json['rgb_light_toggle']
                    light = RGBLight.objects.get(id=int(message))
                    light.toggle()
                elif 'activate_garage_door':
                    topic = "relay/101/garage-door"
                    channel_layer = get_channel_layer()
                    async_to_sync(channel_layer.send)('mqtt.pub', {  # also needs to be mqtt.pub
                        'type': 'mqtt.pub',  # necessary to be mqtt.pub
                        'text': {
                            'topic': topic,
                            'payload': json.dumps({})
                        }
                    })
            except ObjectDoesNotExist as e:
                print("OBJECT DOESNT EXIST")
                # print(e.what())
                return
            except TypeError as e:
                return

    def mqtt_rgb_light_update(self, text_data, **kwargs):
        try:
            self.send(text_data=text_data['message'])
        except ObjectDoesNotExist as e:
            # print(e.what())
            return
        except TypeError as e:
            return
