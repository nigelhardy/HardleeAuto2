from django.http import HttpResponse
from devices.models import RFOutlet
from django.template import loader


def index(request):
    rf_outlets = RFOutlet.objects.all()
    template = loader.get_template('home/index.html')
    context = {
        'rf_outlets': rf_outlets
    }
    return HttpResponse(template.render(context, request))
