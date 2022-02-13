from django.core.exceptions import ObjectDoesNotExist
from django.http import HttpResponse
from django.shortcuts import redirect

from devices.models import RF433Outlet
from django.template import loader


def toggle_rf_outlet(request, rf_outlet_id):
    try:
        outlet = RF433Outlet.objects.get(id=rf_outlet_id)
        outlet.toggle()
        outlet.save()
        return redirect("/")
        # return HttpResponse('Success. Outlet \"' + str(outlet.name) + "\" is " + str(outlet.status) + '.')
    except ObjectDoesNotExist as e:
        return HttpResponse('Failure, rf outlet doesn\'t exist!')
