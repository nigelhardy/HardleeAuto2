from asgiref.sync import async_to_sync
from django.db import models
import socket
import json

from channels.layers import get_channel_layer


class RGBLight(models.Model):
    name = models.CharField(max_length=300)
    unique_id = models.IntegerField(unique=True)
    is_on = models.BooleanField(default=False)
    is_active = models.BooleanField(default=True)
    red = models.IntegerField(default=0)
    green = models.IntegerField(default=0)
    blue = models.IntegerField(default=0)
    # static
    rgb_topic = "rgb-light"

    def toggle(self):
        print("toggle")
        print(self.is_on)
        self.is_on = not self.is_on
        print(self.is_on)
        self.set_color_mqtt()

    def get_state_dict(self):
        return {"id": self.id, "red": self.red, "green": self.green, "blue": self.blue, "is_on": self.is_on,
                "is_active": self.is_active}

    def get_json_state(self):
        return json.dumps({
            'rgb_light_status': self.get_state_dict()
        })

    def set_color_mqtt(self):
        topic = self.rgb_topic + "/" + str(self.unique_id) + "/set-color"
        channel_layer = get_channel_layer()
        print("topic sending " + topic)
        print(self.is_on)
        status = {"id": self.id, "red": self.red, "green": self.green, "blue": self.blue, "is_on": self.is_on,
                  "is_active": self.is_active}
        print(status)
        async_to_sync(channel_layer.send)('mqtt.pub', {  # also needs to be mqtt.pub
            'type': 'mqtt.pub',  # necessary to be mqtt.pub
            'text': {
                'topic': topic,
                'payload': json.dumps(status)
            }
        })
        pass


class RF433Module(models.Model):
    name = models.CharField(max_length=200)
    ip_address = models.CharField(max_length=15, default="127.0.0.1")

    def __str__(self):
        return self.name


class RFOutlet(models.Model):
    name = models.CharField(max_length=200)
    # This value is the 'on' command via 433Mhz
    # add 9 to this value for the 'off' command
    wireless_address = models.IntegerField(default="0")
    status = models.BooleanField(default=False)
    rf_433_module = models.ForeignKey(RF433Module, on_delete=models.CASCADE, blank=True, null=True)

    def toggle(self):
        self.status = not self.status
        if self.send_command():
            self.save()

    def send_command(self):
        return True
        try:
            # Temporary send to python server instead of real devices
            s = socket.socket()
            port = 12345
            ip_address = "127.0.0.1"
            if self.rf_433_module is not None:
                ip_address = self.rf_433_module.ip_address
            s.connect((ip_address, port))
            s.sendall(bytes(self.get_json_state(), encoding="utf-8"))
            recv_data = s.recv(1024).decode()
            s.close()
            if recv_data != "success":
                return False
            return True
        except Exception as e:
            print(e.what())
            return False

    def get_json_state(self):
        return json.dumps({
            'rf_outlet_status': {self.id: self.status}
        })

    def set(self, status):
        self.status = status
        if self.send_command():
            self.save()
        else:
            print('Unable to change rf outlet state!')

    def __str__(self):
        return self.name

    # def update(self):
    #     send_code = int(self.wireless_address)
    #     if not self.status:
    #         send_code += 9
    #     return True
