# devices/consumers.py
import json

from asgiref.sync import async_to_sync
from channels.consumer import SyncConsumer
from channels.generic.websocket import WebsocketConsumer
from django.core.exceptions import ObjectDoesNotExist
from devices.models import RFOutlet


class DevicesConsumer(WebsocketConsumer):
    def connect(self):
        async_to_sync(self.channel_layer.group_add)(
            "testmqtt",
            self.channel_name
        )
        self.accept()

    def disconnect(self, close_code):
        async_to_sync(self.channel_layer.group_discard)(
            "testmqtt",
            self.channel_name
        )
        pass

    def receive(self, text_data, **kwargs):
        print(text_data)
        try:
            text_data_json = json.loads(text_data)
            message = text_data_json['rf_outlet_toggle']
            outlet = RFOutlet.objects.get(id=int(message))
            outlet.toggle()
            self.send(text_data=outlet.get_json_state())
        except ObjectDoesNotExist as e:
            # print(e.what())
            return
        except TypeError as e:
            return

    def handle_mqtt(self, text_data, **kwargs):
        print("handle mqtt")
        try:
            message = text_data['message']["rf_outlet_toggle"]
            print(message)
            outlet = RFOutlet.objects.get(id=int(message))
            outlet.toggle()
            self.send(text_data=outlet.get_json_state())
        except ObjectDoesNotExist as e:
            # print(e.what())
            return
        except TypeError as e:
            return
