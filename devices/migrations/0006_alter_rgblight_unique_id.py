# Generated by Django 4.0.1 on 2022-01-29 19:18

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('devices', '0005_rename_on_off_state_rgblight_is_on_and_more'),
    ]

    operations = [
        migrations.AlterField(
            model_name='rgblight',
            name='unique_id',
            field=models.IntegerField(unique=True),
        ),
    ]
