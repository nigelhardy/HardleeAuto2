import datetime
import django
import os
import logging
from asgiref.sync import async_to_sync
from channels.consumer import SyncConsumer
from channels.layers import get_channel_layer
import paho.mqtt.client as paho
os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'hardleeauto.settings')

django.setup()
logger = logging.getLogger(__name__)
from devices.models import RF433Outlet, RGBLight, ShellyBulb, RF_OnOffPair


class MqttConsumer(SyncConsumer):

    def mqtt_sub(self, event):
        topic = event['text']['topic']
        payload = event['text']['payload']
        # do something with topic and payload
        try:
            topic_split = topic.split("/")
            if len(topic_split) != 3:
                logger.warning("Invalid topic! " + topic)
                return
            module_type = topic_split[0]
            dev_id = int(topic_split[1])
            info_type = topic_split[2]
            if module_type == 'rgbw-strip' and info_type == "status":
                logger.info("sub topic: {0}, payload: {1}".format(topic, payload))
                light = RGBLight.objects.get(unique_id=dev_id)
                if "red" in payload:
                    light.red = int(payload["red"])
                if "green" in payload:
                    light.green = int(payload["green"])
                if "blue" in payload:
                    light.blue = int(payload["blue"])
                logger.info("devid=" + str(dev_id))
                if "is_on" in payload:
                    light.is_on = payload["is_on"]
                light.save()
                channel_layer = get_channel_layer()
                async_to_sync(channel_layer.group_send)(
                    'device_updates',
                    {
                        'type': 'mqtt_rgb_light_update',
                        'message': light.get_json_state()
                    }
                )
            elif module_type == 'rf433rx' and info_type == "recv-payload":
                logger.info("sub topic: {0}, payload: {1}".format(topic, payload))
                logger.info("payload int = " + str(int(payload)))
                isOnButton = False
                rfPairsOn = RF_OnOffPair.objects.filter(onValue=int(payload))
                rfPairsOff = RF_OnOffPair.objects.filter(offValue=int(payload))
                for rfPairOn in rfPairsOn:
                    bulb = None
                    if rfPairOn:
                        outlets = RF433Outlet.objects.filter(recv_triggers=rfPairOn)
                        bulbs = ShellyBulb.objects.filter(recv_triggers=rfPairOn)
                        isOnButton = True
                        for bulb in bulbs:
                            bulb.set_light_on_off(isOnButton)
                        for outlet in outlets:
                            outlet.set_on_off(isOnButton,False)
                for rfPairOff in rfPairsOff:
                    if rfPairOff:
                        outlets = RF433Outlet.objects.filter(recv_triggers=rfPairOff)
                        bulbs = ShellyBulb.objects.filter(recv_triggers=rfPairOff)
                        isOnButton = False
                        for bulb in bulbs:
                            bulb.set_light_on_off(isOnButton)
                        for outlet in outlets:
                            outlet.set_on_off(isOnButton,False)

            elif module_type == 'lora':
                if dev_id == 103 and info_type == "garage-status":
                    logger.info("sub topic: {0}, payload: {1}".format(topic, payload))
                    channel_layer = get_channel_layer()
                    async_to_sync(channel_layer.group_send)(
                        'device_updates',
                        {
                            'type': 'mqtt_garage_update',
                            'message': payload
                        }
                    )
                elif dev_id == 103 and info_type == "logging":
                    now = datetime.datetime.now()
                    now_string = now.strftime("%m/%d/%Y %H:%M:%S")
                    logger.info(now_string + " " + str(payload))

        except Exception as e:
            logger.error("EXCEPTION! in mqtt consumers")
            logger.error(e)
            pass

        # channel_layer = get_channel_layer()
        # async_to_sync(channel_layer.group_send)(
        #     'testmqtt',
        #     {
        #         'type': 'handle_mqtt',
        #         'message': {"rf_outlet_toggle": 1}
        #     }
        # )


    def mqtt_pub(self, event):
        topic = event['text']['topic']
        payload = event['text']['payload']
        # do something with topic and payload

        logger.debug("pub topic: {0}, payload: {1}".format(topic, payload))

