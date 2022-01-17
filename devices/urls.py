from django.urls import path

from . import views

urlpatterns = [
    path('toggle_rf_outlet/<int:rf_outlet_id>/', views.toggle_rf_outlet, name='toggle rf outlet'),
]