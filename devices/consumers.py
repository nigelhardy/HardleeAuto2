# devices/consumers.py
import json

from channels.consumer import SyncConsumer
from channels.generic.websocket import WebsocketConsumer
from django.core.exceptions import ObjectDoesNotExist
from devices.models import RFOutlet


class DevicesConsumer(WebsocketConsumer):
    def connect(self):
        self.accept()

    def disconnect(self, close_code):
        pass

    def receive(self, text_data, **kwargs):
        text_data_json = json.loads(text_data)
        message = text_data_json['rf_outlet_toggle']
        try:
            outlet = RFOutlet.objects.get(id=int(message))
            outlet.toggle()
            self.send(text_data=outlet.get_json_state())
        except ObjectDoesNotExist as e:
            # print(e.what())
            return
