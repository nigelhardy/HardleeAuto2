from asgiref.sync import async_to_sync
from django.db import models
import socket
import json
import logging

logger = logging.getLogger(__name__)

from channels.layers import get_channel_layer


class RF433Module(models.Model):
    name = models.CharField(max_length=300)
    unique_id = models.IntegerField(unique=True)
    is_active = models.BooleanField(default=True)
    rf433_topic = "rf433tx"

    def send_rf_outlet_command(self, rf_outlet):
        topic = self.rf433_topic + "/" + str(self.unique_id) + "/send-command"
        channel_layer = get_channel_layer()
        logger.info("topic sending " + topic)
        status = {"id": rf_outlet.id, "is_on": rf_outlet.is_on,
                  "rf_payload": rf_outlet.get_payload()}
        logger.info(status)
        async_to_sync(channel_layer.send)('mqtt.pub', {  # also needs to be mqtt.pub
            'type': 'mqtt.pub',  # necessary to be mqtt.pub
            'text': {
                'topic': topic,
                'payload': json.dumps(status)
            }
        })
        pass

    def __str__(self):
        return self.name


class RF433Outlet(models.Model):
    name = models.CharField(max_length=300)
    on_payload = models.IntegerField(default=-1)
    recv_trigger = models.IntegerField(default=-1)
    is_on = models.BooleanField(default=False)
    rf_433_mqtt = models.ForeignKey(RF433Module, on_delete=models.CASCADE)

    def get_payload(self):
        if self.is_on:
            return self.on_payload
        else:
            return self.on_payload + 9

    def toggle(self):
        logger.info(self.is_on)
        self.is_on = not self.is_on
        self.rf_433_mqtt.send_rf_outlet_command(self)
        self.save()

    def get_state_dict(self):
        return {"id": self.id, "is_on": self.is_on}

    def get_json_state(self):
        return json.dumps({
            'rf_outlet_status': self.get_state_dict()
        })

    def __str__(self):
        return self.name


class RGBLight(models.Model):
    name = models.CharField(max_length=300)
    unique_id = models.IntegerField(unique=True)
    is_on = models.BooleanField(default=False)
    is_active = models.BooleanField(default=True)
    red = models.IntegerField(default=0)
    green = models.IntegerField(default=0)
    blue = models.IntegerField(default=0)
    hexcolor = models.CharField(max_length=32)
    # static
    rgb_topic = "rgbw-strip"

    def toggle(self):
        logger.info("toggle")
        logger.info(self.is_on)
        self.is_on = not self.is_on
        logger.info(self.is_on)
        self.set_color_mqtt()
    def get_state_dict(self):
        return {"id": self.id, "red": self.red, "green": self.green, "blue": self.blue, "is_on": self.is_on,
                "is_active": self.is_active, "unique_id": self.unique_id}

    def get_json_state(self):
        return json.dumps({
            'rgb_light_status': self.get_state_dict()
        })

    def set_color_mqtt(self):
        topic = self.rgb_topic + "/" + str(self.unique_id) + "/set-color"
        channel_layer = get_channel_layer()
        logger.info("topic sending " + topic)
        logger.info(self.is_on)
        status = {"id": self.id, "red": self.red, "green": self.green, "blue": self.blue, "is_on": self.is_on,
                  "is_active": self.is_active}
        logger.info(status)
        async_to_sync(channel_layer.send)('mqtt.pub', {  # also needs to be mqtt.pub
            'type': 'mqtt.pub',  # necessary to be mqtt.pub
            'text': {
                'topic': topic,
                'payload': json.dumps(status)
            }
        })
        pass
    def save(self, *args, **kwargs):
        self.hexcolor = f'{self.red:02X}{self.green:02X}{self.blue:02X}'
        super(RGBLight, self).save(*args, **kwargs)

    def __str__(self):
        return self.name
