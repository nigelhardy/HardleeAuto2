# devices/consumers.py
import json
import string
import logging

from asgiref.sync import async_to_sync
from channels.consumer import SyncConsumer
from channels.generic.websocket import WebsocketConsumer
from django.core.exceptions import ObjectDoesNotExist
from devices.models import RF433Outlet, RGBLight, ShellyBulb, Garage
from channels.layers import get_channel_layer

logger = logging.getLogger(__name__)

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
            logger.info(text_data)
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

                if 'rf_outlet_command' in text_data_json:
                    message = text_data_json['rf_outlet_command']
                    logger.info(message)
                    outlet = RF433Outlet.objects.get(id=int(message['id']))
                    if 'command' in message:
                        if message['command'] == "on":
                            outlet.set_on_off(True)
                        elif message['command'] == "off":
                            outlet.set_on_off(False)
                    channel_layer = get_channel_layer()
                    async_to_sync(channel_layer.group_send)(
                        'device_updates', {
                            'type': 'mqtt_rgb_light_update',
                            'message': outlet.get_json_state()
                    })
                if 'bulb_toggle' in text_data_json:
                    message = text_data_json['bulb_toggle']
                    bulb = ShellyBulb.objects.get(id=int(message))
                    bulb.toggle()
                    channel_layer = get_channel_layer()
                    async_to_sync(channel_layer.group_send)(
                        'device_updates', {
                            'type': 'mqtt_rgb_light_update',
                            'message': bulb.get_json_state()
                    })
                elif 'rgb_light_toggle' in text_data_json:
                    message = text_data_json['rgb_light_toggle']
                    light = RGBLight.objects.get(id=int(message))
                    light.toggle()
                elif 'open_garage_door' in text_data_json:
                    topic = "lora/103/garage-cmd"
                    channel_layer = get_channel_layer()
                    async_to_sync(channel_layer.send)('mqtt.pub', {  # also needs to be mqtt.pub
                        'type': 'mqtt.pub',  # necessary to be mqtt.pub
                        'text': {
                            'topic': topic,
                            'payload': json.dumps({"cmd":"open"})
                        }
                    })
                elif 'close_garage_door' in text_data_json:
                    topic = "lora/103/garage-cmd"
                    channel_layer = get_channel_layer()
                    async_to_sync(channel_layer.send)('mqtt.pub', {  # also needs to be mqtt.pub
                        'type': 'mqtt.pub',  # necessary to be mqtt.pub
                        'text': {
                            'topic': topic,
                            'payload': json.dumps({"cmd":"close"})
                        }
                    })
                elif 'query_garage_door' in text_data_json:
                    topic = "lora/103/garage-cmd"
                    channel_layer = get_channel_layer()
                    async_to_sync(channel_layer.send)('mqtt.pub', {  # also needs to be mqtt.pub
                        'type': 'mqtt.pub',  # necessary to be mqtt.pub
                        'text': {
                            'topic': topic,
                            'payload': json.dumps({"cmd":"query"})
                        }
                    })
            except ObjectDoesNotExist as e:
                logger.error("OBJECT DOESNT EXIST")
                # logger.error(e.what())
                return
            except TypeError as e:
                return

    def mqtt_rgb_light_update(self, text_data, **kwargs):
        try:
            self.send(text_data=text_data['message'])
        except ObjectDoesNotExist as e:
            logger.error(e.what())
            return
        except TypeError as e:
            return

    def mqtt_garage_update(self, text_data, **kwargs):
        try:
            message = "Unknown"
            if 'message' in text_data:
                # filter out any non printable from the mqtt message
                incoming_msg = ''.join(c for c in text_data['message'] if c.isprintable())
                garages = Garage.objects.all()
                '''
                KEY for comms
                oo = open
                cc = closed
                oc = open and attempting to close
                co = closed and attempting to open
                cf = close failed
                of = open failed
                tf = general unknown timeout from open or close attempt
                uo = unexpectedly opened
                '''
                garage_open = True
                if incoming_msg == 'oo' or incoming_msg == 'oc' or incoming_msg == 'uo':
                    garage_open = True # redundant, but here for future potential
                elif incoming_msg == 'cc' or incoming_msg == 'co':
                    garage_open = False
                message = incoming_msg
                if incoming_msg == 'oo':
                    message = "Garage is Open"
                elif incoming_msg == 'cc':
                    message = "Garage is Closed"
                elif incoming_msg == 'oc':
                    message = "Garage is Attempting to Close"
                elif incoming_msg == 'co':
                    message = "Garage is Attempting to Open"
                elif incoming_msg == 'cf':
                    message = "Garage's Attempt to Close Failed"
                elif incoming_msg == 'of':
                    message = "Garage's Attempt to Open Failed"
                elif incoming_msg == 'tf':
                    message = "Garage Timeout Fail"
                elif incoming_msg == 'uo':
                    message = "Garage Unexpectedly Opened"
                if len(garages) == 1: # for now limit to one garage
                    garages[0].is_open = garage_open
                    garages[0].current_state = message
                    garages[0].save()
            self.send(text_data=json.dumps({'mqtt_garage_update': {'status': message, 'is_open': garage_open}}))
        except ObjectDoesNotExist as e:
            # logger.error(e.what())
            logger.error("Exception ocurred in mqtt_garage_update in consumers.py")
            return
        except TypeError as e:
            logger.error("TypeError mqtt_garage_update")

            return
