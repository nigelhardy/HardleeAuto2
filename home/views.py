from django.http import HttpResponse
from devices.models import RFOutlet, RGBLight
from django.template import loader


def index(request):
    rf_outlets = RFOutlet.objects.all()
    rgb_lights = RGBLight.objects.all()
    template = loader.get_template('home/index.html')
    context = {
        'rf_outlets': rf_outlets,
        'rgb_lights': rgb_lights
    }
    return HttpResponse(template.render(context, request))
