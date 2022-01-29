# devices/consumers.py
import json

from asgiref.sync import async_to_sync
from channels.consumer import SyncConsumer
from channels.generic.websocket import WebsocketConsumer
from django.core.exceptions import ObjectDoesNotExist
from devices.models import RFOutlet, RGBLight


class DevicesConsumer(WebsocketConsumer):
    def connect(self):
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
        print(text_data)
        try:
            text_data_json = json.loads(text_data)
            if 'rf_outlet_toggle' in text_data_json:
                message = text_data_json['rf_outlet_toggle']
                outlet = RFOutlet.objects.get(id=int(message))
                outlet.toggle()
                self.send(text_data=outlet.get_json_state())
            elif 'rgb_light_toggle' in text_data_json:
                message = text_data_json['rgb_light_toggle']
                light = RGBLight.objects.get(id=int(message))
                light.toggle()
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
