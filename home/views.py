from django.http import HttpResponse
from devices.models import RF433Outlet, RGBLight
from django.template import loader
from django.contrib.auth.decorators import login_required


@login_required(login_url="admin/")
def index(request):
    rf_outlets = RF433Outlet.objects.all()
    rgb_lights = RGBLight.objects.all()
    template = loader.get_template('home/index.html')
    context = {
        'rf_outlets': rf_outlets,
        'rgb_lights': rgb_lights
    }
    return HttpResponse(template.render(context, request))
