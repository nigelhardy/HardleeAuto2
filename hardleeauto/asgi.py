"""
ASGI config for hardleeauto project.

It exposes the ASGI callable as a module-level variable named ``application``.

For more information on this file, see
https://docs.djangoproject.com/en/4.0/howto/deployment/asgi/
"""

import os

from channels.auth import AuthMiddlewareStack
from channels.layers import get_channel_layer
from channels.routing import ProtocolTypeRouter, URLRouter, ChannelNameRouter
from django.core.asgi import get_asgi_application
from mqttapp.consumers import MqttConsumer

import django

os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'hardleeauto.settings')
django.setup()
import devices.routing

application = ProtocolTypeRouter({
    "http": get_asgi_application(),
    "websocket": AuthMiddlewareStack(
        URLRouter(
            devices.routing.websocket_urlpatterns
        )
    ),
    "channel": ChannelNameRouter({
        "mqtt": MqttConsumer.as_asgi(),
    }),
})

# Layers
channel_layer = get_channel_layer()
