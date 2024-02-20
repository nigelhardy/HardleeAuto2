from django.urls import path

from . import views
from django.urls import include, path

urlpatterns = [
    path('', views.index, name='index'),
    path('garage/', views.garage, name='garage'),
    path('accounts/', include('django.contrib.auth.urls')),
]
