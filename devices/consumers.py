# devices/consumers.py
import json
import string

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
                if 'rgb_light_color_update' in text_data_json:
                    id = -1
                    hexcolor = "000000"
                    if 'id' in text_data_json['rgb_light_color_update']:
                        id = text_data_json['rgb_light_color_update']['id']
                    if 'hexcolor' in text_data_json['rgb_light_color_update']:
                        hexcolor = text_data_json['rgb_light_color_update']['hexcolor']
                    light = RGBLight.objects.get(unique_id=id)
                    light.red = int(hexcolor[1:3],16)
                    light.green = int(hexcolor[3:5],16)
                    light.blue = int(hexcolor[5:7],16)
                    light.set_color_mqtt()

                if 'rf_outlet_toggle' in text_data_json:
                    message = text_data_json['rf_outlet_toggle']
                    outlet = RF433Outlet.objects.get(id=int(message))
                    outlet.toggle()
                    self.send(text_data=outlet.get_json_state())
                elif 'rgb_light_toggle' in text_data_json:
                    message = text_data_json['rgb_light_toggle']
                    light = RGBLight.objects.get(id=int(message))
                    light.toggle()
                elif 'open_garage_door' in text_data_json:
                    topic = "esp_lora/103/open-garage"
                    channel_layer = get_channel_layer()
                    async_to_sync(channel_layer.send)('mqtt.pub', {  # also needs to be mqtt.pub
                        'type': 'mqtt.pub',  # necessary to be mqtt.pub
                        'text': {
                            'topic': topic,
                            'payload': json.dumps({})
                        }
                    })
                elif 'close_garage_door' in text_data_json:
                    topic = "esp_lora/103/close-garage"
                    channel_layer = get_channel_layer()
                    async_to_sync(channel_layer.send)('mqtt.pub', {  # also needs to be mqtt.pub
                        'type': 'mqtt.pub',  # necessary to be mqtt.pub
                        'text': {
                            'topic': topic,
                            'payload': json.dumps({})
                        }
                    })
                elif 'query_garage_door' in text_data_json:
                    topic = "esp_lora/103/query-garage"
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

    def mqtt_garage_update(self, text_data, **kwargs):
        try:
            message = "Unknown"
            if 'message' in text_data:
                # filter out any non printable from the mqtt message
                incoming_msg = ''.join(c for c in text_data['message'] if c.isprintable())
                if incoming_msg == "Open":
                    message = "Open"
                elif incoming_msg == "Closed":
                    message = "Closed"
                elif incoming_msg == "OK:Close":
                    message = "Success Initiating Door Close"
                elif incoming_msg == "Fail:Close":
                    message = "Fail Initiating Door Close"
                elif incoming_msg == "OK:Open":
                    message = "Success Initiating Door Open"
                elif incoming_msg == "Fail:Open":
                    message = "Fail Initiating Door Open"
                else:
                    message = incoming_msg
            self.send(text_data=json.dumps({'mqtt_garage_update': {'status': message}}))
        except ObjectDoesNotExist as e:
            # print(e.what())
            print("Exception ocurred in mqtt_garage_update in consumers.py")
            return
        except TypeError as e:
            print("TypeError mqtt_garage_update")

            return
