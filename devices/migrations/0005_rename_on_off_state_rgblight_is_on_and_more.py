# Generated by Django 4.0.1 on 2022-01-29 18:18

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('devices', '0004_rgblight'),
    ]

    operations = [
        migrations.RenameField(
            model_name='rgblight',
            old_name='on_off_state',
            new_name='is_on',
        ),
        migrations.AddField(
            model_name='rgblight',
            name='is_active',
            field=models.BooleanField(default=True),
        ),
    ]
