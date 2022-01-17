from django.db import models


class RFOutlet(models.Model):
    name = models.CharField(max_length=200)
    # This value is the 'on' command via 433Mhz
    # add 9 to this value for the 'off' command
    wireless_address = models.IntegerField(default="0")
    status = models.BooleanField(default=False)

    def toggle(self):
        self.status = not self.status
        self.save()
        self.update()

    def set(self, status):
        self.status = status
        self.save()
        self.update()

    def update(self):
        # outlets = RFOutlet.objects.all()
        # outlet_status = {}
        # for outlet in outlets:
        #     outlet_status[outlet.id] = outlet.status

        # chans = ConnectedClients.objects.all()
        # channel_names = []
        # for chan in chans:
        #     channel_names.append(chan.channel_name)
        send_code = int(self.wireless_address)

        if not self.status:
            send_code += 9
        # send_serial_command.apply_async(('http://localhost:3000/send_433', .2,
        #                                  {'address':send_code},
        #                                  {'w_outlets': outlet_status}, channel_names))
        return True
