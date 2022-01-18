# devices/consumers.py
import json
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
            self.send(text_data=json.dumps({
                'rf_outlet_status': {outlet.id: outlet.status}
            }))
        except ObjectDoesNotExist as e:
            return
