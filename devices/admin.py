from django.contrib import admin
from .models import RF433Outlet, RF433Module, RGBLight

admin.site.register(RF433Outlet)
admin.site.register(RF433Module)
admin.site.register(RGBLight)

