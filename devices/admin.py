from django.contrib import admin
from .models import RF433Outlet, RF433Module, RGBLight, ShellyBulb, Garage

admin.site.register(RF433Outlet)
admin.site.register(RF433Module)
admin.site.register(RGBLight)
admin.site.register(ShellyBulb)
admin.site.register(Garage)

