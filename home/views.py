from django.http import HttpResponse
from devices.models import RF433Outlet, RGBLight, ShellyBulb, Garage
from django.template import loader
from django.contrib.auth.decorators import login_required
from channels.layers import get_channel_layer
from asgiref.sync import async_to_sync
import logging
import json

logger = logging.getLogger(__name__)

@login_required()
def index(request):

    rf_outlets = RF433Outlet.objects.all()
    rgb_lights = RGBLight.objects.all()
    shelly_bulbs = ShellyBulb.objects.all()

    topic = "lora/103/garage-cmd"
    channel_layer = get_channel_layer()
    async_to_sync(channel_layer.send)('mqtt.pub', {  # also needs to be mqtt.pub
        'type': 'mqtt.pub',  # necessary to be mqtt.pub
        'text': {
            'topic': topic,
            'payload': json.dumps({"cmd":"query"})
        }
    })

    template = loader.get_template('home/index.html')
    context = {
        'rf_outlets': rf_outlets,
        'rgb_lights': rgb_lights,
        'shelly_bulbs': shelly_bulbs
    }
    return HttpResponse(template.render(context, request))

@login_required()
def garage(request):

    topic = "lora/103/garage-cmd"
    channel_layer = get_channel_layer()
    async_to_sync(channel_layer.send)('mqtt.pub', {  # also needs to be mqtt.pub
        'type': 'mqtt.pub',  # necessary to be mqtt.pub
        'text': {
            'topic': topic,
            'payload': json.dumps({"cmd":"query"})
        }
    })

    garages = Garage.objects.all()
    template = loader.get_template('home/garage.html')
    context = {
            'garages': garages
    }
    return HttpResponse(template.render(context, request))
