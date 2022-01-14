from django.http import HttpResponse
from .models import WelcomeMessage
from django.template import loader


def index(request):
    welcome_message_list = WelcomeMessage.objects.order_by('-pub_date')
    template = loader.get_template('home/index.html')
    context = {
        'welcome_message_list': welcome_message_list
    }
    return HttpResponse(template.render(context, request))
