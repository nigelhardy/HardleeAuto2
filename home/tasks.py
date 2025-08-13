import time
import logging
from asgiref.sync import async_to_sync
from channels.consumer import SyncConsumer
from channels.layers import get_channel_layer
import paho.mqtt.client as paho
from devices.models import RF433Outlet, RGBLight, ShellyBulb, RF_OnOffPair

logger = logging.getLogger(__name__)

def send_mqtt_message():
    # send mqtt
    rf_outlets_dyson = RF433Outlet.objects.filter(name__icontains='dyson')
    for rf_outlet in rf_outlets_dyson:
        rf_outlet.set_on_off(False)

