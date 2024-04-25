from django.http import HttpResponse
from devices.models import RF433Outlet, RGBLight, ShellyBulb, Garage
from django.template import loader
from django.contrib.auth.decorators import login_required
from channels.layers import get_channel_layer
from asgiref.sync import async_to_sync
import logging
import json
from django.shortcuts import redirect

logger = logging.getLogger(__name__)

@login_required()
def index(request):

    rf_outlets = RF433Outlet.objects.all().order_by('order')
    rgb_lights = RGBLight.objects.all()
    shelly_bulbs = ShellyBulb.objects.all()
    garages = Garage.objects.all()

    template = loader.get_template('home/index.html')
    context = {
        'rf_outlets': rf_outlets,
        'rgb_lights': rgb_lights,
        'shelly_bulbs': shelly_bulbs,
        'garages': garages
    }
    return HttpResponse(template.render(context, request))

@login_required()
def wol(request):
    logger.info("WOL")
    # send mqtt
    channel_layer = get_channel_layer()
    topic = "esp_wol/105/pwr-btn"
    payload = "1000"
    async_to_sync(channel_layer.send)('mqtt.pub', {  # also needs to be mqtt.pub
        'type': 'mqtt.pub',  # necessary to be mqtt.pub
        'text': {
            'topic': topic,
            'payload': payload
            }
        })
    return redirect(index)

@login_required()
def garage(request):

    garages = Garage.objects.all()
    template = loader.get_template('home/garage.html')
    context = {
            'garages': garages
    }
    return HttpResponse(template.render(context, request))
